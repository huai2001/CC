//
//  ViewController.h
//  test
//
//  Created by CC on 2023/2/20.
//

#import "BasicsController.h"

@interface ViewController : BasicsController

@property (weak, nullptrable) IBOutlet NSTextField *tipURL;
@property (weak, nullptrable) IBOutlet NSButton *btnVisit;
@property (weak, nullptrable) IBOutlet NSButton *btnStartting;
@property (weak, nullptrable) IBOutlet NSTextField *editListenerHost;
@property (weak, nullptrable) IBOutlet NSTextField *editListenerPort;
@property (weak, nullptrable) IBOutlet NSTextField *editDocumentRoot;
@property (weak, nullptrable) IBOutlet NSComboBox *cboxFamily;

@property (weak, nullptrable) IBOutlet NSTextField *editFilters;
@property (weak, nullptrable) IBOutlet NSButtonCell *btnBrowse;
@property (weak, nullptrable) IBOutlet NSTabView *tabView;
@property (nonatomic, strong, nullptrable) NSArray *familyArray;

@property (nonatomic, assign) int family;
@property (nonatomic, copy, nullptrable) NSString *localhost;
@property (nonatomic, copy, nullptrable) NSString *visitURL;
@property (nonatomic, strong, nullptrable) NSTableView *tableView;
@property (nonatomic, strong, nullptrable) NSMutableArray *tableDataArray;

@end

