#ifndef _C_CC_EVENT_H_INCLUDED_
#define _C_CC_EVENT_H_INCLUDED_

#include "OpenSSL.h"
#include "array.h"
#include "atomic.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __CC_WINDOWS__
    #ifndef _CC_EVENT_DISABLE_IOCP_
        #define _CC_EVENT_USE_IOCP_           1
    #endif
#endif

#define _CC_EVENT_UNKNOWN_                      0x0000

#define _CC_EVENT_ACCEPT_                       0x0001
#define _CC_EVENT_WRITABLE_                     0x0002
#define _CC_EVENT_READABLE_                     0x0004
#define _CC_EVENT_CONNECT_                      0x0008

#define _CC_EVENT_CLOSED_                       0x0080

#define _CC_EVENT_PENDING_                      0x0100
#define _CC_EVENT_SOCKET_UDP_                   0x0200 /**< descriptor refers to a udp */
#define _CC_EVENT_SOCKET_IPV6_                  0x0400

#define _CC_EVENT_NONBLOCKING_                  0x1000
#define _CC_EVENT_CLOEXEC_                      0x2000

/** Used in _cc_event_t to determine what the fd is */
#define _CC_EVENT_SOCKET_                       0x010000 /**< descriptor refers to a socket */
#define _CC_EVENT_FILE_                         0x020000 /**< descriptor refers to a file */
#define _CC_EVENT_TIMEOUT_                      0x040000


#define _CC_TIMEOUT_MAX_LEVEL_                  4
#define _CC_TIMEOUT_NEAR_SHIFT_                 8
#define _CC_TIMEOUT_NEAR_                       (1 << _CC_TIMEOUT_NEAR_SHIFT_)
#define _CC_TIMEOUT_LEVEL_SHIFT_                6
#define _CC_TIMEOUT_LEVEL_                      (1 << _CC_TIMEOUT_LEVEL_SHIFT_)
#define _CC_TIMEOUT_NEAR_MASK_                  (_CC_TIMEOUT_NEAR_ - 1)
#define _CC_TIMEOUT_LEVEL_MASK_                 (_CC_TIMEOUT_LEVEL_ - 1)

typedef struct _cc_io_buffer _cc_io_buffer_t;
typedef struct _cc_event _cc_event_t;
typedef struct _cc_async_event_priv _cc_async_event_priv_t;
typedef struct _cc_async_event _cc_async_event_t;

typedef bool_t (*_cc_event_callback_t)(_cc_async_event_t*, _cc_event_t*, const uint32_t);

typedef struct _cc_io_data {
    int32_t limit;  // !< capacity of 'bytes'
    int32_t off;    // !< number of bytes in 'bytes'
    byte_t* bytes;  // !< pointer to internal memory
} _cc_io_data_t;

struct _cc_io_buffer {
    _cc_io_data_t r;
    _cc_io_data_t w;
    _cc_mutex_t *lock_of_writable;
    _cc_SSL_t *ssl;
};

struct _cc_event {
    /* One or more _CC_EVENT_* flags */
    uint32_t flags;
    /* The system has delivered the event flag */
    uint32_t filter;

    //0xFFF(async index)FFFFF(self index)
    /* read-only */
    uint32_t ident;
    _cc_socket_t fd;

    /* Linked list node */
    _cc_list_iterator_t lnk;

    /* A callback function for an event. */
    _cc_event_callback_t callback;

    /* A user-supplied argument. */
    uintptr_t data;

    /* The timer wheel */
    uint32_t timeout;
    uint32_t expire;

#ifdef _CC_EVENT_USE_IOCP_
    /* accepted socket file descriptor */
	_cc_socket_t accept_fd;
#endif
};

struct _cc_async_event {
    byte_t running;
    /**/
    uint16_t ident;
    /*Number of events processed*/
    int32_t processed;

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
    _cc_async_event_priv_t *priv;
    /* A user-supplied argument. */
    pvoid_t args;

