# StupidDNS 子对话框设计需求

## 1. DNS服务器管理对话框

### 资源定义
**对话框 ID**: IDD_DNS_SETTINGS_DIALOG
**尺寸**: 500x400
**标题**: "DNS服务器管理"

### 控件 ID 定义
- IDC_LIST_DNS_SERVERS (DNS服务器列表)
- IDC_BTN_ADD_DNS (添加按钮)
- IDC_BTN_DELETE_DNS (删除按钮)
- IDC_BTN_EDIT_DNS (编辑按钮)
- IDC_BTN_TEST_DNS (测速按钮)
- IDC_BTN_TEST_ALL_DNS (全部测速按钮)
- IDC_BTN_IMPORT_DNS (导入按钮)
- IDC_BTN_EXPORT_DNS (导出按钮)
- IDC_BTN_RESET_DNS (恢复默认按钮)
- IDC_CHECK_AUTO_TEST (自动测速复选框)
- IDOK (确定按钮)
- IDCANCEL (取消按钮)

### 界面布局
```
┌─ DNS服务器列表 ──────────────────────────────┐
│ [√] 香港HKNet        202.45.84.58      45ms  │
│ [√] 台湾HiNet        168.95.1.1        52ms  │
│ [ ] 日本OpenDNS      210.141.113.7     78ms  │
│ [√] 新加坡SingNet    165.21.83.88      38ms  │
│ [√] 韩国KT           168.126.63.1      65ms  │
│ ...                                           │
└─────────────────────────────────────────────┘

[添加] [删除] [编辑] [测速] [全部测速]

[√] 自动测速并优选最快的5个DNS

[导入列表] [导出列表] [恢复默认]     [确定] [取消]
```

### 控件位置
- ListControl (IDC_LIST_DNS_SERVERS) at (10,10,480,280)
  列定义：启用(40), 名称(120), IP地址(150), 延迟(80), 状态(80)
  样式：LVS_REPORT | LVS_SINGLESEL | WS_BORDER | WS_TABSTOP
  扩展样式：LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES

- Button (IDC_BTN_ADD_DNS) at (10,300,60,25) "添加"
- Button (IDC_BTN_DELETE_DNS) at (80,300,60,25) "删除"
- Button (IDC_BTN_EDIT_DNS) at (150,300,60,25) "编辑"
- Button (IDC_BTN_TEST_DNS) at (220,300,60,25) "测速"
- Button (IDC_BTN_TEST_ALL_DNS) at (290,300,80,25) "全部测速"

- CheckBox (IDC_CHECK_AUTO_TEST) at (10,335,250,20) "自动测速并优选最快的5个DNS"

- Button (IDC_BTN_IMPORT_DNS) at (10,365,80,25) "导入列表"
- Button (IDC_BTN_EXPORT_DNS) at (100,365,80,25) "导出列表"
- Button (IDC_BTN_RESET_DNS) at (190,365,80,25) "恢复默认"
- Button (IDOK) at (330,365,80,25) "确定"
- Button (IDCANCEL) at (420,365,70,25) "取消"

### 类定义 (DNSSettingsDlg.h)
```cpp
class CDNSSettingsDlg : public CDialogEx
{
public:
    CDNSSettingsDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_DNS_SETTINGS_DIALOG };

protected:
    CListCtrl m_listDNS;
    BOOL m_bAutoTest;
    
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    
    afx_msg void OnBnClickedAddDns();
    afx_msg void OnBnClickedDeleteDns();
    afx_msg void OnBnClickedEditDns();
    afx_msg void OnBnClickedTestDns();
    afx_msg void OnBnClickedTestAllDns();
    afx_msg void OnBnClickedImportDns();
    afx_msg void OnBnClickedExportDns();
    afx_msg void OnBnClickedResetDns();
    afx_msg void OnBnClickedOk();
    
    void InitDNSList();
    void LoadDefaultDNS();
    void TestDNSSpeed(int index);
    
    DECLARE_MESSAGE_MAP()
};
```

### 功能实现要点
- OnInitDialog: 初始化列表，加载默认DNS服务器（香港、台湾、日本、韩国、新加坡各2-3个）
- LoadDefaultDNS: 预置10-15个亚洲DNS服务器
- OnBnClickedAddDns: 弹出输入对话框，添加新DNS（名称、IP）
- OnBnClickedTestDns: 对选中的DNS进行ICMP ping测速
- OnBnClickedTestAllDns: 遍历所有启用的DNS测速，更新延迟列
- OnBnClickedResetDns: 恢复到默认DNS列表

