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
void _xxtea_encrypt_file(_cc_buf_t *rbuf, const tchar_t *save_path);
_cc_buf_t *_xxtea_decrypt_file(const tchar_t *source_path);

static _cc_atomic32_t refcount = 0;
static _cc_list_iterator_t downloading;
static time_t timeOfExpiration = 0;

int32_t _mk(tchar_t *path) {
    tchar_t *cp;
    int32_t length = _cc_realpath(path);
    //
    cp = path;
    /* Skip the first / */

    if (*cp == _CC_T_PATH_SEP_C_) {
        cp++;
    }

    /**/
    while (*cp) {
        if (*cp == _CC_T_PATH_SEP_C_) {
            *cp = 0;
            if (_taccess(path, 0) != 0) {
                _tmkdir(path);
            }
            *cp = _CC_T_PATH_SEP_C_;
        }
        cp++;
    }
    return length;
}

#define CHUNK 16384

uint64_t fileCheck(const tchar_t *fileName, tchar_t *output) {
    byte_t t = 0;
    const tchar_t hex[] = _T("0123456789abcdef");
    byte_t md[_CC_MD5_DIGEST_LENGTH_];
    byte_t buf[CHUNK];
    int32_t i;
    _cc_md5_t c;
    FILE *fp = _tfopen(fileName, _T("rb"));
    uint64_t fileSize;

    if (fp == NULL)
        return 0;

    _cc_md5_init(&c);

    fseek(fp, 0, SEEK_SET);

    fileSize = 0;
    while ((i = (int32_t)fread(buf, sizeof(byte_t), _cc_countof(buf), fp))) {
        _cc_md5_update(&c, buf, (unsigned long)i);
        fileSize += i;
    }
    _cc_md5_final(&c, &(md[0]));

    fclose(fp);

    if (output) {
        int index = 0;
        for (i = 0; i < _CC_MD5_DIGEST_LENGTH_; i++) {
            t = md[i];
            output[index++] = hex[t / 16];
            output[index++] = hex[t % 16];
        }
        output[index] = _T('\0');
    }
    return fileSize;
}

_CC_FORCE_INLINE_ void InjectFile(tagRequestFile *rf) {
    _cc_buf_t *buf;
    _cc_buf_t *injectFile;
    _cc_buf_t modify;
    tchar_t *packageUrl; //, *clickEmailLogin, *officialUrl;
    tchar_t path[_CC_MAX_PATH_];
    size_t pos = 0;

    // 解压注入
    _sntprintf(path, _cc_countof(path), _T("%s/inject.js"), modulePath);
    injectFile = _cc_load_buf(path);
    if (injectFile == NULL) {
        return;
    }

    buf = _xxtea_decrypt_file(rf->path);
    if (buf == NULL) {
        return;
    }
    _cc_buf_alloc(&modify, buf->length + 2480 + injectFile->length);
    packageUrl = _tcsstr((tchar_t *)buf->bytes, "updateManifestUrls: function(");
    if (packageUrl) {
        //_cc_strA_t clickEmailLoginFunc = _cc_string("clickEmailLogin: function() {");
        _cc_strA_t updateManifestUrls = _cc_string(
            "updateManifestUrls: function(e, t, n){return this.old_updateManifestUrls(e,dyad.packageUrl,n);},old_");
        pos = (size_t)(packageUrl - (tchar_t *)buf->bytes);
        _cc_buf_write(&modify, buf->bytes, pos);
        _cc_buf_write(&modify, updateManifestUrls.data, updateManifestUrls.length);

        /*clickEmailLogin = strstr(packageUrl,clickEmailLoginFunc.data);
        if (clickEmailLogin) {
            clickEmailLogin += clickEmailLoginFunc.length;
            pos = (clickEmailLogin - (tchar_t*)buf->bytes);
            _cc_buf_write(&modify, packageUrl, (size_t)(clickEmailLogin - packageUrl));
            _cc_buf_write(&modify, "\ndyad.setPhoneNum(this);", sizeof("\ndyad.setPhoneNum(this);") - 1);
        }
        officialUrl = strstr(clickEmailLogin,"getOfficialUrl: function() {");
        if (officialUrl) {

        }*/
        _cc_buf_write(&modify, buf->bytes + pos, buf->length - pos);
        /*
        {
            tchar_t path[_CC_MAX_PATH_];
            FILE* wf;
            _tcsncpy(path, rf->path, rf->lengthPath - 2);
            path[rf->lengthPath - 2] = 0;
            wf = _tfopen(path, _T("wb"));
            if (wf) {
                fwrite(modify.bytes, sizeof(byte_t), modify.length, wf);
                fclose(wf);
            }
        }*/
    }

    rf->path[rf->lengthPath - 2] = 'c';
    rf->path[rf->lengthPath] = 0;

    _cc_buf_write(&modify, injectFile->bytes, injectFile->length);
    _xxtea_encrypt_file(&modify, rf->path);
    _cc_buf_free(&modify);
    _cc_destroy_buf(&buf);
    _cc_destroy_buf(&injectFile);
}

