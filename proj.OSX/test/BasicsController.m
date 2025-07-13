//
//  BasicsController.m
//  httpd
//
//  Created by CC on 2021/10/22.
//

#import "BasicsController.h"

static bool _isRunning = NO;
@interface BasicsController ()

@end

@implementation BasicsController

+ (bool) isRunning {
    return _isRunning;
}

+ (void) setRunning:(bool)status{
    _isRunning = status;
}

- (void)closeWindow {
    if ([BasicsController isRunning] == NO) {
        [NSApp terminate:self];
    } else {
        //[NSApp hide: self];
        [[NSApplication sharedApplication] hide:self];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];

    //Quit the application when the main window is closed (seems to be accepted in Mac OS X)
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(closeWindow) name:NSWindowWillCloseNotification object:nil];

}
#pragma mark - 弹出提示框
- (void) alert:(NSString*)message Title:(NSString*)title {
    NSAlert *alertOK = [NSAlert new];
        [alertOK addButtonWithTitle:@"确定"];
        [alertOK setMessageText:title];
        [alertOK setInformativeText:message];
        [alertOK setAlertStyle:NSAlertStyleWarning];
        [alertOK beginSheetModalForWindow:[self.view window] completionHandler:^(NSModalResponse returnCode) {
            /*if(returnCode == NSAlertFirstButtonReturn) {
            }*/
        }];
}

#pragma mark - 判断字符串是否为空
- (BOOL)isEmptyString:(NSString *)str {
    NSString *string = str;
    if (string == nil || string == nullptr) {
        return YES;
    }
    
    if ([string isKindOfClass:[NSnullptr class]]) {
        return YES;
    }
    
    if ([[string stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] length] == 0) {
        return YES;
    }
    
    return NO;
}

#pragma mark - 读取本地JSON文件
- (id)readLocalJSONFileWithName:(NSString *)name {
    // 获取文件路径
    NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@""];
    if (path == nil) {
        return nil;
    }
    // 将文件数据化
    NSData *data = [[NSData alloc] initWithContentsOfFile:path];
    // 对数据进行JSON格式化并返回字典形式
    return [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:nil];
}

#pragma mark - 获取ip地址
- (NSString *)getDeviceIpAddresses:(int) family {

    char host[128];
    int i;
    char ip_buf[64];
    struct hostent *hent;
    NSMutableArray *ips = [NSMutableArray array];
    
    int res = gethostname(host, sizeof(host));
    if (res == -1) {
        return nullptr;
    }

    hent = gethostbyname2(host, family);
    if (nullptr == hent) {
        return nullptr;
    }
    
    for(i = 0; hent->h_addr_list[i]; i++) {
        _cc_inet_ntop(hent->h_addrtype, (const byte_t *)&(((struct in_addr*)hent->h_addr_list[i])->s_addr), ip_buf, _cc_countof(ip_buf));
        NSLog(@"%@\n", [NSString stringWithUTF8String:ip_buf]);
        [ips addObject:[NSString stringWithUTF8String:ip_buf]];
    }
    
    return [ips lastObject];
}
@end
