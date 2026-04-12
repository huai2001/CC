#include "../../event/event.c.h"
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <libcc/timeout.h>
#include <linux/io_uring.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define _CC_IO_URING_EVENTS_ _CC_MAX_CHANGE_EVENTS_

#ifndef __NR_io_uring_setup
#define __NR_io_uring_setup 425
#endif
#ifndef __NR_io_uring_enter
#define __NR_io_uring_enter 426
#endif
#ifndef __NR_io_uring_register
#define __NR_io_uring_register 427
#endif

#ifndef POLLRDHUP
#define POLLRDHUP 0
#endif

struct _cc_io_uring_sq {
    unsigned *khead;
    unsigned *ktail;
    unsigned *kring_mask;
    unsigned *kring_entries;
    unsigned *kflags;
    unsigned *kdropped;
    unsigned *array;
    struct io_uring_sqe *sqes;
    unsigned sqe_head;
    unsigned sqe_tail;
    void *ring_ptr;
    size_t ring_sz;
    void *sqes_ptr;
    size_t sqes_sz;
};

struct _cc_io_uring_cq {
    unsigned *khead;
    unsigned *ktail;
    unsigned *kring_mask;
    unsigned *kring_entries;
    unsigned *koverflow;
    struct io_uring_cqe *cqes;
    void *ring_ptr;
    size_t ring_sz;
};

struct _cc_async_event_priv {
    int fd;
    struct io_uring_params params;
    struct _cc_io_uring_sq sq;
    struct _cc_io_uring_cq cq;
};

_CC_FORCE_INLINE_ int io_uring_setup(unsigned entries, struct io_uring_params *p) {
    return syscall(__NR_io_uring_setup, entries, p);
}

_CC_FORCE_INLINE_ int io_uring_enter(int fd, unsigned to_submit, unsigned min_complete, unsigned flags, sigset_t *sig) {
    return syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, sig, 0L);
}

_CC_FORCE_INLINE_ int io_uring_register(int fd, unsigned opcode, const void *arg, unsigned nr_args) {
    return syscall(__NR_io_uring_register, fd, opcode, arg, nr_args);
}

_CC_FORCE_INLINE_ short _io_uring_poll_mask(const _cc_event_t *e) {
    short mask = POLLERR | POLLHUP | POLLRDHUP;

    if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags)) {
        mask |= POLLIN;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, e->flags)) {
        mask |= POLLOUT;
    }

    return mask;
}

_CC_FORCE_INLINE_ uint32_t _io_uring_cq_ready(_cc_async_event_priv_t *priv) {
    return *priv->cq.ktail - *priv->cq.khead;
}

static struct io_uring_sqe *_io_uring_get_sqe(_cc_async_event_priv_t *priv) {
    struct _cc_io_uring_sq *sq = &priv->sq;
    unsigned head = *sq->khead;
    unsigned tail = sq->sqe_tail;
    struct io_uring_sqe *sqe;

    if ((tail - head) >= *sq->kring_entries) {
        _cc_logger_error("io_uring SQ ring is full");
        return NULL;
    }

    sqe = &sq->sqes[tail & *sq->kring_mask];
    memset(sqe, 0, sizeof(*sqe));
    sq->sqe_tail = tail + 1;
    return sqe;
}

static int32_t _io_uring_submit(_cc_async_event_priv_t *priv) {
    struct _cc_io_uring_sq *sq = &priv->sq;
    unsigned ktail = *sq->ktail;
    unsigned pending = sq->sqe_tail - sq->sqe_head;
    unsigned index;
    int32_t rc;

    if (pending == 0) {
        return 0;
    }

    for (index = sq->sqe_head; index < sq->sqe_tail; ++index) {
        sq->array[ktail & *sq->kring_mask] = index & *sq->kring_mask;
        ktail++;
    }

    *sq->ktail = ktail;
    rc = io_uring_enter(priv->fd, pending, 0, 0, NULL);
    if (rc < 0) {
        int32_t err = _cc_last_errno();
        _cc_logger_error("io_uring_enter submit failed:%d, %s", err, _cc_last_error(err));
        return -1;
    }

    sq->sqe_head = sq->sqe_tail;
    return rc;
}

