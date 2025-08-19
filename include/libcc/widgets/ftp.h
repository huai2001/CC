/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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

#ifndef _C_CC_LIBFTP_H_INCLUDED_
#define _C_CC_LIBFTP_H_INCLUDED_

#include "dylib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
    
/* Access() type codes */
#define _CC_LIBFTP_DIR 1
#define _CC_LIBFTP_DIR_VERBOSE 2
#define _CC_LIBFTP_FILE_READ 3
#define _CC_LIBFTP_FILE_WRITE 4

/* Access() mode codes */
#define _CC_LIBFTP_TEXT 'A'
#define _CC_LIBFTP_BINARY 'I'

/* connection modes */
#define _CC_LIBFTP_PASSIVE 1
#define _CC_LIBFTP_PORT 2

enum {
    _CC_LIBFTP_RESP_PENDING = 0,
    _CC_LIBFTP_RESP_CONNECTED,
    _CC_LIBFTP_RESP_LOGIN_USER,
    _CC_LIBFTP_RESP_LOGIN_PASSWORD,
    _CC_LIBFTP_RESP_LOGOUT,
    _CC_LIBFTP_RESP_CWD,
    _CC_LIBFTP_RESP_CDUP,
    _CC_LIBFTP_RESP_MKDIR,
    _CC_LIBFTP_RESP_RENAME_FILE,
    _CC_LIBFTP_RESP_DEL_FILE,
    _CC_LIBFTP_RESP_DEL_FOLDER,
    _CC_LIBFTP_RESP_LIST,
    _CC_LIBFTP_RESP_OPTS_SYST,
    _CC_LIBFTP_RESP_OPTS_DATATYPE,
    _CC_LIBFTP_RESP_OPTS_UTF8,
    _CC_LIBFTP_RESP_OPTS_PWD,
    _CC_LIBFTP_RESP_OPTS_PASV,
    _CC_LIBFTP_RESP_OPTS_PORT
};

enum {
    _CC_LIBFTP_CONNECTED = 0,
    _CC_LIBFTP_CONNECT_FAILED,
    _CC_LIBFTP_LOGINED,
    _CC_LIBFTP_LOGIN_USER_FAILED,
    _CC_LIBFTP_LOGIN_PASSWORD_FAILED,
    _CC_LIBFTP_LOGOUT,
    _CC_LIBFTP_LOGOUT_FAILED,
    
    _CC_LIBFTP_RENAME,
    _CC_LIBFTP_CWD,
    _CC_LIBFTP_MKDIR,
    _CC_LIBFTP_LIST,
    _CC_LIBFTP_LIST_WAITING,
    _CC_LIBFTP_DEL_FILE,
    _CC_LIBFTP_DEL_FOLDER,
    _CC_LIBFTP_OPTS_UTF8,
    _CC_LIBFTP_OPTS_DATATYPE,
    _CC_LIBFTP_OPTS_PORT,
    _CC_LIBFTP_OPTS_PASV,
    
    _CC_LIBFTP_RENAME_FAILED,
    _CC_LIBFTP_CWD_FAILED,
    _CC_LIBFTP_MKDIR_FAILED,
    _CC_LIBFTP_LIST_FAILED,
    _CC_LIBFTP_DEL_FILE_FAILED,
    _CC_LIBFTP_DEL_FOLDER_FAILED,
    _CC_LIBFTP_OPTS_FAILED,
    _CC_LIBFTP_OPTS_PORT_FAILED,
};

typedef struct _cc_ftp _cc_ftp_t;
typedef struct _cc_ftp_resp _cc_ftp_resp_t;
/**/
typedef bool_t (*_cc_ftp_resp_callback_t)(_cc_ftp_t*, const byte_t *buf, uint32_t len);
typedef bool_t (*_cc_ftp_callback_t)(_cc_ftp_t*, uint16_t which);

/**/
struct _cc_ftp_resp {
    uint16_t flag;
    pvoid_t data;
    _cc_ftp_resp_callback_t callback;
};
    
/**/
struct _cc_ftp {
    bool_t logined;
        
    byte_t cmode;
    byte_t smode;
        
    struct {
        _cc_event_t *e;
        _cc_async_event_t *async;
    } ctrl;
        
    struct {
        _cc_event_t *e;
        _cc_async_event_t *async;
        struct {
            _cc_event_t *e;
            _cc_async_event_t *async;
        }accept;
    } data;
        
    struct sockaddr sa;
        
    _cc_ftp_resp_t resp;
    _cc_ftp_callback_t callback;
        
    char_t *user;
    char_t *password;
};

/**/
void libftp_set_error_info(const char_t *p, int32_t len);
/**/
void libftp_setup(_cc_ftp_t *ftp, uint16_t flag, _cc_ftp_resp_callback_t fn, pvoid_t data);
/**/
void libftp_del_attach(_cc_ftp_t *ftp);

/**/
_CC_WIDGETS_API(int32_t) _ftp_send_command(_cc_event_t*, const pvoid_t, int32_t);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_connected(_cc_ftp_t *ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_disconnected(_cc_ftp_t *ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_login(_cc_ftp_t *ftp, const char_t *user, const char_t *password);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_logout(_cc_ftp_t *ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_opts_utf8(_cc_ftp_t *ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_opts_datatype(_cc_ftp_t *ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_opts_port_passive(_cc_ftp_t *ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_open_port(_cc_ftp_t *ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_bind_accept(_cc_ftp_t* ftp, _cc_async_event_t *async, _cc_event_t *e);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_unbind_accept(_cc_ftp_t* ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_list(_cc_ftp_t *ftp, const char_t *path);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_mkdir(_cc_ftp_t *ftp, const char_t* path);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_cwd(_cc_ftp_t *ftp, const char_t* path);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_cdup(_cc_ftp_t *ftp);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_del_file(_cc_ftp_t *ftp, const char_t* file);
/**/
_CC_WIDGETS_API(bool_t) _cc_ftp_del_folder(_cc_ftp_t *ftp, const char_t* folder);
/**/
_CC_WIDGETS_API(const char_t*) _cc_ftp_get_error(void);
    
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_LIBFTP_H_INCLUDED_ */


