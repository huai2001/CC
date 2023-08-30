#include <stdio.h>
#include <libcc.h>
#include <cc/socket/socket.h>

#define REDIS_REPLY_STRING      1 //'$'
#define REDIS_REPLY_ARRAY       2 //'*'
#define REDIS_REPLY_INTEGER     3 //':'
#define REDIS_REPLY_NIL         4 //'_'
#define REDIS_REPLY_STATUS      5 //'+'
#define REDIS_REPLY_ERROR       6 //'-'
#define REDIS_REPLY_DOUBLE      7 //','
#define REDIS_REPLY_BOOL        8 //'#'
#define REDIS_REPLY_MAP         9 //'%'
#define REDIS_REPLY_SET         10 //'~'
#define REDIS_REPLY_ATTR        11
#define REDIS_REPLY_PUSH        12 //'>'
#define REDIS_REPLY_BIGNUM      13
#define REDIS_REPLY_VERB        14 //'='

char buf[1024 * 5];
bool_t keepActive = true;

_cc_socket_t sock;

int getInputLine(char *result, int maxlen) {
    return (int)read(fileno(stdin),result,maxlen);
}

int32_t redisCommanReply(_cc_thread_t *thrd, void* param) {
    fd_set rfds, efds;

    printf("thread running!\n");

    while (keepActive) {
        long res = 0;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        
        FD_ZERO(&efds);
        FD_SET(sock, &efds);

        res = select(sock + 1,&rfds,NULL,&efds,NULL);
        if (res < 0) {
            printf("fail to select : %s\n", _cc_last_error(_cc_last_errno()));
            break;
        }
        
        if (FD_ISSET(sock, &rfds)) {
            res = (long)recv(sock, buf, sizeof(buf) - 1, 0);
            if (res < 0) {
                printf("fail to recv : %s\n", _cc_last_error(_cc_last_errno()));
                break;
            } else if (res == 0) {
                printf("close socket!\n");
                break;
            }
            
            buf[res] = 0;

            int type = 0;
            switch (buf[0]) {
                case '-':
                    type = REDIS_REPLY_ERROR;
                    break;
                case '+':
                    type = REDIS_REPLY_STATUS;
                    break;
                case ':':
                    type = REDIS_REPLY_INTEGER;
                    break;
                case ',':
                    type = REDIS_REPLY_DOUBLE;
                    break;
                case '_':
                    type = REDIS_REPLY_NIL;
                    break;
                case '$':
                    type = REDIS_REPLY_STRING;
                    break;
                case '*':
                    type = REDIS_REPLY_ARRAY;
                    break;
                case '%':
                    type = REDIS_REPLY_MAP;
                    break;
                case '~':
                    type = REDIS_REPLY_SET;
                    break;
                case '#':
                    type = REDIS_REPLY_BOOL;
                    break;
                case '=':
                    type = REDIS_REPLY_VERB;
                    break;
                case '>':
                    type = REDIS_REPLY_PUSH;
                    break;
                default:
                    break;
            }
            printf("get %ld bytes of data: %stype:%d\n",res,buf, type);
            bzero(&buf, sizeof(buf));
        } else if (FD_ISSET(sock, &efds)) {
            res = (long)recv(sock, buf, sizeof(buf) - 1, MSG_OOB);
            if (res < 0) {
                printf("exception to recv : %s\n", _cc_last_error(_cc_last_errno()));
                break;
            } else if (res == 0) {
                printf("exception close socket!\n");
                break;
            }
            
            buf[res] = 0;
            printf("get %ld bytes of exception data: %s \n",res,buf);
            bzero(&buf, sizeof(buf));
        }
    }

    return 0;
}

/**/
int main (int argc, char *argv[])
{
    struct sockaddr_in dest;
    bzero(&dest, sizeof(struct sockaddr_in));
    bzero(&buf, sizeof(buf));
    
    _cc_install_socket();
    
    _cc_inet_ipv4_addr(&dest, "127.0.0.1", 6379);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if ( sock < 0 ) {
        printf("socket fail: %s\n", _cc_last_error(_cc_last_errno()));
        return 0;
    }
    
    if (connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == 0) {
        char_t command[1024];
        printf("connect OK!\n");

        _cc_thread_start(redisCommanReply, "command-reply", NULL);

        while(keepActive) {
            int len = getInputLine(command, _cc_countof(command));
            if (_strnicmp(command, "quit", 4) == 0) {
                keepActive = false;
                break;
            }
            
            if (_strnicmp(command, "test", 4) == 0) {
                //char_t *c = "*3\r\n$3\r\nSET\r\n$3\r\nabc\r\n$3\r\n500\r\n";
                char_t *c = "*2\r\n$3\r\nGET\r\n$3\r\nabc\r\n";
                
                int clen = strlen(c);
                if (_cc_send(sock, (pvoid_t)c, clen) != clen) {
                    keepActive = false;
                    break;
                }
                continue;;
            }
            
            printf("input:%s\n", command);

            if (len > 0) {
                if (_cc_send(sock, (pvoid_t)command, len) != len) {
                    keepActive = false;
                    break;
                }

                _cc_send(sock, "\r\n", 2);
            }
        }
        _cc_close_socket(sock);
    } else {
        printf("socket connect fail:%s\n", _cc_last_error(_cc_last_errno()));
    }

    _cc_uninstall_socket();
    return 0;
}
