#include "../event.c.h"
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <libcc/timeout.h>
#include <linux/io_uring.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>


#define _CC_IO_URING_EVENTS_ _CC_MAX_CHANGE_EVENTS_

#ifndef __NR_io_uring_setup
#define __NR_io_uring_setup  425
#endif
#ifndef __NR_io_uring_enter
#define __NR_io_uring_enter  426
#endif
#ifndef __NR_io_uring_register
#define __NR_io_uring_register 427
#endif

const uint32_t G_MAX_WAIT_REQ = 2000;

struct cq_ring_offsets {
    uint32_t head;
    uint32_t tail;
    uint32_t ring_mask;
    uint32_t ring_entries;
    uint32_t overflow;
    uint32_t cqes;
    uint64_t reserved0;
    uint64_t reserved1;
};

struct sq_ring_offsets {
    uint32_t head;
    uint32_t tail;
    uint32_t ring_mask;
    uint32_t ring_entries;
    uint32_t flags;
    uint32_t dropped;
    uint32_t array;
    uint32_t reserved0;
    uint64_t reserved1;
};

struct io_uring {
    struct sq_ring_offsets sq;
    struct cq_ring_offsets cq;
    struct io_uring_sqe *sqes;
};

struct _cc_async_event_priv {
    int fd;
    struct io_uring_params params;
    struct io_uring uring;
    pvoid_t sq_ptr;
    pvoid_t cq_ptr;
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

/**
 * @brief Get a Submission Queue Entry (SQE) from the SQ ring.
 *
 * @param ring Pointer to the io_uring structure.
 * @return struct io_uring_sqe* Pointer to the SQE, or NULL if the ring is full.
 * @note This function updates the SQ tail pointer if a SQE is successfully obtained.
 * @warning Caller must ensure the ring is not full before calling this function.
 */
static struct io_uring_sqe *get_sqe_from_ring(struct io_uring *uring) {
    struct io_uring_sqe *sqe;
    unsigned tail = uring->sq.tail;
    unsigned next_tail = tail + 1;

    /* Check if the ring is full */
    if (next_tail - uring->sq.head > uring->sq.ring_entries) {
        _cc_logger_error(_T("SQ ring is full"));
        return nullptr;
    }

    /* Get the SQE from the ring */
    sqe = &uring->sqes[tail & uring->sq.ring_mask];

    /* Update the tail pointer (visible to the kernel) */
    uring->sq.tail = next_tail;
    return sqe;
}

/**/
_CC_API_PRIVATE(bool_t) _io_uring_event_update(_cc_async_event_t *async, _cc_event_t *e, bool_t rm) {
    uint16_t filter = 0;
    struct io_uring_sqe *sqe;
    _cc_async_event_priv_t *priv = (_cc_async_event_priv_t *)async->priv;
    unsigned to_submit = 0;

    if (rm) {
        /* Remove event from io_uring */
        sqe = get_sqe_from_ring(&priv->uring);
        if (!sqe) {
            _cc_logger_error(_T("Failed to get SQE for remove event on fd %d"), e->fd);
            return false;
        }
        sqe->opcode = IORING_OP_NOP;
        sqe->user_data = (uintptr_t)e;
        e->filter = 0;
        if (io_uring_enter(priv->fd, 1, 0, 0, NULL) < 0) {
            _cc_logger_error(_T("io_uring_enter failed: %s"), _cc_last_error(_cc_last_errno()));
            return false;
        }
        return true;
    }

    /* Setting the readable event flag */
    if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags)) {
        sqe = get_sqe_from_ring(&priv->uring);
        if (!sqe) {
            _cc_logger_error(_T("Failed to get SQE for read event on fd %d"), e->fd);
            return false;
        }
        sqe->opcode = IORING_OP_RECV;
        sqe->fd = e->fd;
        if (_CC_ISSET_BIT(_CC_EVENT_BUFFER_, e->flags) && e->buffer) {
            sqe->addr = (uintptr_t)(e->buffer->r.bytes + e->buffer->r.length);
            sqe->len = e->buffer->r.limit - e->buffer->r.length;
        } else {
            sqe->addr = 0;
            sqe->len = 0;
        }
        sqe->user_data = (uintptr_t)e;
        filter = _CC_EVENT_IS_SOCKET(e->flags);
        to_submit++;
    }

    /* Setting the writable event flag */
    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, e->flags)) {
        sqe = get_sqe_from_ring(&priv->uring);
        if (!sqe) {
            _cc_logger_error(_T("Failed to get SQE for write event on fd %d"), e->fd);
            return false;
        }
        sqe->opcode = IORING_OP_WRITE;
        sqe->fd = e->fd;
        if (_CC_ISSET_BIT(_CC_EVENT_BUFFER_, e->flags) && e->buffer) {
            sqe->addr = (uintptr_t)(e->buffer->w.bytes);
            sqe->len = e->buffer->w.length;
        } else {
            sqe->addr = 0;
            sqe->len = 0;
        }
        sqe->user_data = (uintptr_t)e;
        filter = _CC_EVENT_IS_SOCKET(e->flags);
        to_submit++;
    }

    if (io_uring_enter(priv->fd, to_submit, 0, 0, NULL) < 0) {
        _cc_logger_error(_T("io_uring_enter failed: %s"), _cc_last_error(_cc_last_errno()));
        return false;
    }
    e->filter = filter;
    return true;
}


