//
//  AppDelegate.m
//  test
//
//  Created by CC on 2023/2/20.
//

#import "AppDelegate.h"
#import "PopupViewController.h"

@interface AppDelegate ()


@end

@implementation AppDelegate

- (void)clickStatusBar:(NSStatusBarButton *)item {
    //NSLog(@"statusOnClick ----- ");
    [self.popover showRelativeToRect:item.bounds ofView:item preferredEdge:NSRectEdgeMaxY];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
    NSStatusBar *statusBar = [NSStatusBar systemStatusBar];
    self.statusItem = [statusBar statusItemWithLength: NSSquareStatusItemLength];
    
    [self.statusItem.button setImage:[NSImage imageNamed:@"StatusBar"]];
    [self.statusItem.button setToolTip:@"这是一个个人WebServer"];
    [self.statusItem.button setTarget:self];
    [self.statusItem.button setAction:@selector(clickStatusBar:)];
    
    self.popover = [[NSPopover alloc] init];
    self.popover.behavior = NSPopoverBehaviorTransient;
    self.popover.contentViewController = [[PopupViewController alloc]initWithNibName:@"PopupViewController" bundle:nil];
    
    // 防止下面的block方法中造成循环引用
    __weak typeof (self) weakSelf = self;
    // 添加对鼠标左键进行事件监听
    // 如果想对其他事件监听也进行监听，可以修改第一个枚举参数： NSEventMaskLeftMouseDown | 你要监听的其他枚举值
    [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown handler:^(NSEvent * event) {
        if (weakSelf.popover.isShown) {
            // 关闭popover；
            [weakSelf.popover close];
        }
    }];
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
    NSStatusBar *statusBar = [NSStatusBar systemStatusBar];
    //删除item
    [statusBar removeStatusItem:self.statusItem];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag {
    if (!flag) {
        //[[theApplication windows][0] makeKeyAndOrderFront:nil];
        for (NSWindow* window in [theApplication windows]) {
            [window makeKeyAndOrderFront:nil];
        }
        
        return YES;
    }
    return NO;
}

@end
