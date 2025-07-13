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
#ifndef _C_CC_WINDOWS_IOCP_H_INCLUDED_
#define _C_CC_WINDOWS_IOCP_H_INCLUDED_

#include <libcc/core/windows.h>
#include "../event.c.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CC_EVENT_USE_IOCP_

#define _CC_IOCP_SOCKET_                  0xFFFFFFF0
#define _CC_IOCP_PENDING_                 0xFFFFFFF1
#define _CC_IOCP_EXIT_                    0xFFFFFFFF

struct _cc_event_cycle_priv {
	HANDLE port;
    _cc_list_iterator_t overlapped_active;
    _cc_list_iterator_t overlapped_idle;
    int32_t idle_count;
    _cc_atomic_lock_t lock;
};

typedef struct _iocp_overlapped {
    uint16_t round;
	uint16_t flag;
	_cc_socket_t fd;
    OVERLAPPED overlapped;
    _cc_list_iterator_t lnk;
    _cc_event_t *e;
}_iocp_overlapped_t;


/**
 * @brief IOCP Socket TCP Accept
 *
 * @param e event structure
 * @param fd Socket handle
 * @param overlapped IOCP OVERLAPPED structure
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_accept(_cc_event_t* e, _cc_socket_t fd, LPOVERLAPPED overlapped);
/**
 * @brief IOCP Socket TCP Send
 *
 * @param e event structure
 * @param overlapped IOCP OVERLAPPED structure
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_send(_cc_event_t *e, LPOVERLAPPED overlapped);
/**
 * @brief IOCP Socket TCP Read
 *
 * @param e event structure
 * @param overlapped IOCP OVERLAPPED structure
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_receive(_cc_event_t *e, LPOVERLAPPED overlapped);
/**
 * @brief IOCP Socket UDP Send
 *
 * @param e event structure
 * @param overlapped IOCP OVERLAPPED structure
 * @param sa _cc_sockaddr_t structure
 * @param sa_len Length of send byte buffer
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_sendto(_cc_event_t *e, LPOVERLAPPED overlapped, _cc_sockaddr_t *sa, _cc_socklen_t sa_len);
/**
 * @brief IOCP Socket UDP Read
 *
 * @param e event structure
 * @param overlapped IOCP OVERLAPPED structure
 * @param sa _cc_sockaddr_t structure
 * @param sa_len Length of receive byte buffer
 *
 * @return 0 if successful or socket on error.
 */
int _WSA_socket_receivefrom(_cc_event_t *fd, LPOVERLAPPED overlapped, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len);

/**
 * @brief IOCP Initialize Overlapped 
 *
 * @param priv _cc_event_cycle_priv_t
 */
void _cc_iocp_overlapped_init(_cc_event_cycle_priv_t* priv);

/**
 * @brief IOCP Uninitialize Overlapped
 * 
 * @param priv _cc_event_cycle_priv_t
 */
void _cc_iocp_overlapped_quit(_cc_event_cycle_priv_t *priv);

/**
 * @brief IOCP Create Overlapped
 *
 * @param priv _cc_event_cycle_priv_t
 *
 * @return 
 */
_iocp_overlapped_t* _cc_iocp_overlapped_alloc(_cc_event_cycle_priv_t *priv);
/**
 * @brief IOCP Free Overlapped
 *
 * @param priv _cc_event_cycle_priv_t
 * @param iocp_overlapped _iocp_overlapped_t
 *
 */
void _cc_iocp_overlapped_free(_cc_event_cycle_priv_t *priv,_iocp_overlapped_t *iocp_overlapped);

#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_WINDOWS_IOCP_H_INCLUDED_ */
