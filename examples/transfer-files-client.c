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
#include "transfer-files.h"

struct _transfer_files {
    _cc_file_t *fp;
    int16_t status;
    int32_t ident;
    int64_t transfer_length;
    //_transfer_files_fn_t callback;
    transfer_files_header_t header;

};

static void _free_transfer_files(transfer_files_t *transfer_files) {
    if (transfer_files->fp) {
        _cc_file_close(transfer_files->fp);
    }
    _cc_free(transfer_files);
}

static void _transfer_header(_cc_event_t *e) {
    transfer_files_t *transfer_files = (transfer_files_t*)e->args;
    byte_t masking[4];
    int32_t payload;

    transfer_files->status = STATUS_TRANSFER_FILES;
    transfer_files->transfer_length = _cc_file_size(transfer_files->fp);
    transfer_files->header.version = transfer_files_version;
    transfer_files->header.user_id = 0;
    transfer_files->header.file_length = transfer_files->transfer_length;
    SHA1File(transfer_files->fp, transfer_files->header.check);
    payload = sizeof(transfer_files_header_t) - (1024 * 4 * sizeof(tchar_t)) + _tcslen(transfer_files->header.path) + 1;

    _WritePacketHeader(e, FileData, payload, masking);
    _WritePacket(e, (byte_t*)&transfer_files->header, payload, masking);
}

static bool_t transfer_files_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    transfer_files_t *transfer_files = (transfer_files_t*)e->args;

    if (events & _CC_EVENT_CONNECT_) {
        return true;
    }
    
    if (events & _CC_EVENT_DISCONNECT_) {
        transfer_files->status = _CC_STATUS_NOT_FOUND_;
        return false;
    }

    /**/
    if (events & _CC_EVENT_READABLE_) {
        _cc_event_rbuf_t *r;
        _Packet_t package;
        uint64_t rc;
        bzero(&package, sizeof(_Packet_t));

        if (!_cc_event_recv(e)) {
            _cc_event_force_disconnect(e);
            _cc_logger_debug(_T("server close"));
            return false;
        }

        r = &e->buffer->r;

        do {
            rc = _ReceivePacket(r->buf, r->length, &package);
            if (rc == 0) {
                _cc_logger_debug(_T("server packet error"));
                return false;
            } else if (rc == 1) {
                break;
            } else if (rc == 2) {
                if (sizeof(transfer_header_t) == package.length) {
                    transfer_header_t *header = (transfer_header_t*)&r->buf[package.offset];
                    if (header->version != transfer_files_version) {
                        transfer_files->status = _CC_STATUS_VERSION_NOT_SUPPORTED_;
                        return false;
                    }

                    if (header->status == _CC_STATUS_ACCEPTED_) {
                        _transfer_header(e);
                    } else if (header->status == _CC_STATUS_OK_) {
                        transfer_files->status = STATUS_COMMAND;
                        _cc_logger_debug(_T("finish:%d\n"), transfer_files->status);
                    } else {
                        transfer_files->status = header->status;
                        _cc_logger_debug(_T("error:%d\n"), transfer_files->status);
                        return false;
                    }
                }
                package.offset += package.length;
            } else {
                //The packet is too big, so we just throw it out
                return false;
            }
        } while (package.offset < r->length);

        rc = (r->length - package.offset);
        if (rc > 0) {
            memmove(r->buf, r->buf + package.offset, rc);
            r->length = rc;
        } else {
            r->length = 0;
        }
    }

    if (events & _CC_EVENT_WRITABLE_) {
        _cc_event_wbuf_t *w = &e->buffer->w;

        if (w->r != w->w) {
            if (!_cc_event_sendbuf(e)) {
                transfer_files->status = _CC_STATUS_BAD_GATEWAY_;
                return false;
            }
        }

        if (w->r == w->w) {
            if (transfer_files->transfer_length > 0) {
                size_t r;
                _cc_spin_lock(&w->wlock);
                w->r = 0;
                w->w = 0;
                r = (size_t)_cc_file_read(transfer_files->fp, w->buf, sizeof(byte_t), w->length);
                if (r > 0) {
                    transfer_files->transfer_length -= r;
                    w->w += r;
                }
                _cc_spin_unlock(&w->wlock);

                _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
            } else {
                _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
            }
        }
    }
    
    if (events & _CC_EVENT_TIMEOUT_) {
        transfer_files->status = _CC_STATUS_GATEWAY_TIMEOUT_;
        return false;
    }

    if (events & _CC_EVENT_DELETED_ && e->args != NULL) {
        _free_transfer_files(transfer_files);
    }

    return true;
}

bool_t upload_files(transfer_files_t *transfer_files) {
    _cc_event_t *e;
    _cc_event_cycle_t *cycle = _cc_get_event_cycle();
    struct sockaddr_in sa;
    _cc_inet_ipv4_addr(&sa, _T("127.0.0.1"), 6688);
    e = _cc_alloc_event(cycle,  _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_|_CC_EVENT_BUFFER_);
    if (e == NULL) {
        return false;
    }
    
    e->callback = transfer_files_callback;
    e->timeout = 60000;
    e->args = transfer_files;

    if (!_cc_tcp_connect(cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(cycle, e);
        return false;
    }

    return true;
}

bool_t upload_files2(const tchar_t *file, const tchar_t *path, int32_t ident, int32_t create_time) {
    transfer_files_t *transfer_files;
    _cc_file_t *rfp = _cc_open_file(file, _T("rb"));
    if (rfp == NULL) {
        return false;
    }

    transfer_files = (transfer_files_t*)_cc_malloc(sizeof(transfer_files_t));
    bzero(transfer_files, sizeof(transfer_files_t));
    transfer_files->fp = rfp;
    transfer_files->header.create_time = create_time;

    _tcsncpy(transfer_files->header.path, path, _cc_countof(transfer_files->header.path));
    transfer_files->header.path[_cc_countof(transfer_files->header.path) - 1] = 0;

    return upload_files(transfer_files);
}

int main (int argc, char * const argv[]) {
    _cc_event_loop(1, NULL);
    upload_files2("./error.txt","/error1.txt",100, 1000);
    upload_files2("./error.txt","/error2.txt",100, 1000);
    upload_files2("./error.txt","/error3.txt",100, 1000);
    upload_files2("./error.txt","/error4.txt",100, 1000);
    upload_files2("./error.txt","/error5.txt",100, 1000);
    upload_files2("./error.txt","/error6.txt",100, 1000);
    upload_files2("./error.txt","/error7.txt",100, 1000);
    upload_files2("./error.txt","/error8.txt",100, 1000);
    upload_files2("./error.txt","/error9.txt",100, 1000);
    upload_files2("./error.txt","/error0.txt",100, 1000);

    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_quit_event_loop();
    
    return 0;
}