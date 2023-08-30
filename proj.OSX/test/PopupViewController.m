//
//  PopViewController.m
//  httpd
//
//  Created by CC on 2021/10/22.
//

#import "PopupViewController.h"
#include "ViewController.h"

@interface PopupViewController ()

@end

@implementation PopupViewController

- (IBAction)exit:(id)sender {
    [[NSApplication sharedApplication] terminate:self];
}

- (IBAction)clickVisitGithub:(NSButton*)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://github.com/huai2001/CC/"]];
}
- (IBAction)clickSwitch:(id)sender {
    if ([self.btnAutoLaunched state] == NSControlStateValueOn) {
        [self autoLaunched:NO];
    } else {
        [self autoLaunched:YES];
    }
}

#pragma mark - 设置开机自启动
- (void)autoLaunched:(bool)del {
    NSString* launchFolder = [NSString stringWithFormat:@"%@/Library/LaunchAgents",NSHomeDirectory()];
    NSString* bundleID = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleIdentifierKey];
    NSString* dstLaunchPath = [launchFolder stringByAppendingFormat:@"/%@.plist",bundleID];
    
    if (del) {
        _cc_unlink([dstLaunchPath UTF8String]);
        return;
    }
    
    //NSLog(@"dstLaunchPath : %@",dstLaunchPath);
    
    NSFileManager* fm = [NSFileManager defaultManager];
    BOOL isDir = NO;
    //已经存在启动项中，就不必再创建
    if ([fm fileExistsAtPath:dstLaunchPath isDirectory:&isDir] && !isDir) {
        return;
    }
    
    //下面是一些配置
    NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
    NSMutableArray* arr = [[NSMutableArray alloc] init];
    [arr addObject:[[NSBundle mainBundle] executablePath]];
    [arr addObject:@"-runMode"];
    [arr addObject:@"autoLaunched"];
    [dict setObject:[NSNumber numberWithBool:true] forKey:@"RunAtLoad"];
    [dict setObject:bundleID forKey:@"Label"];
    [dict setObject:arr forKey:@"ProgramArguments"];
    isDir = NO;
    if (![fm fileExistsAtPath:launchFolder isDirectory:&isDir] && isDir) {
        [fm createDirectoryAtPath:launchFolder withIntermediateDirectories:NO attributes:nil error:nil];
    }
    
    [dict writeToFile:dstLaunchPath atomically:NO];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    BOOL isDir = NO;
    
    // Do view setup here.
    NSString* launchFolder = [NSString stringWithFormat:@"%@/Library/LaunchAgents",NSHomeDirectory()];
    NSString* bundleID = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleIdentifierKey];
    NSString* dstLaunchPath = [launchFolder stringByAppendingFormat:@"/%@.plist",bundleID];
    NSFileManager* fm = [NSFileManager defaultManager];
    
    if ([fm fileExistsAtPath:dstLaunchPath isDirectory:&isDir]) {
        [self.btnAutoLaunched setState:NSControlStateValueOn];
    } else {
        [self.btnAutoLaunched setState:NSControlStateValueOff];
    }
}

@end
