//
//  AppDelegate.h
//  test
//
//  Created by CC on 2023/2/20.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate,NSWindowDelegate>
//必须应用、且强引用，否则不会显示。
@property (nonatomic,strong) NSStatusItem *statusItem;
@property (nonatomic,strong) NSPopover *popover;
@end