static bool_t _io_uring_queue_poll_add(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_async_event_priv_t *priv = async->priv;
    struct io_uring_sqe *sqe;
    short mask;

    if (e->fd == _CC_INVALID_SOCKET_) {
        e->filter = _CC_EVENT_UNKNOWN_;
        return true;
    }

    mask = _io_uring_poll_mask(e);
    if ((mask & ~(POLLERR | POLLHUP | POLLRDHUP)) == 0) {
        e->filter = _CC_EVENT_UNKNOWN_;
        return true;
    }

    sqe = _io_uring_get_sqe(priv);
    if (sqe == NULL) {
        return false;
    }

    sqe->opcode = IORING_OP_POLL_ADD;
    sqe->fd = e->fd;
    sqe->poll_events = (__poll_t)mask;
    sqe->user_data = e->ident;
    e->filter = _CC_EVENT_IS_SOCKET(e->flags);
    return true;
}

static bool_t _io_uring_queue_poll_remove(_cc_async_event_t *async, _cc_event_t *e) {
#ifdef IORING_OP_POLL_REMOVE
    _cc_async_event_priv_t *priv = async->priv;
    struct io_uring_sqe *sqe;

    if (_CC_EVENT_IS_SOCKET(e->filter) == 0) {
        e->filter = _CC_EVENT_UNKNOWN_;
        return true;
    }

    sqe = _io_uring_get_sqe(priv);
    if (sqe == NULL) {
        return false;
    }

    sqe->opcode = IORING_OP_POLL_REMOVE;
    sqe->addr = e->ident;
    sqe->user_data = 0;
#endif
    e->filter = _CC_EVENT_UNKNOWN_;
    return true;
}

_CC_API_PRIVATE(bool_t) _io_uring_event_update(_cc_async_event_t *async, _cc_event_t *e, bool_t rm) {
    if (rm) {
        return _io_uring_queue_poll_remove(async, e);
    }
    return _io_uring_queue_poll_add(async, e);
}

_CC_API_PRIVATE(bool_t) _io_uring_event_attach(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_assert(async != NULL && e != NULL);
    if (async->ident != _event_async_ident(e->ident)) {
        return false;
    }
    return _reset_event(async, e);
}

_CC_API_PRIVATE(bool_t) _io_uring_event_reset(_cc_async_event_t *async, _cc_event_t *e) {
    return _reset_event(async, e);
}

_CC_API_PRIVATE(bool_t) _io_uring_event_disconnect(_cc_async_event_t *async, _cc_event_t *e) {
    return _disconnect_event(async, e);
}

_CC_API_PRIVATE(bool_t) _io_uring_event_connect(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _io_uring_event_attach(async, e);
    }
    return false;
}

_CC_API_PRIVATE(_cc_socket_t) _io_uring_event_accept(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    uint16_t armed = _CC_EVENT_IS_SOCKET(e->filter);
    uint16_t desired;

    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        if (armed) {
            _io_uring_event_update(async, e, true);
        }
        _cc_free_event(async, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        if (armed) {
            _io_uring_event_update(async, e, true);
        }
        if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
            _cc_list_swap(&async->pending, &e->lnk);
            return;
        }
    } else {
        desired = _CC_EVENT_IS_SOCKET(e->flags);
        if (desired) {
            _io_uring_event_update(async, e, false);
        }
    }

    _reset_event_timeout(async, e);
}

