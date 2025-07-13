//
//  ViewController.m
//  test
//
//  Created by CC on 2023/2/20.
//

#import "ViewController.h"

@interface ViewController()<NSTableViewDelegate,NSTableViewDataSource>
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do any additional setup after loading the view.
    self.btnVisit.hidden = YES;
    
    [self.cboxFamily removeAllItems];
    [self.cboxFamily addItemWithObjectValue:@"IPv4"];
    [self.cboxFamily addItemWithObjectValue:@"IPv6"];
    [self.cboxFamily addItemWithObjectValue:@"自动"];
    
    NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
    [formatter setNumberStyle:NSNumberFormatterNoStyle];
    [self.editListenerPort setFormatter:formatter];
    
    [self.editListenerPort setStringValue:@"8088"];
    //[self.editDocumentRoot setStringValue:@""];
    [self.cboxFamily selectItemAtIndex:0];
    
    [self initTableView];
    
    //[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(textChange) name:NSControlTextDidEndEditingNotification object:self.editFilters];
}

-(void) textChange {
    //[self tableDataArrayFilters];
}

- (IBAction)clickVisit:(NSButton *)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:self.visitURL]];
}

- (IBAction)clickVisitGithub:(NSButton*)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://github.com/huai2001/CC/"]];
}

- (void)dealloc {
    //移除监听
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}

- (IBAction)clickBrowse:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    // 设置默认选中按钮的显示（OK 、打开，Open ...）
    [panel setPrompt: @"选择"];
    // 设置面板上的提示信息
    [panel setMessage: @"选择站点目录"];
    // 是否可以选择文件夹
    [panel setCanChooseDirectories: YES];
    // 是否可以创建文件夹
    [panel setCanCreateDirectories: YES];
    // 是否可以选择文件
    [panel setCanChooseFiles: NO];
    // 是否可以多选
    [panel setAllowsMultipleSelection: NO];
    // 所能选择的文件类型
    //[panel setAllowedFileTypes: [NSArray arrayWithObjects:@"png",@"jpg",@"bmp", nil]];
    // 默认打开路径（桌面、 下载、...）
    [panel setDirectoryURL:nullptr];
    __block NSArray *chooseFiles;
    [panel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSModalResponse result) {
        if (result == NSModalResponseOK) {
            chooseFiles = [panel URLs];
            if (chooseFiles.count > 0) {
                self.editDocumentRoot.stringValue = [chooseFiles[0] path];
            }
        } else if(result == NSModalResponseCancel) {
            NSLog(@"Click cancle");
        }
    }];
}

-(void) startup {
    if (1) {
        [BasicsController setRunning:YES];
        self.btnStartting.title = @"停止";
        self.btnBrowse.enabled = NO;
        self.editDocumentRoot.enabled = NO;
        self.editListenerHost.enabled = NO;
        self.editListenerPort.enabled = NO;
        self.cboxFamily.enabled = NO;
        self.btnVisit.hidden = NO;
        
        NSString *ip = self.localhost;
        if (self.family == AF_INET6) {
            ip = [NSString stringWithFormat:@"[%@]", self.localhost];
        }
        
        NSInteger port = [self.editListenerPort intValue];
        [self.tipURL setStringValue:[NSString stringWithFormat:@"Listen: %@:%ld", ip, port]];

    }
}

-(void) stop {
    [BasicsController setRunning:NO];
    self.btnStartting.title = @"启动";
    self.btnBrowse.enabled = YES;
    self.editDocumentRoot.enabled = YES;
    self.editListenerHost.enabled = YES;
    self.editListenerPort.enabled = YES;
    self.cboxFamily.enabled = YES;
    self.btnVisit.hidden = YES;
    [self.tipURL setStringValue:@""];
}