---

## 2. 域名规则编辑对话框

### 资源定义
**对话框 ID**: IDD_DOMAIN_RULES_DIALOG
**尺寸**: 550x450
**标题**: "域名规则管理"

### 控件 ID 定义
- IDC_TAB_RULES (规则分类标签)
- IDC_LIST_CHINA_DOMAINS (国内域名列表)
- IDC_LIST_FOREIGN_DOMAINS (国外域名列表)
- IDC_LIST_CUSTOM_DOMAINS (自定义域名列表)
- IDC_EDIT_DOMAIN_INPUT (域名输入框)
- IDC_BTN_ADD_RULE (添加按钮)
- IDC_BTN_DELETE_RULE (删除按钮)
- IDC_BTN_IMPORT_RULES (导入按钮)
- IDC_BTN_EXPORT_RULES (导出按钮)
- IDC_STATIC_RULE_COUNT (规则计数显示)
- IDOK, IDCANCEL

### 界面布局
```
┌─ 域名规则分类 ─────────────────────────────┐
│ [国内域名] [国外域名] [自定义规则]          │
├───────────────────────────────────────────┤
│ 当前规则列表：                 共 1234 条  │
│ ┌─────────────────────────────────────┐  │
│ │ *.baidu.com          [国内DNS]       │  │
│ │ *.taobao.com         [国内DNS]       │  │
│ │ *.qq.com             [国内DNS]       │  │
│ │ ...                                  │  │
│ └─────────────────────────────────────┘  │
│                                           │
│ 添加规则: [________________] [添加]       │
│                                           │
│ [删除选中] [导入规则] [导出规则]           │
└───────────────────────────────────────────┘

[确定] [取消]
```

### 控件位置
- TabControl (IDC_TAB_RULES) at (10,10,530,30)
  标签页："国内域名", "国外域名", "自定义规则"

- Static (IDC_STATIC_RULE_COUNT) at (400,50,130,20) "共 0 条规则"

- ListControl (IDC_LIST_CHINA_DOMAINS) at (10,50,530,310)
  列定义：域名模式(300), 规则类型(150), 备注(70)
  
- EditControl (IDC_EDIT_DOMAIN_INPUT) at (10,370,400,25)
- Button (IDC_BTN_ADD_RULE) at (420,370,60,25) "添加"

- Button (IDC_BTN_DELETE_RULE) at (10,405,80,25) "删除选中"
- Button (IDC_BTN_IMPORT_RULES) at (100,405,80,25) "导入规则"
- Button (IDC_BTN_EXPORT_RULES) at (190,405,80,25) "导出规则"

- Button (IDOK) at (380,405,80,25) "确定"
- Button (IDCANCEL) at (470,405,70,25) "取消"

### 类定义 (DomainRulesDlg.h)
```cpp
class CDomainRulesDlg : public CDialogEx
{
public:
    CDomainRulesDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_DOMAIN_RULES_DIALOG };

protected:
    CTabCtrl m_tabRules;
    CListCtrl m_listChinaDomains;
    CListCtrl m_listForeignDomains;
    CListCtrl m_listCustomDomains;
    CEdit m_editDomainInput;
    int m_nCurrentTab;
    
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    
    afx_msg void OnTcnSelchangeTabRules(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedAddRule();
    afx_msg void OnBnClickedDeleteRule();
    afx_msg void OnBnClickedImportRules();
    afx_msg void OnBnClickedExportRules();
    
    void InitRuleLists();
    void SwitchRuleList(int tabIndex);
    void LoadDefaultRules();
    void UpdateRuleCount();
    
    DECLARE_MESSAGE_MAP()
};
```

### 功能实现要点
- 支持3个标签页切换，显示不同类型域名规则
- 预置常见国内域名（.cn, baidu.com, qq.com, taobao.com等）
- 预置常见国外域名（google.com, facebook.com, twitter.com等）
- 支持通配符规则（*.example.com）
- 导入导出为文本格式（每行一个域名）

---

## 3. 高级选项对话框

### 资源定义
**对话框 ID**: IDD_ADVANCED_DIALOG
**尺寸**: 450x400
**标题**: "高级选项"