_CC_API_PRIVATE(bool_t) _io_uring_event_wait(_cc_async_event_t *async, uint32_t timeout) {
    _cc_async_event_priv_t *priv = async->priv;
    struct pollfd pfd;
    int32_t rc;
    unsigned head;
    unsigned tail;

    _reset_event_pending(async, _reset);
    if (_io_uring_submit(priv) < 0) {
        goto IO_URING_END;
    }

    if (async->diff > 0) {
        timeout = (async->diff >= timeout) ? 0 : (timeout - async->diff);
    }

    if (_io_uring_cq_ready(priv) == 0) {
        pfd.fd = priv->fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        rc = poll(&pfd, 1, (int32_t)timeout);
        if (rc < 0) {
            int32_t err = _cc_last_errno();
            if (err != _CC_EINTR_) {
                _cc_logger_error("io_uring poll wait failed:%d, %s", err, _cc_last_error(err));
            }
            goto IO_URING_END;
        }
    }

    head = *priv->cq.khead;
    tail = *priv->cq.ktail;
    while (head != tail) {
        struct io_uring_cqe *cqe = &priv->cq.cqes[head & *priv->cq.kring_mask];
        uint64_t user_data = cqe->user_data;
        int32_t res = cqe->res;

        if (user_data != 0) {
            _cc_event_t *e = _cc_get_event_by_id(user_data);
            if (e != NULL) {
                uint32_t which = _CC_EVENT_UNKNOWN_;

                e->filter = _CC_EVENT_UNKNOWN_;
                if (res < 0) {
                    if (res != -ECANCELED && res != -ENOENT) {
                        which = _CC_EVENT_CLOSED_;
                    }
                } else if ((res & POLLERR) || (res & POLLNVAL)) {
                    which = _CC_EVENT_CLOSED_;
                } else if (res & POLLHUP) {
                    which = _CC_EVENT_READABLE_;
                } else {
                    if (res & POLLIN) {
                        which |= _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
                    }
                    if (res & POLLOUT) {
                        which |= _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags);
                        if (which & _CC_EVENT_CONNECT_) {
                            if (_valid_fd(e->fd)) {
                                _CC_UNSET_BIT(_CC_EVENT_CONNECT_, e->flags);
                            } else {
                                which = _CC_EVENT_CLOSED_;
                            }
                        }
                    }
                    if (res & POLLRDHUP) {
                        which = _CC_EVENT_CLOSED_;
                    }
                }

                if (which) {
                    _event_callback(async, e, which);
                }
            }
        }

        head++;
    }

    *priv->cq.khead = head;

IO_URING_END:
    _update_event_timeout(async, timeout);
    return true;
}

_CC_API_PRIVATE(bool_t) _io_uring_event_free(_cc_async_event_t *async) {
    _cc_assert(async != NULL);
    if (async == NULL) {
        return false;
    }

    if (async->priv) {
        _cc_async_event_priv_t *priv = async->priv;

        if (priv->sq.sqes_ptr) {
            munmap(priv->sq.sqes_ptr, priv->sq.sqes_sz);
        }

        if (priv->sq.ring_ptr && priv->sq.ring_ptr == priv->cq.ring_ptr) {
            munmap(priv->sq.ring_ptr, priv->sq.ring_sz > priv->cq.ring_sz ? priv->sq.ring_sz : priv->cq.ring_sz);
        } else {
            if (priv->sq.ring_ptr) {
                munmap(priv->sq.ring_ptr, priv->sq.ring_sz);
            }
            if (priv->cq.ring_ptr) {
                munmap(priv->cq.ring_ptr, priv->cq.ring_sz);
            }
        }

        if (priv->fd != -1) {
            close(priv->fd);
        }

        _cc_free(priv);
        async->priv = NULL;
    }

    return _unregister_async_event(async);
}

