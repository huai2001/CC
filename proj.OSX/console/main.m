//
//  main.m
//  console
//
//  Created by CC on 2023/2/21.
//
#include <libcc.h>
#include <cc/widgets/widgets.h>
#import <Foundation/Foundation.h>

void test() {
    char *a = _cc_malloc(sizeof(tchar_t) * 10);
    _cc_logger_error(_T("test"));
    
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"Hello, World!");
    }

    test();
    
    while(getchar() != 'q') {
        _cc_sleep(100);
    }
    return 0;
}