### 控件 ID 定义
- IDC_CHECK_ENABLE_CACHE (启用缓存)
- IDC_EDIT_CACHE_SIZE (缓存大小)
- IDC_EDIT_CACHE_TTL (缓存有效期)
- IDC_CHECK_POLLUTION_DETECT (启用污染检测)
- IDC_EDIT_QUERY_TIMEOUT (查询超时)
- IDC_EDIT_CONCURRENT_QUERIES (并发查询数)
- IDC_CHECK_ENABLE_LOG (启用日志)
- IDC_EDIT_LOG_PATH (日志路径)
- IDC_BTN_BROWSE_LOG (浏览按钮)
- IDC_CHECK_AUTO_START (开机自启)
- IDC_CHECK_MINIMIZE_TO_TRAY (最小化到托盘)
- IDC_COMBO_LOG_LEVEL (日志级别)
- IDOK, IDCANCEL

### 界面布局
```
┌─ DNS缓存设置 ─────────────────────────┐
│ [√] 启用DNS缓存                        │
│     缓存大小: [10000] 条               │
│     缓存有效期: [3600] 秒              │
└───────────────────────────────────────┘

┌─ 查询设置 ────────────────────────────┐
│ [√] 启用DNS污染检测                    │
│     查询超时: [2000] 毫秒              │
│     并发查询DNS数量: [5] 个            │
└───────────────────────────────────────┘

┌─ 日志设置 ────────────────────────────┐
│ [√] 启用文件日志                       │
│     日志路径: [C:\logs\dns.log] [...]  │
│     日志级别: [详细 ▼]                 │
└───────────────────────────────────────┘

┌─ 其他设置 ────────────────────────────┐
│ [√] 开机自动启动服务                   │
│ [√] 最小化到系统托盘                   │
└───────────────────────────────────────┘

            [恢复默认]  [确定] [取消]
```

### 控件位置
- GroupBox at (10,10,430,90) "DNS缓存设置"
- CheckBox (IDC_CHECK_ENABLE_CACHE) at (20,25,150,20) "启用DNS缓存"
- Static at (30,50,80,20) "缓存大小:"
- EditControl (IDC_EDIT_CACHE_SIZE) at (120,48,80,20) ES_NUMBER
- Static at (210,50,20,20) "条"
- Static at (30,75,80,20) "缓存有效期:"
- EditControl (IDC_EDIT_CACHE_TTL) at (120,73,80,20) ES_NUMBER
- Static at (210,75,20,20) "秒"

- GroupBox at (10,110,430,90) "查询设置"
- CheckBox (IDC_CHECK_POLLUTION_DETECT) at (20,125,150,20) "启用DNS污染检测"
- Static at (30,150,80,20) "查询超时:"
- EditControl (IDC_EDIT_QUERY_TIMEOUT) at (120,148,80,20) ES_NUMBER
- Static at (210,150,30,20) "毫秒"
- Static at (30,175,100,20) "并发查询DNS数量:"
- EditControl (IDC_EDIT_CONCURRENT_QUERIES) at (140,173,60,20) ES_NUMBER
- Static at (210,175,20,20) "个"

- GroupBox at (10,210,430,90) "日志设置"
- CheckBox (IDC_CHECK_ENABLE_LOG) at (20,225,150,20) "启用文件日志"
- Static at (30,250,80,20) "日志路径:"
- EditControl (IDC_EDIT_LOG_PATH) at (100,248,280,20)
- Button (IDC_BTN_BROWSE_LOG) at (390,248,40,20) "..."
- Static at (30,275,80,20) "日志级别:"
- ComboBox (IDC_COMBO_LOG_LEVEL) at (100,273,100,100) CBS_DROPDOWNLIST
  选项："简洁", "正常", "详细", "调试"

- GroupBox at (10,310,430,50) "其他设置"
- CheckBox (IDC_CHECK_AUTO_START) at (20,325,200,20) "开机自动启动服务"
- CheckBox (IDC_CHECK_MINIMIZE_TO_TRAY) at (230,325,200,20) "最小化到系统托盘"

- Button (IDC_BTN_RESET_ADVANCED) at (180,370,80,25) "恢复默认"
- Button (IDOK) at (270,370,80,25) "确定"
- Button (IDCANCEL) at (360,370,70,25) "取消"

### 类定义 (AdvancedDlg.h)
```cpp
class CAdvancedDlg : public CDialogEx
{
public:
    CAdvancedDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_ADVANCED_DIALOG };

protected:
    BOOL m_bEnableCache;
    int m_nCacheSize;
    int m_nCacheTTL;
    BOOL m_bPollutionDetect;
    int m_nQueryTimeout;
    int m_nConcurrentQueries;
    BOOL m_bEnableLog;
    CString m_strLogPath;
    CComboBox m_comboLogLevel;
    BOOL m_bAutoStart;
    BOOL m_bMinimizeToTray;
    
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    
    afx_msg void OnBnClickedBrowseLog();
    afx_msg void OnBnClickedResetAdvanced();
    afx_msg void OnBnClickedOk();
    
    void LoadSettings();
    void SaveSettings();
    void SetDefaultValues();
    
    DECLARE_MESSAGE_MAP()
};
```

