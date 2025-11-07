#ifndef _C_CC_WINDOWS_IOCP_H_INCLUDED_
#define _C_CC_WINDOWS_IOCP_H_INCLUDED_

#include <libcc/socket.h>
#include <libcc/os/windows.h>
#include "../../event/event.c.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CC_EVENT_USE_IOCP_

#define _CC_IOCP_SOCKET_                  0xFFFFFFF0
#define _CC_IOCP_PENDING_                 0xFFFFFFF1
#define _CC_IOCP_SSL_HANDSHAKING_         0xFFFFFFF2
#define _CC_IOCP_EXIT_                    0xFFFFFFFF

struct _cc_async_event_priv {
	HANDLE port;
    _cc_list_iterator_t io_active;
    _cc_list_iterator_t io_idle;
    int32_t frees;
};

typedef struct _io_context {
    uint32_t ident;
	uint32_t flag;
	_cc_socket_t fd;
    OVERLAPPED overlapped;
	DWORD number_of_bytes;
    _cc_list_iterator_t lnk;
    _cc_event_t *e;
} _io_context_t;

/**
 * @brief IOCP Socket TCP Accept
 *
 * @param io_context IOCP OVERLAPPED structure
 * @param fd Socket handle
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_accept(_io_context_t* io_context);
/**
 * @brief IOCP Socket TCP Send
 *
 * @param e event structure
 * @param io_context IOCP OVERLAPPED structure
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_send(_io_context_t *io_context);
/**
 * @brief IOCP Socket TCP Read
 *
 * @param io_context IOCP OVERLAPPED structure
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_receive(_io_context_t *io_context);
/**
 * @brief IOCP Socket UDP Send
 *
 * @param io_context IOCP OVERLAPPED structure
 * @param sa _cc_sockaddr_t structure
 * @param sa_len Length of send byte buffer
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_sendto(_io_context_t *io_context, _cc_sockaddr_t *sa, _cc_socklen_t sa_len);
/**
 * @brief IOCP Socket UDP Read
 *
 * @param e event structure
 * @param io_context IOCP OVERLAPPED structure
 * @param sa _cc_sockaddr_t structure
 * @param sa_len Length of receive byte buffer
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_receivefrom(_io_context_t *io_context, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len);

/**
 * @brief IOCP Initialize Overlapped 
 *
 * @param priv _cc_async_event_priv_t
 */
void _io_context_init(_cc_async_event_priv_t* priv);

/**
 * @brief IOCP Uninitialize Overlapped
 * 
 * @param priv _cc_async_event_priv_t
 */
void _io_context_quit(_cc_async_event_priv_t *priv);

/**
 * @brief IOCP Create Overlapped
 *
 * @param priv _cc_async_event_priv_t
 *
 * @return 
 */
_io_context_t* _io_context_alloc(_cc_async_event_priv_t *priv, _cc_event_t *e);
/**
 * @brief IOCP Free Overlapped
 *
 * @param priv _cc_async_event_priv_t
 * @param io_context _io_context_t
 *
 */
void _io_context_free(_cc_async_event_priv_t *priv, _io_context_t *io_context);

#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_WINDOWS_IOCP_H_INCLUDED_ */
