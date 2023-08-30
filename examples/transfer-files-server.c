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

static tchar_t save_directory[_CC_MAX_PATH_];

struct _transfer_files {
    _cc_file_t *fp;
    int16_t status;
    int64_t transfer_length;
    transfer_files_header_t header;
    tchar_t save_file[_CC_MAX_PATH_];
};

static _cc_socket_t _accept(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_socket_t fd;
    _cc_sockaddr_t remote_addr = {0};
    _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);

    fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
    if (fd == _CC_INVALID_SOCKET_) {
        _cc_logger_error(_T("accept fail %s.\n"), _cc_last_error(_cc_last_errno()));
        return _CC_INVALID_SOCKET_;
    }

    _cc_set_socket_nonblock(fd, 1);

    {
        struct sockaddr_in *remote_ip = (struct sockaddr_in *)&remote_addr;
        byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
        _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
    }

    return fd;
}

static void _free_transfer_files(transfer_files_t *transfer_files) {
    if (transfer_files->fp) {
        _cc_file_close(transfer_files->fp);
    }
    _cc_free(transfer_files);
}

static bool_t _send_status(_cc_event_t *e, uint16_t status) {
    byte_t masking[4];
    transfer_header_t transfer_status;
    transfer_status.version = transfer_files_version;
    transfer_status.status = status;

    _WritePacketHeader(e, BinData, sizeof(transfer_header_t), masking);
    _WritePacket(e, (byte_t*)&transfer_status, sizeof(transfer_header_t), masking);
    return true;
}

static bool_t _init(transfer_files_t *transfer_files) {
    transfer_files_header_t *header = (transfer_files_header_t*)&transfer_files->header;
    _sntprintf(transfer_files->save_file, _cc_countof(transfer_files->save_file), _T("%s/%lld/%s"), save_directory, header->user_id, header->path);
    _cc_mkdir(transfer_files->save_file);
    _cc_logger_debug(_T("upload %s"), header->path);

    transfer_files->transfer_length = 0;
    transfer_files->fp = _cc_open_file(transfer_files->save_file, _T("wb+"));
    if (transfer_files->fp) {
        return true;
    }
    return false;
}

static bool_t _check_file(_cc_event_t *e, transfer_files_t *transfer_files) {
    byte_t check[_CC_SHA1_DIGEST_LENGTH_];
    transfer_files_header_t *header = (transfer_files_header_t*)&transfer_files->header;
    /*
    */
    if (SHA1File(transfer_files->fp, check)) {
        _cc_file_close(transfer_files->fp);
        transfer_files->fp = NULL;
        transfer_files->status = STATUS_COMMAND;
        if (memcmp(check, header->check, _CC_SHA1_DIGEST_LENGTH_) == 0) {
            _send_status(e, _CC_STATUS_OK_);
            _cc_logger_debug(_T("%s sucess"), header->path);
            return true;
        }
    }
    _send_status(e, _CC_STATUS_BAD_REQUEST_);
    return false;
}