### 功能实现要点
- 默认值：缓存10000条/3600秒，超时2000ms，并发5个
- 日志路径浏览使用 CFolderPickerDialog
- 保存设置到注册表或INI文件
- 验证输入范围（缓存>0, 超时100-10000, 并发1-10）

---

## 4. 关于对话框

### 资源定义
**对话框 ID**: IDD_ABOUT_DIALOG
**尺寸**: 350x250
**标题**: "关于 StupidDNS"

### 控件 ID 定义
- IDC_STATIC_LOGO (图标/Logo)
- IDC_STATIC_APP_NAME (应用名称)
- IDC_STATIC_VERSION (版本信息)
- IDC_STATIC_COPYRIGHT (版权信息)
- IDC_STATIC_DESCRIPTION (功能描述)
- IDC_LINK_GITHUB (GitHub链接)
- IDOK

### 界面布局
```
        ┌───────┐
        │ LOGO  │
        └───────┘
        
    StupidDNS
    智能DNS解析服务
    
    版本: 1.0.0
    构建日期: 2025-01-24
    
    © 2025 StupidDNS Project
    对抗DNS污染，捍卫网络自由
    
    GitHub: github.com/yourname/stupiddns
    
            [确定]
```

### 控件位置
- Static (IDC_STATIC_LOGO) at (145,20,60,60) - Icon显示
- Static (IDC_STATIC_APP_NAME) at (10,90,330,25) "StupidDNS" 居中，字体加大
- Static at (10,115,330,20) "智能DNS解析服务" 居中
- Static (IDC_STATIC_VERSION) at (10,145,330,20) "版本: 1.0.0" 居中
- Static at (10,165,330,20) "构建日期: 2025-01-24" 居中
- Static (IDC_STATIC_COPYRIGHT) at (10,195,330,20) "© 2025 StupidDNS Project" 居中
- Static (IDC_STATIC_DESCRIPTION) at (10,215,330,20) "对抗DNS污染，捍卫网络自由" 居中
- Link (IDC_LINK_GITHUB) at (10,240,330,20) "GitHub: github.com/yourname/stupiddns" 居中

- Button (IDOK) at (135,270,80,25) "确定"

### 类定义 (AboutDlg.h)
```cpp
class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
    enum { IDD = IDD_ABOUT_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    
    afx_msg void OnNMClickLinkGithub(NMHDR *pNMHDR, LRESULT *pResult);
    
    DECLARE_MESSAGE_MAP()
};
```

### 功能实现要点
- 居中对齐所有文本
- GitHub链接可点击，打开浏览器
- 可添加应用程序图标显示

---

## 主对话框连接子对话框

在 StupidDNSDlg.cpp 中实现按钮响应：
```cpp
void CStupidDNSDlg::OnBnClickedDNSSettings()
{
    CDNSSettingsDlg dlg;
    dlg.DoModal();
}

void CStupidDNSDlg::OnBnClickedDomainRules()
{
    CDomainRulesDlg dlg;
    dlg.DoModal();
}

void CStupidDNSDlg::OnBnClickedAdvanced()
{
    CAdvancedDlg dlg;
    dlg.DoModal();
}

void CStupidDNSDlg::OnBnClickedAbout()
{
    CAboutDlg dlg;
    dlg.DoModal();
}
```

---

## 文件组织

生成以下文件：
1. DNSSettingsDlg.h / DNSSettingsDlg.cpp
2. DomainRulesDlg.h / DomainRulesDlg.cpp
3. AdvancedDlg.h / AdvancedDlg.cpp
4. AboutDlg.h / AboutDlg.cpp
5. 在 resource.h 中添加所有新的 ID 定义
6. 在 StupidDNS.rc 中添加所有对话框资源

## 代码规范
- Unicode 字符集
- 详细中文注释
- 错误处理
- 数据验证
```

---

## **使用方法**

把这个文档发给 Copilot：
```
@workspace 请根据 SUB_DIALOGS_REQUIREMENTS.md 生成所有子对话框的完整代码，包括：
1. 所有 .h 和 .cpp 文件
2. resource.h 中的新 ID 定义
3. .rc 文件中的对话框资源定义
4. 主对话框中连接子对话框的代码

要求可以直接编译运行。