_CC_API_PRIVATE(bool_t) _io_uring_event_alloc(_cc_async_event_t *async) {
    _cc_async_event_priv_t *priv;
    struct io_uring_params params;
    size_t sq_ring_sz;
    size_t cq_ring_sz;
    void *sq_ring_ptr;
    void *cq_ring_ptr;
    void *sqes_ptr;

    if (!_register_async_event(async)) {
        return false;
    }

    memset(&params, 0, sizeof(params));
    priv = (_cc_async_event_priv_t *)_cc_calloc(1, sizeof(*priv));
    if (priv == NULL) {
        _unregister_async_event(async);
        return false;
    }

    priv->fd = io_uring_setup(_CC_IO_URING_EVENTS_, &params);
    if (priv->fd < 0) {
        _cc_logger_error("io_uring_setup failed:%d, %s", _cc_last_errno(), _cc_last_error(_cc_last_errno()));
        _cc_free(priv);
        _unregister_async_event(async);
        return false;
    }

    memcpy(&priv->params, &params, sizeof(params));
    sq_ring_sz = params.sq_off.array + params.sq_entries * sizeof(__u32);
    cq_ring_sz = params.cq_off.cqes + params.cq_entries * sizeof(struct io_uring_cqe);

    if (params.features & IORING_FEAT_SINGLE_MMAP) {
        size_t ring_sz = sq_ring_sz > cq_ring_sz ? sq_ring_sz : cq_ring_sz;
        sq_ring_ptr = mmap(NULL, ring_sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, priv->fd, IORING_OFF_SQ_RING);
        if (sq_ring_ptr == MAP_FAILED) {
            close(priv->fd);
            _cc_free(priv);
            _unregister_async_event(async);
            return false;
        }
        cq_ring_ptr = sq_ring_ptr;
        priv->sq.ring_sz = ring_sz;
        priv->cq.ring_sz = ring_sz;
    } else {
        sq_ring_ptr = mmap(NULL, sq_ring_sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, priv->fd, IORING_OFF_SQ_RING);
        if (sq_ring_ptr == MAP_FAILED) {
            close(priv->fd);
            _cc_free(priv);
            _unregister_async_event(async);
            return false;
        }

        cq_ring_ptr = mmap(NULL, cq_ring_sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, priv->fd, IORING_OFF_CQ_RING);
        if (cq_ring_ptr == MAP_FAILED) {
            munmap(sq_ring_ptr, sq_ring_sz);
            close(priv->fd);
            _cc_free(priv);
            _unregister_async_event(async);
            return false;
        }

        priv->sq.ring_sz = sq_ring_sz;
        priv->cq.ring_sz = cq_ring_sz;
    }

    sqes_ptr = mmap(NULL,
                    params.sq_entries * sizeof(struct io_uring_sqe),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_POPULATE,
                    priv->fd,
                    IORING_OFF_SQES);
    if (sqes_ptr == MAP_FAILED) {
        if (sq_ring_ptr == cq_ring_ptr) {
            munmap(sq_ring_ptr, priv->sq.ring_sz);
        } else {
            munmap(sq_ring_ptr, priv->sq.ring_sz);
            munmap(cq_ring_ptr, priv->cq.ring_sz);
        }
        close(priv->fd);
        _cc_free(priv);
        _unregister_async_event(async);
        return false;
    }

    priv->sq.ring_ptr = sq_ring_ptr;
    priv->cq.ring_ptr = cq_ring_ptr;
    priv->sq.sqes_ptr = sqes_ptr;
    priv->sq.sqes_sz = params.sq_entries * sizeof(struct io_uring_sqe);

    priv->sq.khead = (unsigned *)((char *)sq_ring_ptr + params.sq_off.head);
    priv->sq.ktail = (unsigned *)((char *)sq_ring_ptr + params.sq_off.tail);
    priv->sq.kring_mask = (unsigned *)((char *)sq_ring_ptr + params.sq_off.ring_mask);
    priv->sq.kring_entries = (unsigned *)((char *)sq_ring_ptr + params.sq_off.ring_entries);
    priv->sq.kflags = (unsigned *)((char *)sq_ring_ptr + params.sq_off.flags);
    priv->sq.kdropped = (unsigned *)((char *)sq_ring_ptr + params.sq_off.dropped);
    priv->sq.array = (unsigned *)((char *)sq_ring_ptr + params.sq_off.array);
    priv->sq.sqes = (struct io_uring_sqe *)sqes_ptr;
    priv->sq.sqe_head = 0;
    priv->sq.sqe_tail = 0;

    priv->cq.khead = (unsigned *)((char *)cq_ring_ptr + params.cq_off.head);
    priv->cq.ktail = (unsigned *)((char *)cq_ring_ptr + params.cq_off.tail);
    priv->cq.kring_mask = (unsigned *)((char *)cq_ring_ptr + params.cq_off.ring_mask);
    priv->cq.kring_entries = (unsigned *)((char *)cq_ring_ptr + params.cq_off.ring_entries);
    priv->cq.koverflow = (unsigned *)((char *)cq_ring_ptr + params.cq_off.overflow);
    priv->cq.cqes = (struct io_uring_cqe *)((char *)cq_ring_ptr + params.cq_off.cqes);

    async->priv = priv;
    return true;
}

_CC_API_PUBLIC(bool_t) _cc_register_io_uring(_cc_async_event_t *async) {
    if (!_io_uring_event_alloc(async)) {
        return false;
    }

    async->reset = _io_uring_event_reset;
    async->attach = _io_uring_event_attach;
    async->connect = _io_uring_event_connect;
    async->disconnect = _io_uring_event_disconnect;
    async->accept = _io_uring_event_accept;
    async->wait = _io_uring_event_wait;
    async->free = _io_uring_event_free;
    return true;
}