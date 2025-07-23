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
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <time.h>
#include <libcc.h>
#include <libcc/widgets/widgets.h>

#define LOOP_MAX 10000000

int main(int argc, char *argv[]) {
    // int32_t i = 0;
    // clock_t start, end;
    // _cc_buf_t buf;
    _cc_install_socket();
    // SetConsoleOutputCP(65001);
    _cc_logger_open_syslog(_CC_LOG_FACILITY_USER_, "test",_T("127.0.0.1"), _CC_PORT_SYSLOG_);
    //_cc_logger_open_syslog(_CC_LOG_FACILITY_USER_, "test", nullptr, _CC_PORT_SYSLOG_);

    // _cc_buf_alloc(&buf, 1024);
    // start = clock();
    // for (i = 0; i < LOOP_MAX; i++) {
    //     _cc_buf_cleanup(&buf);
    //     _cc_buf_appendf(&buf, "%s,%d,%s", "cc;", 100, "ghhgfhfdghfdhgfhfgdhgfdh");
    // }
    // end = clock()
    //     ;
    // printf("Buf: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    // start = clock();
    // for (i = 0; i < LOOP_MAX; i++) {
    //     _cc_buf_cleanup(&buf);
    //     _cc_buf_appendf(&buf, "%s,", "cName);");
    // }
    // end = clock();
    // printf("Buf: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    _cc_logger_error("test");
    _cc_loggerA(_CC_LOG_LEVEL_ERROR_,"test");

    return 0;
}