static bool_t _command(_cc_event_t *e, _cc_event_rbuf_t *r) {
    transfer_files_t *transfer_files = (transfer_files_t*)e->args;
    _Packet_t package;
    uint32_t rc;
    bzero(&package, sizeof(_Packet_t));
    do {
        uint64_t rc = _ReceivePacket(r->buf, r->length, &package);
        if (rc == 0) {
            return false;
        } else if (rc == 1) {
            break;
        } else if (rc == 2) {
            transfer_files_header_t *header = &transfer_files->header;
            if (package.oc != FileData) {
                _send_status(e, _CC_STATUS_NOT_ACCEPTABLE_);
                return false;
            }
            memcpy(header, &r->buf[package.offset], package.length);

            if (header->version != transfer_files_version) {
                _send_status(e, _CC_STATUS_VERSION_NOT_SUPPORTED_);
                return false;
            }

            if (!_init(transfer_files)) {
                _send_status(e, _CC_STATUS_INTERNAL_SERVER_ERROR_);
                return false;
            }
            package.offset += package.length;
            transfer_files->status = STATUS_TRANSFER_FILES;
            break;
        } else {
#if 0
            //If you want big data.
            //This is just a simple example, you can optimize it
            byte_t masking[4];
            uint64_t left = 0;
            //Save the received data size
            uint64_t payload = rc;
            byte_t *dest = _cc_malloc(sizeof(byte_t) * payload);
            memcpy(masking, package.mask, 4);
            memcpy(dest, &r->buf[package.offset], r->length - package.offset);
            //Continue receiving packet
            do {
                size_t r = _cc_recv(e->fd, dest + left, (int32_t)_min(payload - left, _CC_IO_BUFFER_SIZE_));
                if (r <= 0) {
                    //disconnected
                    return false;
                }
                left += r;
            } while (payload > left);
            //Get the complete packet
            _Masking(dest, payload, masking);
            //You can handle the packet
            _cc_free(dest);
            r->length = 0;
            return true;
#endif
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

    return true;
}

static bool_t transfer_files_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    if (events & _CC_EVENT_ACCEPT_) {
        _cc_socket_t fd = _accept(cycle, e);
        _cc_event_t *event;
        _cc_event_cycle_t *cycle_event;
        transfer_files_t *transfer_files;

        if (fd == _CC_INVALID_SOCKET_) {
            return true;
        }

        cycle_event = _cc_get_event_cycle();
        event = _cc_alloc_event(cycle_event, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_ | _CC_EVENT_BUFFER_);
        if (event == NULL) {
            _cc_close_socket(fd);
            return true;
        }

        transfer_files = (transfer_files_t*)_cc_malloc(sizeof(transfer_files_t));
        bzero(transfer_files, sizeof(transfer_files_t));
        transfer_files->status = STATUS_COMMAND;
        transfer_files->fp = NULL;

        event->fd = fd;
        event->callback = e->callback;
        event->timeout = e->timeout;
        event->args = transfer_files;

        if (cycle_event->driver.attach(cycle_event, event) == false) {
            _cc_free_event(cycle_event, event);
            _free_transfer_files(transfer_files);
        }

        _send_status(event, _CC_STATUS_ACCEPTED_);
        return true;
    }

    if (events & _CC_EVENT_DISCONNECT_) {
        _cc_logger_debug(_T("%d disconnect to client."), e->fd);
        return false;
    }

    if (events & _CC_EVENT_READABLE_) {
        transfer_files_t *transfer_files = (transfer_files_t*)e->args;
        _cc_event_rbuf_t *r;

        if (!_cc_event_recv(e)) {
            _cc_event_force_disconnect(e);
            _cc_logger_debug(_T("client close"));
            return false;
        }

        r = &e->buffer->r;
        if (transfer_files->status == STATUS_COMMAND) {
            if (!_command(e, r)) {
                return false;
            }
        }

        if (transfer_files->status == STATUS_TRANSFER_FILES && r->length > 0) {
            transfer_files_header_t *header = (transfer_files_header_t*)&transfer_files->header;
            size_t left, w;
            if (transfer_files->fp == NULL) {
                _send_status(e, _CC_STATUS_INTERNAL_SERVER_ERROR_);
                return false;
            }

            left = 0;
            while ((w = _cc_file_write(transfer_files->fp, r->buf + left, sizeof(byte_t), r->length - left)) > 0) {
                left += w;
                if (left == r->length) {
                    break;
                }
            }

            transfer_files->transfer_length += left;
            r->length -= left;

            if (transfer_files->transfer_length == header->file_length) {
                transfer_files->transfer_length = 0;
                transfer_files->status = STATUS_COMMAND;
                return _check_file(e, transfer_files);
            }
        }
    }

    if (events & _CC_EVENT_WRITABLE_) {
        _cc_event_wbuf_t *w = &e->buffer->w;
        if (w->r != w->w) {
            if (!_cc_event_sendbuf(e)) {
                return false;
            }
        }

        if (w->r == w->w) {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
            if (e->flags & _CC_EVENT_DISCONNECT_) {
                return false;
            }
        }

        return true;
    }

    if (events & _CC_EVENT_TIMEOUT_) {
        transfer_files_t *transfer_files = (transfer_files_t*)e->args;
        if (transfer_files->status == STATUS_TRANSFER_FILES) {
            _send_status(e, _CC_STATUS_GATEWAY_TIMEOUT_);
        }
        _cc_logger_debug(_T("TCP timeout %d\n"), e->fd);
        return false;
    }

    if (events & _CC_EVENT_DELETED_ && e->args != NULL) {
        _free_transfer_files((transfer_files_t*)e->args);
        e->args = NULL;
    }

    return true;
}

void quit(int sig) {
    //kill(0,SIGTERM);
    puts("\n");
    switch (sig) {
    case 1:
        printf("quit -- SIGHUP\n");
        return;
    case 2:
        printf("quit -- SIGINT\n");
        break;
    case 3:
        printf("quit -- SIGQUIT\n");
        break;
    case 15:
        printf("quit -- KILLALL\n");
        break;
    default:
        printf("quit error - %d\n", sig);
        break;
    }

    _cc_quit_event_loop();
    _cc_sleep(3000);
    exit(0);
}

int main(int argc, char *argv[]) {
    _cc_event_t *e;
    struct sockaddr_in sa;
    _cc_event_cycle_t *cycle;
    _cc_event_loop(0, NULL);
    _cc_get_module_directory(NULL, save_directory, _cc_countof(save_directory));

#ifndef __CC_WINDOWS__
    signal(SIGHUP, quit);
    signal(SIGINT, quit);
    signal(SIGQUIT, quit);
    signal(SIGTERM, quit);
#endif

    cycle = _cc_get_event_cycle();
    e = _cc_alloc_event(cycle, _CC_EVENT_ACCEPT_);
    if (e == NULL) {
        return -1;
    }

    e->callback = transfer_files_callback;
    e->timeout = 60000;

    _cc_inet_ipv4_addr(&sa, NULL, 6688);
    _cc_tcp_listen(cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in));
    _cc_logger_debug("listening: 6688");

    _task_init();

    _load_global_config(_T("Database.json"));
    _InitSQLConnector(6, DB_TB);

    if (!_load_system_config()) {
        return 0;
    }

    while (getchar() != 'q') {
        _cc_sleep(100);
    }

    quit(15);
    return 0;
}
