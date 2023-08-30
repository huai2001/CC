//
//  ViewController.h
//  test
//
//  Created by CC on 2023/2/20.
//

#import "BasicsController.h"

@interface ViewController : BasicsController

@property (weak, nullable) IBOutlet NSTextField *tipURL;
@property (weak, nullable) IBOutlet NSButton *btnVisit;
@property (weak, nullable) IBOutlet NSButton *btnStartting;
@property (weak, nullable) IBOutlet NSTextField *editListenerHost;
@property (weak, nullable) IBOutlet NSTextField *editListenerPort;
@property (weak, nullable) IBOutlet NSTextField *editDocumentRoot;
@property (weak, nullable) IBOutlet NSComboBox *cboxFamily;

@property (weak, nullable) IBOutlet NSTextField *editFilters;
@property (weak, nullable) IBOutlet NSButtonCell *btnBrowse;
@property (weak, nullable) IBOutlet NSTabView *tabView;
@property (nonatomic, strong, nullable) NSArray *familyArray;

@property (nonatomic, assign) int family;
@property (nonatomic, copy, nullable) NSString *localhost;
@property (nonatomic, copy, nullable) NSString *visitURL;
@property (nonatomic, strong, nullable) NSTableView *tableView;
@property (nonatomic, strong, nullable) NSMutableArray *tableDataArray;

@end

