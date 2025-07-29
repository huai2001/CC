#include <stdio.h>
#include <libcc.h>
#include <libcc/widgets/ini.h>

const tchar_t *iniconf = _T("[reactor]\n")\
                         _T("maxConn = 1024   /*abc*/\n")\
                         _T("threadNum = 5   ;1234\n")\
                         _T("ip = \"127.0.0.1\"\n")\
                         _T("port = 7779\n")\
                         _T("\n")\
                         _T("//double-slash comments, to end of line.\n")\
                         _T("; comments, to end of line.\n")\
                         _T("[mysql]\n")\
                         _T("db_host = '127.0.0.1'  \n")\
                         _T("db_port = 3306\n")\
                         _T("db_user = root \n")\
                         _T("db_passwd = **Your PassWord**\n")\
                         _T("db_name = lars dns\n")\
                         _T("\n")\
                         _T("[reactor]\n")\
                         _T("db_thread_cnt = 3  \n");


int _tmain (int argc, tchar_t * const argv[]) {
    _cc_ini_t *ini = _cc_parse_ini(iniconf);
    if (ini) {
        _cc_buf_t buf;
        _cc_ini_t *section = _cc_ini_find(ini, _T("mysql"));
        if (section) {
            _tprintf(_T("HOST:%s\n"), _cc_ini_find_string(section, _T("db_host")));
            _tprintf(_T("PSWD:%s\n"), _cc_ini_find_string(section, _T("db_passwd")));
        }
        _cc_dump_ini(ini, &buf);
        _tprintf(_T("%s"), (tchar_t*)_cc_buf_stringify(&buf));
    } else {
        _tprintf(_T("error:%s"), _cc_ini_error());
    }


    system("pause");
    return 0;
}
