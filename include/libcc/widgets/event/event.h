/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#ifndef _C_CC_EVENT_H_INCLUDED_
#define _C_CC_EVENT_H_INCLUDED_

#include "../dylib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __CC_WINDOWS__
    #ifndef _CC_EVENT_DISABLE_IOCP_
        #define _CC_EVENT_USE_IOCP_           1
    #endif
#endif

#define _CC_EVENT_UNKNOWN_                    0x0000
#define _CC_EVENT_ACCEPT_                     0x0001
#define _CC_EVENT_WRITABLE_                   0x0002
#define _CC_EVENT_READABLE_                   0x0004
#define _CC_EVENT_TIMEOUT_                    0x0008
#define _CC_EVENT_CONNECT_                    0x0010
#define _CC_EVENT_CONNECTED_                  _CC_EVENT_CONNECT_
#define _CC_EVENT_DISCONNECT_                 0x0020
#define _CC_EVENT_DELETED_                    0x0040


#define _CC_EVENT_CHANGING_                   0x0100
#define _CC_EVENT_PENDING_                    0x0200
#define _CC_EVENT_BUFFER_                     0x0400

#define _CC_EVENT_NONBLOCKING_                0x1000
#define _CC_EVENT_CLOEXEC_                    0x2000

/** Used in _cc_event_t to determine what the fd is */

#define _CC_EVENT_NO_DESC_                    0x0000 /**< nothing here */
#define _CC_EVENT_DESC_SOCKET_                0x0001 /**< descriptor refers to a socket */
#define _CC_EVENT_DESC_FILE_                  0x0002 /**< descriptor refers to a file */
#define _CC_EVENT_DESC_TIMER_                 0x0004 /**< descriptor refers to a timer */
#define _CC_EVENT_DESC_IPV6_                  0x0008

#define _CC_EVENT_DESC_POLL_EPOLL_            0x0010 /**< Poll uses epoll method */
#define _CC_EVENT_DESC_POLL_KQUEUE_           0x0020 /**< Poll uses kqueue method */
#define _CC_EVENT_DESC_POLL_IOCP_             0x0040 /**< Poll uses iocp method */
#define _CC_EVENT_DESC_POLL_SELECT_           0x0080 /**< Poll uses select method */
#define _CC_EVENT_DESC_POLL_POLLFD_           0x0100 /**< Poll uses poll method */
#define _CC_EVENT_DESC_POLL_DEVPOLL_          0x0200 /**< Poll uses devpoll method */

#define _CC_EVENT_WBUF_HAS_DATA(rw) ((rw)->w.r != (rw)->w.w)
#define _CC_EVENT_WBUF_NO_DATA(rw) ((rw)->w.r == (rw)->w.w)

#define _CC_TIMEOUT_MAX_LEVEL_                  4
#define _CC_TIMEOUT_NEAR_SHIFT_                 8
#define _CC_TIMEOUT_NEAR_                       (1 << _CC_TIMEOUT_NEAR_SHIFT_)
#define _CC_TIMEOUT_LEVEL_SHIFT_                6
#define _CC_TIMEOUT_LEVEL_                      (1 << _CC_TIMEOUT_LEVEL_SHIFT_)
#define _CC_TIMEOUT_NEAR_MASK_                  (_CC_TIMEOUT_NEAR_ - 1)
#define _CC_TIMEOUT_LEVEL_MASK_                 (_CC_TIMEOUT_LEVEL_ - 1)

typedef struct _cc_event _cc_event_t;
typedef struct _cc_event_buffer _cc_event_buffer_t;
typedef struct _cc_event_rbuf _cc_event_rbuf_t;
typedef struct _cc_event_wbuf _cc_event_wbuf_t;
typedef struct _cc_event_cycle_priv _cc_event_cycle_priv_t;
typedef struct _cc_event_cycle _cc_event_cycle_t;

typedef bool_t (*_cc_event_callback_t)(_cc_event_cycle_t *, _cc_event_t *, const uint16_t);

struct _cc_event_rbuf {
    int32_t length;
    int32_t limit;
    byte_t *bytes;
};

struct _cc_event_wbuf {
    int32_t w;
    int32_t r;
    int32_t limit;
    _cc_atomic_lock_t lock;
    byte_t *bytes;
};

