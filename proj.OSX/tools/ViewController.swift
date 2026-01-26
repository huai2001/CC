//
//  ViewController.swift
//  tools
//
//  Created by . Qiu on 2026/1/21.
//

import Cocoa

class ViewController: NSViewController {
    @IBOutlet weak var cleanupButton: NSButtonCell!
    @IBOutlet weak var fileTextField: NSTextFieldCell!
    @IBOutlet weak var browserButton: NSButtonCell!
    override func viewDidLoad() {
        super.viewDidLoad()
        // 设置为不可编辑（推荐）
        fileTextField.isEditable = false
        cleanupButton.isEnabled = false
        // Do any additional setup after loading the view.
    }

    @IBAction func browser(_ sender: Any) {
        let panel = NSOpenPanel()
        panel.canChooseFiles = true
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.prompt = "选择"
        panel.message = "请选择一个目录或者文件"
        
        if panel.runModal() == .OK {
            if let url = panel.url {
                fileTextField.stringValue = url.path
                fileTextField.textColor = NSColor.black
                cleanupButton.isEnabled = true
            }
        }
    }
    
    @IBAction func cleanup(_ sender: Any) {
        cleanupButton.isEnabled = false
        fileTextField.stringValue = ""
    }
    
    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
}