- (IBAction)clickStartting:(id)sender {
    if ([BasicsController isRunning] == NO) {
        NSInteger index = [self.cboxFamily indexOfSelectedItem];
        NSString *v = [self.cboxFamily itemObjectValueAtIndex:index];
        //NSString *document = [self.editDocumentRoot stringValue];
        NSString *host = [self.editListenerHost stringValue];
        NSInteger port = [self.editListenerPort intValue];
        /*if ([self isEmptyString:document]) {
            [self alert:@"请选择站点目录" Title:@"无效的站点目录"];
            return;
        }*/
        
        self.family = 0;
        if ([v caseInsensitiveCompare:@"ipv6"] == NSOrderedSame) {
            self.family = AF_INET6;
        } else if ([v caseInsensitiveCompare:@"ipv4"] == NSOrderedSame) {
            self.family = AF_INET;
        }
        /*
        if ([self isEmptyString:host]) {
            [self alert:@"主机IP不能为空请输入0.0.0.0 或 ::1" Title:@"主机IP"];
            return;
        }*/
        if (![self isEmptyString:host]) {
            if (self.family == AF_INET6) {
                struct sockaddr_in6 addr;
                if (!_cc_inet_pton(AF_INET6, [host UTF8String], (byte_t*)&addr.sin6_addr)) {
                    [self alert:@"监听主机地址不是有效的IPv6类" Title:@"监听主机地址无效"];
                    return;
                }
            } else if (self.family == AF_INET) {
                struct sockaddr_in addr;
                if (!_cc_inet_pton(AF_INET, [host UTF8String], (byte_t*)&addr.sin_addr)) {
                    [self alert:@"监听主机地址不是有效的IPv4类" Title:@"监听主机地址无效"];
                    return;
                }
            } else {
                struct sockaddr_in6 addr;
                if (_cc_inet_pton(AF_INET6, [host UTF8String], (byte_t*)&addr.sin6_addr)) {
                    self.family = AF_INET6;
                } else {
                    struct sockaddr_in addr;
                    if (_cc_inet_pton(AF_INET, [host UTF8String], (byte_t*)&addr.sin_addr)) {
                        self.family = AF_INET;
                    }
                    if (self.family == 0) {
                        [self alert:@"监听主机地址不是有效的,请重新输入！" Title:@"监听主机地址无效"];
                        return ;
                    }
                }
            }
        }
        
        self.localhost = [self getDeviceIpAddresses:self.family?self.family:AF_INET];
        if (![self isEmptyString:host]) {
            if ([host caseInsensitiveCompare:self.localhost] != NSOrderedSame) {
                self.localhost = host;
            }
        }
        /*
        BOOL isDir = YES;
        NSFileManager* fm = [NSFileManager defaultManager];
        if (![fm fileExistsAtPath:document isDirectory:&isDir]) {
            [self alert:@"目录无法访问，请重新选择目录" Title:@"无效的目录"];
            return;
        }
        */
        if (port <= 0 || port >= 65535) {
            [self alert:@"请输入端口号 (1 ~ 65534) 之间" Title:@"端口值无效"];
            return;
        }
        
        [self startup];
    } else {
        [self stop];
    }
}

- (void)initTableView {
    NSTabViewItem *tabViewItem = [self.tabView.tabViewItems objectAtIndex:1];
    NSRect rect = tabViewItem.view.bounds;

    NSScrollView *scrollView = [[NSScrollView alloc] init];
    scrollView.hasVerticalScroller  = YES;
    scrollView.frame = CGRectMake(rect.origin.x, rect.origin.y + 30, rect.size.width, rect.size.height - 30);
   
    [tabViewItem.view addSubview:scrollView];
    
    
    NSTableColumn * column1 = [[NSTableColumn alloc]initWithIdentifier:@"ext"];
    NSTableColumn * column2 = [[NSTableColumn alloc]initWithIdentifier:@"desc"];
    column1.width = 100;
    column1.maxWidth = 100;
    column1.title = @"扩展名";
    column1.editable = YES ;
    
    column2.width = scrollView.frame.size.width - 120;
    column2.minWidth = 100;
    column2.title = @"类型描述";
    column2.editable = YES ;
    
    self.tableView = [[NSTableView alloc]initWithFrame:scrollView.frame];
    [self.tableView addTableColumn:column1];
    [self.tableView addTableColumn:column2];
    scrollView.contentView.documentView = self.tableView;
    
    
    self.tableView.delegate = self;
    self.tableView.dataSource = self;
    
    //[self loadMimeTableView];
}

//设置行数 通用
-(NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.tableDataArray.count;
}

//绑定数据
-(id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {

    if (tableColumn == [self.tableView.tableColumns objectAtIndex:0]) {
        return self.tableDataArray[row][0];
    }
    
    if (tableColumn == [self.tableView.tableColumns objectAtIndex:1]) {
        return self.tableDataArray[row][1];
    }
    return nil;
}

//用户编辑列表
- (void)tableView:(NSTableView *)tableView setObjectValue:(nullptrable id)object forTableColumn:(nullptrable NSTableColumn *)tableColumn row:(NSInteger)row {
    if (tableColumn == [self.tableView.tableColumns objectAtIndex:0]) {
        self.tableDataArray[row][0] = object;
    }
    if (tableColumn == [self.tableView.tableColumns objectAtIndex:1]) {
        self.tableDataArray[row][1] = object;
    }
}

//cell-base的cell展示前调用 可以进行自定制
- (void)tableView:(NSTableView *)tableView willDisplayCell:(id)cell forTableColumn:(nullptrable NSTableColumn *)tableColumn row:(NSInteger)row {
    //NSTextFieldCell * _cell = cell;
   //_cell.textColor = [NSColor redColor];
}

//设置是否可以进行编辑
- (BOOL)tableView:(NSTableView *)tableView shouldEditTableColumn:(nullptrable NSTableColumn *)tableColumn row:(NSInteger)row {
    return YES;
}

@end
