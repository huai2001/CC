/*
 * Copyright (c) 2006 - 2018 QIU ZHONG HUAI <huai2011@163.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes GHO software, freely available from
 *    <https://github.com/huai2001/CC>".
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS`` AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "Header.h"

tchar_t remoteUpdatedUrl[_REMOTE_UPDATED_URL_];
tchar_t remoteUpdatedVersion[32];
tchar_t modulePath[_CC_MAX_PATH_];
_cc_SSL_CTX_t *SSL_handle = NULL;

_CC_FORCE_INLINE_ bool_t readJSON(tchar_t *szUpdatePath) {
    char_t path[_CC_MAX_PATH_];
    _cc_json_t *rootJSON = NULL;

    _cc_get_module_directory(_T("config.json"), path, _CC_MAX_PATH_);

    rootJSON = _cc_open_json_file(path);
    if (rootJSON) {
        _tcsncpy(szUpdatePath,_cc_json_object_find_string(rootJSON, _T("path")), _CC_MAX_PATH_);
        szUpdatePath[_CC_MAX_PATH_ - 1] = 0;
        _cc_destroy_json(&rootJSON);
        return true;
    }
    return false;
}

int startSyncFils(int argc, const tchar_t* argv[]) {
    _cc_event_loop(0, NULL);
    SSL_handle = _SSL_init(true);
    //
    if (argc > 1 && argv[1]) {
        _tcsncpy(modulePath, argv[1], _cc_countof(modulePath));
        modulePath[_cc_countof(modulePath) - 1] = 0;
    } else if (!readJSON(modulePath)) {
        _cc_get_module_directory("wepoker", modulePath, _CC_MAX_PATH_);
    }

    _mk(modulePath);
    downloadVersion();

    while (_cc_event_loop_is_running()) {
        _cc_sleep(100);
    }

    _cc_quit_event_loop();
    return 0;
}
//////////////////////////////////////////////////////////////////////////////////