    /**/
    bool_t (*reset)(_cc_async_event_t *async, _cc_event_t *e);
    /**/
    bool_t (*attach)(_cc_async_event_t *async, _cc_event_t *e);
    /**/
    bool_t (*connect)(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len);
    /**/
    _cc_socket_t (*accept)(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len);
    /**/
    bool_t (*disconnect)(_cc_async_event_t *async, _cc_event_t *e);
    /**/
    bool_t (*wait)(_cc_async_event_t *async, uint32_t timeout);
    /**/
    bool_t (*free)(_cc_async_event_t *async);
};

/* {{{ event */
/**/
_CC_API_PUBLIC(_cc_event_t*) _cc_alloc_event(_cc_async_event_t *async, const uint32_t flags);
/**/
_CC_API_PUBLIC(void) _cc_free_event(_cc_async_event_t *async, _cc_event_t *e);
/**/
_CC_API_PUBLIC(_cc_async_event_t *) _cc_get_async_event(void);
/**/
_CC_API_PUBLIC(_cc_event_t *) _cc_get_event_by_id(uint32_t ident);
/**/
_CC_API_PUBLIC(_cc_async_event_t *) _cc_get_async_event_by_id(uint32_t ident);
/* }}} */

/* {{{ io buffer */
/**/
_CC_API_PUBLIC(_cc_io_buffer_t *) _cc_alloc_io_buffer(int32_t limit);
/**/
_CC_API_PUBLIC(void) _cc_free_io_buffer(_cc_io_buffer_t *io);
/**/
_CC_API_PUBLIC(void) _cc_realloc_read_buffer(_cc_io_buffer_t *io,int32_t limit);
/**/
_CC_API_PUBLIC(void) _cc_realloc_write_buffer(_cc_io_buffer_t *io,int32_t limit);
/**/
_CC_API_PUBLIC(int32_t) _cc_io_buffer_flush(_cc_event_t *e, _cc_io_buffer_t *io);
/**/
_CC_API_PUBLIC(int32_t) _cc_io_buffer_send(_cc_event_t *e, _cc_io_buffer_t *io, const byte_t *bytes, int32_t length);
/**/
_CC_API_PUBLIC(int32_t) _cc_io_buffer_read(_cc_event_t *e, _cc_io_buffer_t *io);
/* }}} */


/* @{ */
/**/
_CC_API_PUBLIC(bool_t) _cc_tcp_listen(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen);
/**/
_CC_API_PUBLIC(bool_t) _cc_tcp_connect(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen);
/* }}} */

/**/
_CC_API_PUBLIC(bool_t) _cc_register_select(_cc_async_event_t*);

#ifdef __CC_WINDOWS__
    #ifdef _CC_EVENT_USE_IOCP_
    _CC_API_PUBLIC(bool_t) _cc_register_iocp(_cc_async_event_t*);
        #define _cc_register_poller _cc_register_iocp
    #else
        #define _cc_register_poller _cc_register_select
    #endif
#elif defined(__CC_LINUX__)
    _CC_API_PUBLIC(bool_t) _cc_register_poll(_cc_async_event_t*);
    _CC_API_PUBLIC(bool_t) _cc_register_epoll(_cc_async_event_t*);
    //_CC_API_PUBLIC(bool_t) _cc_register_io_uring(_cc_async_event_t*);
    #define _cc_register_poller _cc_register_epoll
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__) || \
    defined(__CC_FREEBSD__) || defined(__CC_OPENBSD__) ||   \
    defined(__CC_NETBSD__)
    _CC_API_PUBLIC(bool_t) _cc_register_poll(_cc_async_event_t*);
    _CC_API_PUBLIC(bool_t) _cc_register_kqueue(_cc_async_event_t*);
    #define _cc_register_poller _cc_register_kqueue
#else
    #define _cc_register_poller _cc_register_select
#endif

/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_async_event(int32_t cores, void (*cb)(_cc_async_event_t*, bool_t));
/**/
_CC_API_PUBLIC(bool_t) _cc_async_event_is_running(void);
/**/
_CC_API_PUBLIC(bool_t) _cc_free_async_event(void);
/**/
_CC_API_PUBLIC(void) _cc_async_event_abort(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_EVENT_H_INCLUDED_*/
