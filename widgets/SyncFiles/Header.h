/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#ifndef _C_SYNCFILES_HEADER_H_INCLUDED_
#define _C_SYNCFILES_HEADER_H_INCLUDED_

#include <libcc.h>
#include <cc/widgets/widgets.h>
#include <cc/json/json.h>


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _REMOTE_UPDATED_URL_ 128
extern tchar_t modulePath[_CC_MAX_PATH_];
extern tchar_t remoteUpdatedUrl[_REMOTE_UPDATED_URL_];
extern tchar_t remoteUpdatedVersion[32];
extern _cc_SSL_CTX_t *SSL_handle;

typedef struct tagRequestFile {
    _cc_file_t *wfp;
    uint64_t fileSize;
    uint64_t rangeBytes;
    uint64_t index;
    uint32_t lengthPath;
    int xxtea;
    tchar_t url[1024];
    tchar_t path[_CC_MAX_PATH_];
    void (*completed)(struct tagRequestFile*, bool_t success);
    _cc_list_iterator_t lnk;
} tagRequestFile;

int32_t _mk(tchar_t* path);
uint64_t fileCheck(const tchar_t* fileName, tchar_t* output);

void downloadSync(void);
void downloadVersion(void);
void downloadAssets(const _cc_rbtree_t* object, const tchar_t* packageUrl);
void downloadProject(const tchar_t *remoteUrl, tagRequestFile *rf);
int startSyncFils (int argc, const tchar_t * argv[]);
bool_t url_request(const tchar_t *url, pvoid_t args);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif


#endif /* _C_SYNCFILES_HEADER_H_INCLUDED_ */