_CC_FORCE_INLINE_ void downloadCompleted(tagRequestFile *rf, bool_t success) {
    if (!success) {
        _cc_logger_debug("%d %s, download fial.", rf->index, rf->url);
    }

    if (rf->xxtea == 1) {
        InjectFile(rf);
    }

    _cc_atomic32_dec(&refcount);
    _cc_free(rf);
}

void _traversalAssets(const _cc_rbtree_iterator_t *object, const tchar_t *packageUrl) {
    _cc_json_t *item;
    const tchar_t *md5Value;
    tagRequestFile *rf;
    uint64_t fileSize;
    tchar_t requestMD5[33];

    if (object->left) {
        _traversalAssets(object->left, packageUrl);
    }

    if (object->right) {
        _traversalAssets(object->right, packageUrl);
    }

    rf = (tagRequestFile *)_cc_malloc(sizeof(tagRequestFile));
    bzero(rf, sizeof(tagRequestFile));

    item = _cc_upcast(object, _cc_json_t, node);
    rf->xxtea = 0;
    rf->completed = downloadCompleted;

    rf->fileSize = _cc_json_object_find_number(item, "size");
    md5Value = _cc_json_object_find_string(item, "md5");
    _sntprintf(rf->path, _cc_countof(rf->path), _T("%s/%s"), modulePath, item->name);
    rf->lengthPath = _mk(rf->path);

    // 需要注入的文件
    if (_tcsicmp(item->name, "assets/main/index.jsc") == 0) {
        rf->path[rf->lengthPath - 2] = 'x';
        rf->xxtea = 1;
    }
    fileSize = fileCheck(rf->path, requestMD5);

    if (fileSize == 0 || _tcsnicmp(md5Value, requestMD5, 33) != 0) {
        _sntprintf(rf->url, _cc_countof(rf->url), _T("%s%s"), packageUrl, item->name);
        _cc_list_iterator_push(&downloading, &rf->lnk);
        return;
    }

    _cc_free(rf);
}

_CC_FORCE_INLINE_ bool_t _timer(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {

    int32_t count = _cc_atomic32_get(&refcount);
    time_t now = time(NULL);
    if (!_cc_list_iterator_empty(&downloading)) {
        int32_t i;
        tagRequestFile *rf;
        _cc_list_iterator_t *lnk;
        int32_t c = 10 - count;
        for (i = 0; i < c; i++) {
            lnk = _cc_list_iterator_pop(&downloading);
            if (lnk == NULL) {
                break;
            }
            rf = _cc_upcast(lnk, tagRequestFile, lnk);
            _mk(rf->path);
            _cc_atomic32_inc(&refcount);
            url_request(rf->url, rf);
        }
        timeOfExpiration = now + 5 * 60 * 1000;
        return true;
    }

    if (count <= 0) {
        _cc_logger_debug("Updated.");
        _cc_event_loop_abort();
        return false;
    }

    if (now > timeOfExpiration) {
        _cc_logger_debug("Download Timeout.");
        _cc_event_loop_abort();
        return false;
    }

    return true;
}

void downloadAssets(const _cc_rbtree_t *object, const tchar_t *packageUrl) {
    _cc_list_iterator_cleanup(&downloading);
    _traversalAssets(object->rb_node, packageUrl);
}
/**
 *
 */
void downloadSync(void) {
    if (!_cc_list_iterator_empty(&downloading)) {
        timeOfExpiration = time(NULL) + 5 * 60 * 1000;
        _cc_add_event_timeout(_cc_get_event_cycle(), 1000, _timer, NULL);
    } else {
        _cc_logger_debug("Updated.");
        _cc_event_loop_abort();
    }
}