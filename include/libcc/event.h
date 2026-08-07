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

/*
 * Event flag definitions
 * These flags are used to describe event interests and state. They are
 * combined in the `flags` field of `_cc_event_t` to indicate what the
 * caller is interested in (read/write/accept/connect) and the type of the
 * registration (socket/file/timeout). The poller sets `filter` to report
 * which events have actually occurred.
 */

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

/*
 * Simple byte buffer descriptor
 * - `limit` is the capacity of the buffer
 * - `off` is the number of bytes currently stored in `bytes`
 * - `bytes` points to the raw memory region
 */
typedef struct _cc_io_data {
    int32_t limit;  // !< capacity of 'bytes'
    int32_t off;    // !< number of bytes in 'bytes'
    byte_t* bytes;  // !< pointer to internal memory
} _cc_io_data_t;

/*
 * I/O buffer
 * Wraps a read and write buffer with an optional mutex used to serialize
 * concurrent writes and an SSL object when using TLS. The API offers
 * allocation, reallocation and simple read/flush helpers implemented in
 * `src/event/buffer.c`.
 */
struct _cc_io_buffer {
    _cc_io_data_t r;
    _cc_io_data_t w;
    _cc_mutex_t *lock_of_writable;
    _cc_SSL_t *ssl;
};

/*
 * _cc_event
 * Represents a single registration with the poller. Important fields:
 * - `flags`: requested interests and type
 * - `filter`: events reported by the kernel/poller
 * - `ident`: a composite identifier encoding async index and slot index
 * - `fd`: the underlying file descriptor (socket/file)
 * - `callback`: invoked when the event is triggered
 * - `timeout`/`expire`: used by the timer wheel when this event has a timeout
 */
struct _cc_event {
    /* One or more _CC_EVENT_* flags */
    uint32_t flags;
    /* The system has delivered the event flag */
    uint32_t filter;

    _cc_socket_t fd;
#ifdef _CC_EVENT_USE_IOCP_
    /* accepted socket file descriptor */
    _cc_socket_t accept_fd;
#endif

    /* The timer wheel */
    uint32_t timeout;
    uint32_t expire;

    /*
     * 64-bit local handle.
     * High 32 bits: generation/version.
     * Low 32 bits : 0xFFF(async index)FFFFF(self index).
     */
    uint64_t ident;

    /* A callback function for an event. */
    _cc_event_callback_t callback;

    /* A user-supplied argument. */
    uintptr_t data;

    /* Linked list node */
    _cc_list_t lnk;
};

/*
 * _cc_async_event
 * Represents an asynchronous event loop / poller instance. The structure
 * stores the timer wheel (`nears`, `level`) used for efficient timeout
 * scheduling, a list of pending changes (`changes`) that will be applied
 * by the poller, and backend-specific `priv` state. The function pointers
 * implement a polymorphic interface allowing different poller backends
 * to be plugged in (select, epoll, kqueue, iocp, ...).
 */
struct _cc_async_event {
    byte_t running;
    /**/
    uint16_t ident;
    /*Number of events processed*/
    int32_t processed;
    int32_t actives;