struct _cc_event_buffer {
    _cc_event_rbuf_t r;
    _cc_event_wbuf_t w;
};

struct _cc_event {
    uint16_t descriptor;
    /* One or more _CC_EVENT_* flags */
    uint16_t flags;

    uint16_t marks;
    uint16_t round;

    uint32_t ident;

    _cc_socket_t fd;

#ifdef _CC_EVENT_USE_IOCP_
    /* private */
    _cc_socket_t accept_fd;
#endif

    /* The timer wheel */
    uint32_t timer;
    uint32_t timeout;

    _cc_list_iterator_t lnk;

    /* Read-write buffer */
    _cc_event_buffer_t *buffer;

    /* A callback function for an event. */
    _cc_event_callback_t callback;
    /* A user-supplied argument. */
    pvoid_t args;
};

struct _cc_event_cycle {
    byte_t running;
    /**/
    byte_t ident;
    /*Number of events processed*/
    int32_t processed;
    _cc_atomic32_t count;

    /*timers wheel*/
    uint32_t timer;
    uint32_t diff;
    uint64_t tick;

    _cc_list_iterator_t nears[_CC_TIMEOUT_NEAR_];
    _cc_list_iterator_t level[_CC_TIMEOUT_MAX_LEVEL_][_CC_TIMEOUT_LEVEL_];
    _cc_list_iterator_t pending;
    _cc_list_iterator_t no_timer;

    /*thread lock*/
#ifdef _CC_EVENT_USE_MUTEX_
    _cc_mutex_t *lock;
#else
    _cc_atomic_lock_t lock;
#endif

    _cc_array_t changes;

    /* private */
    _cc_event_cycle_priv_t *priv;
    /* A user-supplied argument. */
    pvoid_t args;

    /**/
    bool_t (*reset)(_cc_event_cycle_t *cycle, _cc_event_t *e);
    /**/
    bool_t (*attach)(_cc_event_cycle_t *cycle, _cc_event_t *e);
    /**/
    bool_t (*connect)(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len);
    /**/
    _cc_socket_t (*accept)(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len);
    /**/
    bool_t (*disconnect)(_cc_event_cycle_t *cycle, _cc_event_t *e);
    /**/
    bool_t (*wait)(_cc_event_cycle_t *cycle, uint32_t timeout);
    /**/
    bool_t (*quit)(_cc_event_cycle_t *cycle);
    /**/
    void (*cleanup)(_cc_event_cycle_t *cycle, _cc_event_t *e);
};

/**
 * @brief Initializes an event class
 *
 * @param cycle _cc_event_cycle_t structure
 * @param flags current event flag
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(_cc_event_t*) _cc_event_alloc(_cc_event_cycle_t *cycle, const uint32_t flags);

/**
 * @brief Free event
 *
 * @param cycle _cc_event_cycle_t structure
 * @param e _cc_event_t structure
 */
_CC_WIDGETS_API(void) _cc_free_event(_cc_event_cycle_t *cycle, _cc_event_t *e);
/**
 * @brief Get event cycle handle
 *
 * @return _cc_event_cycle_t structure
 */
_CC_WIDGETS_API(_cc_event_cycle_t *) _cc_get_event_cycle(void);

/**
 * @brief Get event handle
 *
 * @param ident
 *
 * @return _cc_event_t structure
 */
_CC_WIDGETS_API(_cc_event_t *) _cc_get_event_by_id(uint32_t ident);

/**
 * @brief Get event_cycle handle
 *
 * @param ident
 *
 * @return _cc_event_cycle_t structure
 */
_CC_WIDGETS_API(_cc_event_cycle_t *) _cc_get_event_cycle_by_id(uint32_t round);

/**
 * @brief alloc a read/write socket buffer
 *
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(_cc_event_buffer_t*) _cc_alloc_event_buffer(void);

/**
 * @brief free a read/write socket buffer
 *
 * @param 1 _cc_event_buffer_t structure
 *
 */
_CC_WIDGETS_API(void) _cc_free_event_buffer(_cc_event_buffer_t *);

/**
 * @brief alloc a read buffer
 *
 * @param 1 _cc_event_t structure
 * @param 2 data length
 */
