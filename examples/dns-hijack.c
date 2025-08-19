#include <stdio.h>
#include <libcc.h>
#include <libcc/widgets/widgets.h>
#include <locale.h>

int c = 0;

int main (int argc, char * const argv[]) {
    //_cc_dns_t dns;
    //char *dns_servers_list[2] = {"114.114.114.114", "223.5.5.5"};
    setlocale( LC_CTYPE, "chs" );
    _cc_install_async_event(0, nullptr);

    //bzero(&dns, sizeof(_cc_dns_t));

    _cc_dns_listen();
    /*Get the DNS servers from the resolv.conf file*/
    //_cc_dns_servers((const char **)dns_servers_list, _cc_countof(dns_servers_list));

    //_cc_dns_lookup(&dns, "www.baidu.com", _CC_DNS_T_A_);
    //_cc_dns_lookup(&dns, "www.a.shifen.com", _CC_DNS_T_A_);



    while ((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    //_cc_dns_free(&dns);
    _cc_uninstall_async_event();

    return 0;
}