    /*timers wheel*/
    uint32_t timer;
    uint32_t diff;
    uint64_t tick;
    /*
        Timer Wheel
        nears[256]        // 0-256ms (2^8) 
        level[0][64]      // 256-16s (2^14)
        level[1][64]      // 16s-17min (2^20)
        level[2][64]      // 17min-18h (2^26) 
        level[3][64]      // 18h-47d (2^32)
    */
    _cc_list_t nears[_CC_TIMEOUT_NEAR_];
    _cc_list_t level[_CC_TIMEOUT_MAX_LEVEL_][_CC_TIMEOUT_LEVEL_];
    _cc_list_t pending;
    _cc_list_t no_timer;

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
/**
 * Allocate a new event slot tied to the given async instance.
 * `flags` indicates the event type/interests (combination of _CC_EVENT_*).
 * Returns a pointer to a fresh `_cc_event_t` or NULL on failure.
 */
_CC_API_PUBLIC(_cc_event_t*) _cc_alloc_event(_cc_async_event_t *async, const uint32_t flags);

/**
 * Free an event previously obtained via `_cc_alloc_event`.
 * This closes any associated fd and returns the slot to the global pool.
 */
_CC_API_PUBLIC(void) _cc_free_event(_cc_async_event_t *async, _cc_event_t *e);

/**
 * Select and return an active async event instance.
 * Typically used by internal helpers to get the current poller instance.
 */
_CC_API_PUBLIC(_cc_async_event_t *) _cc_get_async_event(void);

/**
 * Lookup an event by its composite 64-bit identifier (`ident`). Returns NULL if
 * the id is invalid or the slot has been recycled.
 */
_CC_API_PUBLIC(_cc_event_t *) _cc_get_event_by_id(uint64_t ident);

/**
 * Return the async event instance associated with `ident` (high bits).
 * Returns NULL if the async id is out of range or unregistered.
 */
_CC_API_PUBLIC(_cc_async_event_t *) _cc_get_async_event_by_id(uint64_t ident);

/* }}} */

/* {{{ io buffer */
/**
 * Allocate an I/O buffer pair (read/write) with an initial capacity of
 * `limit` bytes. Returns the allocated `_cc_io_buffer_t*` or NULL.
 * The write buffer is protected by an internal mutex for concurrent use.
 */
_CC_API_PUBLIC(_cc_io_buffer_t *) _cc_alloc_io_buffer(int32_t limit, _cc_SSL_t *ssl);
/**
 * Free an `_cc_io_buffer_t` and its internal memory. Caller must ensure
 * no concurrent users remain.
 */
_CC_API_PUBLIC(void) _cc_free_io_buffer(_cc_io_buffer_t *io);

/**
 * Resize the read buffer to `limit` bytes. If `off` exceeds `limit`, the
 * read offset is reset to 0.
 */
_CC_API_PUBLIC(void) _cc_realloc_read_buffer(_cc_io_buffer_t *io,int32_t limit);

/**
 * Resize the write buffer to `limit` bytes. If `off` exceeds `limit`, the
 * write offset is reset to 0. Caller must hold no locks; function handles
 * internal memory reallocation.
 */
_CC_API_PUBLIC(void) _cc_realloc_write_buffer(_cc_io_buffer_t *io,int32_t limit);

/**
 * Attempt to flush pending write bytes to the event's `fd`.
 * Returns number of bytes sent (>0) on success, 0 if nothing sent,
 * or -1 on fatal error (errno set). For EAGAIN/EINTR the function will
 * preserve the buffer and return 0.
 */
_CC_API_PUBLIC(int32_t) _cc_io_buffer_flush(_cc_event_t *e, _cc_io_buffer_t *io);

/**
 * Queue data for sending. If the write queue is empty this tries a direct
 * send first. Returns number of bytes directly written (>=0) or -1 on
 * error (errno set). The remainder is appended to the write queue.
 * This function serializes writers using the internal mutex.
 */
_CC_API_PUBLIC(int32_t) _cc_io_buffer_send(_cc_event_t *e, _cc_io_buffer_t *io, const byte_t *bytes, int32_t length);

/**
 * Read from `e->fd` into the read buffer. Returns number of bytes read (>0),
 * 0 for EAGAIN/EINTR, -1 for EOF, or -1 on fatal error (errno set).
 */
_CC_API_PUBLIC(int32_t) _cc_io_buffer_read(_cc_event_t *e, _cc_io_buffer_t *io);

/* }}} */


/* @{ */
/**/
_CC_API_PUBLIC(bool_t) _cc_tcp_listen(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen);
/**
 * Create a non-blocking listen socket, bind to `sockaddr` and register the
 * listening event with the provided async instance. Returns true on
 * success and false on failure.
 */
/**/
_CC_API_PUBLIC(bool_t) _cc_tcp_connect(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen);
/**
 * Create a non-blocking socket and initiate a connect to `sockaddr`.
 * The event is attached to `async` to complete the connection in the
 * background. Returns true if the connect was started successfully.
 */
/* }}} */

/**/
_CC_API_PUBLIC(bool_t) _cc_register_select(_cc_async_event_t*);

/**
 * Set the wait timeout (in milliseconds) used by internal slot expansion
 * logic when waiting for idle slots. Minimum is 1 ms.
 */
_CC_API_PUBLIC(void) _cc_event_set_slot_wait(uint32_t ms);

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