_CC_WIDGETS_API(void) _cc_alloc_event_rbuf(_cc_event_rbuf_t *, int32_t);

/**
 * @brief alloc a write buffer
 *
 * @param 1 _cc_event_t structure
 * @param 2 data length
 */
_CC_WIDGETS_API(void) _cc_alloc_event_wbuf(_cc_event_wbuf_t *, int32_t);

/**
 * @brief Copy to a write buffer
 *
 * @param 1 _cc_event_t structure
 * @param 2 Send data buffer
 * @param 3 Send data length
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t) _cc_copy_event_wbuf(_cc_event_wbuf_t *, const byte_t *, int32_t);

/**
 * @brief Send socket buffer
 *
 * @param 1 _cc_event_t structure
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(int32_t) _cc_event_sendbuf(_cc_event_t *);

/**
 * @brief Send Socket data with write Buffer
 *
 * @param 1 _cc_event_t structure
 * @param 2 Send data buffer
 * @param 3 Send data length
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(int32_t) _cc_event_send(_cc_event_t *, const byte_t *, int32_t);

/**
 * @brief Read Socket data to Buffer
 *
 * @param 1 _cc_event_t structure
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t) _cc_event_recv(_cc_event_t *);

/**
 * @brief Socket connection event
 *
 * @param cycle _cc_event_cycle_t structure
 * @param e _cc_event_t structure
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t) _cc_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e);
/**
 * @brief Socket accept event
 *
 * @param cycle _cc_event_cycle_t structure
 * @param e _cc_event_t structure
 * @param sa Remote client of _cc_sockaddr_t structure
 * @param sa_len Size of _cc_sockaddr_t structure
 *
 * @return Socket Handle
 */
_CC_WIDGETS_API(_cc_socket_t) _cc_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len);

/**
 * @brief Socket connection event
 *
 * @param cycle _cc_event_cycle_t
 * @param e _cc_event_t structure
 * @param sa Remote server of _cc_sockaddr_t structure
 * @param sa_len Size of _cc_sockaddr_t structure
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t) _cc_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len);

/**
 * @brief Wait events
 *
 * @param cycle _cc_event_cycle_t structure
 * @param timeout timeout in seconds
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t) _cc_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout);
/**/
_CC_WIDGETS_API(bool_t) _cc_tcp_listen(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen);
/**/
_CC_WIDGETS_API(bool_t) _cc_tcp_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen);

/**/
_CC_WIDGETS_API(bool_t) _cc_init_event_select(_cc_event_cycle_t*);

#ifdef __CC_WINDOWS__
    #ifdef _CC_EVENT_USE_IOCP_
    _CC_WIDGETS_API(bool_t) _cc_init_event_iocp(_cc_event_cycle_t*);
        #define _cc_init_event_poller _cc_init_event_iocp
    #else
        #define _cc_init_event_poller _cc_init_event_select
    #endif
#elif defined(__CC_LINUX__)
    _CC_WIDGETS_API(bool_t) _cc_init_event_poll(_cc_event_cycle_t*);
    _CC_WIDGETS_API(bool_t) _cc_init_event_epoll(_cc_event_cycle_t*);
    #define _cc_init_event_poller _cc_init_event_epoll
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__) || \
    defined(__CC_FREEBSD__) || defined(__CC_OPENBSD__) ||   \
    defined(__CC_NETBSD__)
    _CC_WIDGETS_API(bool_t) _cc_init_event_poll(_cc_event_cycle_t*);
    _CC_WIDGETS_API(bool_t) _cc_init_event_kqueue(_cc_event_cycle_t*);
    #define _cc_init_event_poller _cc_init_event_kqueue
#else
    #define _cc_init_event_poller _cc_init_event_select
#endif

/**/
_CC_WIDGETS_API(bool_t) _cc_event_loop(int32_t threads, void (*func)(_cc_event_cycle_t*,bool_t));
/**/
_CC_WIDGETS_API(bool_t) _cc_event_loop_is_running(void);
/**/
_CC_WIDGETS_API(bool_t) _cc_quit_event_loop(void);
/**/
_CC_WIDGETS_API(void) _cc_event_loop_abort(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_EVENT_H_INCLUDED_*/
