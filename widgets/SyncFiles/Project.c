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

_CC_FORCE_INLINE_ void downloadProjectCompleted(tagRequestFile *rf, bool_t success) {
    _cc_buf_t *buf;
    _cc_json_t *manifest;
    const tchar_t *packageUrl;
    const _cc_rbtree_t *assets;
    FILE *wfp;
    tchar_t url[128];

    if (!success) {
        _cc_logger_debug("project.manifest, download fial.");
        _cc_free(rf);
        _cc_event_loop_abort();
        return;
    }

    manifest = _cc_open_json_file(rf->path);
    if (manifest == NULL) {
        _cc_logger_error("%s, json parse fial", rf->path);
        _cc_free(rf);
        _cc_event_loop_abort();
        return;
    }

    packageUrl = _cc_json_object_find_string(manifest, "packageUrl");
    assets = _cc_json_object_find_object(manifest, "assets");
    if (packageUrl && assets) {
        downloadAssets(assets, packageUrl);
    }

    _cc_json_add_string(manifest, "packageUrl", remoteUpdatedUrl, true);
    _sntprintf(url, _cc_countof(url), _T("%sproject.manifest"), remoteUpdatedUrl);
    _cc_json_add_string(manifest, "remoteManifestUrl", url, true);
    _sntprintf(url, _cc_countof(url), _T("%sversion.manifest"), remoteUpdatedUrl);
    _cc_json_add_string(manifest, "remoteVersionUrl", url, true);

    buf = _cc_print_json(manifest);
    wfp = _tfopen(rf->path, _T("wb"));
    if (wfp) {
        fwrite(_cc_buf_bytes(buf), sizeof(byte_t), buf->length, wfp);
        fclose(wfp);
    }
    _cc_destroy_buf(&buf);

    _cc_destroy_json(&manifest);
    _cc_free(rf);

    downloadSync();
}

void downloadProject(const tchar_t *remoteUrl, tagRequestFile *rf) {
    rf->completed = downloadProjectCompleted;
    url_request(remoteUrl, rf);
}