/**/
_CC_API_PRIVATE(bool_t) _io_uring_event_attach(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_assert(async != nullptr && e != nullptr);
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _io_uring_event_reset(_cc_async_event_t *async, _cc_event_t *e) {
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _io_uring_event_disconnect(_cc_async_event_t *async, _cc_event_t *e) {
    return _disconnect_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _io_uring_event_connect(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _io_uring_event_attach(async, e);
    }
    return false;
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _io_uring_event_accept(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    uint16_t m = _CC_EVENT_IS_SOCKET(e->filter), u;
    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        if (m) {
            _io_uring_event_update(async, e, true);
        }
        _cc_free_event(async, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        if (m) {
            _io_uring_event_update(async, e, true);
        }

        _cc_list_iterator_swap(&async->pending, &e->lnk);
        return;
    }

    /*update event*/
    u = _CC_EVENT_IS_SOCKET(e->flags);
    if (u && u != m) {
        _io_uring_event_update(async, e, false);
    }

    _reset_event_timeout(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _io_uring_event_wait(_cc_async_event_t *async, uint32_t timeout) {
    _cc_async_event_priv_t *priv = async->priv;
    struct io_uring_cqe *cqes[_CC_IO_URING_EVENTS_];
    int32_t rc,i;
    struct __kernel_timespec ts;

    _reset_event_pending(async, _reset);

    if (async->diff > 0) {
        timeout = timeout > async->diff ? timeout - async->diff : 0;
    }

    ts.tv_sec = timeout / 1000;
    ts.tv_nsec = (timeout % 1000) * 1000000;

    rc = io_uring_wait_cqe_timeout(&priv->uring, &cqes[0], &ts);
    if (rc < 0) {
        int32_t lerrno = _cc_last_errno();
        if (lerrno != _CC_EINTR_) {
            _cc_logger_error(_T("io_uring_wait_cqe error:%d, %s"), lerrno, _cc_last_error(lerrno));
        }
        goto URING_END;
    }

    rc = io_uring_peek_batch_cqe(&priv->uring, cqes, _CC_IO_URING_EVENTS_);
    if (rc > 0) {
        for (i = 0; i < rc; ++i) {
            struct io_uring_cqe *cqe = cqes[i];
            _cc_event_t *e = (_cc_event_t *)cqe->user_data;
            uint32_t which = _CC_EVENT_UNKNOWN_;

            if (cqe->res < 0) {
                which = _CC_EVENT_CLOSED_;
            } else {
                switch (cqe->opcode) {
                case IORING_OP_RECV:
                    which |= _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
                    if ((which & _CC_EVENT_READABLE_) && e->buffer && (e->flags & _CC_EVENT_BUFFER_)) {
                        e->buffer->r.length += cqe->res;
                    }
                    break;
                case IORING_OP_WRITE:
                    which |= _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags);
                    if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, which)) {
                        which = _valid_connected(e, which);
                    } else if (e->buffer && (e->flags & _CC_EVENT_BUFFER_)) {
                        _cc_event_wbuf_t *wbuf;
                        _cc_assert(e->buffer != nullptr);

                        wbuf = &e->buffer->w;
                        if (wbuf->length == 0) {
                            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
                            break;
                        }
                        _cc_spin_lock(&wbuf->lock);
                        if (cqe->res < 0) {
                            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
                            wbuf->length = 0;
                        } else if (cqe->res > 0) {
                            if (cqe->res < wbuf->length) {
                                wbuf->length -= cqe->res;
                                memmove(wbuf->bytes, wbuf->bytes + cqe->res, wbuf->length);
                            } else {
                                _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
                                wbuf->length = 0;
                            }
                        }
                        _cc_unlock(&wbuf->lock);
                    }
                    break;
                }
            }
            
            if (which) {
                _event_callback(async, e, which);
            }
        }
        io_uring_cq_advance(&priv->uring, rc);
    }

URING_END:
    _update_event_timeout(async, timeout);
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _io_uring_event_free(_cc_async_event_t *async) {
    _cc_assert(async != nullptr);
    if (async == nullptr) {
        return false;
    }

    if (async->priv) {
        _cc_async_event_priv_t *priv = async->priv;
        struct io_uring_params *p = &priv->params;
        munmap(priv->sq_ptr, p->sq_off.array + p->sq_entries * sizeof(__u32));
        munmap(priv->cq_ptr, p->cq_off.cqes + p->cq_entries * sizeof(struct io_uring_cqe));
        munmap(priv->uring.sqes, p->sq_entries * sizeof(struct io_uring_sqe));

        if (priv->fd != -1) {
            close(priv->fd);
        }
        _cc_free(priv);
        async->priv = nullptr;
    }

    return _unregister_async_event(async);
}


/**/
_CC_API_PRIVATE(bool_t) _io_uring_event_alloc(_cc_async_event_t *async) {
    pvoid_t sq_ptr, cq_ptr;
    struct io_uring *uring;
    struct io_uring_sqe *sqes;

    struct _cc_async_event_priv *priv;
    struct io_uring_params params = {
        .flags = IORING_SETUP_SQPOLL,
        .sq_thread_idle = 1000
    };

    priv = (_cc_async_event_priv_t *)_cc_malloc(sizeof(_cc_async_event_priv_t));
    priv->fd = io_uring_setup(_CC_IO_URING_EVENTS_, &params);
    if (priv->fd < 0) {
        _cc_logger_error(_T("io_uring_setup failed: %s"), _cc_last_error(_cc_last_errno()));
        return false;
    }
    memcpy(&priv->params, &params, sizeof(struct io_uring_params));
    /* IORING_FEAT_RSRC_TAGS is used to detect linux v5.13 but what we're
     * actually detecting is whether IORING_OP_STATX works with SQPOLL.
     */
    if (!(params.features & IORING_FEAT_RSRC_TAGS)) {
        _cc_logger_error(_T("io_uring_setup failed: %s"), _cc_last_error(_cc_last_errno()));
        close(priv->fd);
        _cc_free(priv);
        return false;
    }

    /* Implied by IORING_FEAT_RSRC_TAGS but checked explicitly anyway. */
    if (!(params.features & IORING_FEAT_SINGLE_MMAP)) {
        _cc_logger_error(_T("io_uring_setup failed: %s"), _cc_last_error(_cc_last_errno()));
        close(priv->fd);
        _cc_free(priv);
        return false;
    }

    /* Implied by IORING_FEAT_RSRC_TAGS but checked explicitly anyway. */
    if (!(params.features & IORING_FEAT_NODROP)) {
        _cc_logger_error(_T("io_uring_setup failed: %s"), _cc_last_error(_cc_last_errno()));
        close(priv->fd);
        _cc_free(priv);
        return false;
    }

    uring = &priv->uring;
    bzero(uring, sizeof(struct io_uring));
    // SQ
    sq_ptr = mmap(NULL, 
                    params.sq_off.array + params.sq_entries * sizeof(__u32),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_POPULATE,
                    priv->fd,
                    IORING_OFF_SQ_RING);
    if (sq_ptr == MAP_FAILED) {
        close(priv->fd);
        _cc_free(priv);
        return false;
    }

    // CQ
    cq_ptr = mmap(NULL,
                    params.cq_off.cqes + params.cq_entries * sizeof(struct io_uring_cqe),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_POPULATE,
                    priv->fd,
                    IORING_OFF_CQ_RING);
    if (cq_ptr == MAP_FAILED) {
        munmap(sq_ptr, params.sq_off.array + params.sq_entries * sizeof(__u32));
        close(priv->fd);
        _cc_free(priv);
        return false;
    }

    sqes = mmap(NULL,
                    params.sq_entries * sizeof(struct io_uring_sqe),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_POPULATE | MAP_LOCKED,
                    priv->fd,
                    IORING_OFF_SQES);
    if (sqes == MAP_FAILED) {
        munmap(sq_ptr, params.sq_off.array + params.sq_entries * sizeof(__u32));
        munmap(cq_ptr, params.cq_off.cqes + params.cq_entries * sizeof(struct io_uring_cqe));
        close(priv->fd);
        _cc_free(priv);
        return false;
    }
    
    priv->cq_ptr = cq_ptr;
    priv->sq_ptr = sq_ptr;

    /* Initialize SQ ring */
    uring->sq.ring_mask = *(unsigned *)((char *)sq_ptr + params.sq_off.ring_mask);
    uring->sq.ring_entries = *(unsigned *)((char *)sq_ptr + params.sq_off.ring_entries);
    uring->sq.head = (unsigned *)((char *)sq_ptr + params.sq_off.head);
    uring->sq.tail = (unsigned *)((char *)sq_ptr + params.sq_off.tail);
    uring->sqes = sqes;

    /* Initialize CQ ring */
    uring->cq.ring_mask = *(unsigned *)((char *)cq_ptr + params.cq_off.ring_mask);
    uring->cq.ring_entries = *(unsigned *)((char *)cq_ptr + params.cq_off.ring_entries);
    uring->cq.head = (unsigned *)((char *)cq_ptr + params.cq_off.head);
    uring->cq.tail = (unsigned *)((char *)cq_ptr + params.cq_off.tail);

    async->priv = priv;

    return true;
}


/**/
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
    async->reset = _io_uring_event_reset;
    return true;
}