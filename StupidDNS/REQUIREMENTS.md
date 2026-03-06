\# StupidDNS 界面实现需求



\## 项目说明

这是一个 MFC 对话框应用程序，用于智能 DNS 解析。



\## 需要修改/创建的文件



\### 1. resource.h

定义以下资源 ID（从 1001 开始）：

\- IDD\_STUPIDDNS\_DIALOG (主对话框)

\- IDC\_BTN\_START (启动按钮)

\- IDC\_BTN\_STOP (停止按钮)

\- IDC\_EDIT\_PORT (端口输入框)

\- IDC\_STATIC\_STATUS (状态显示)

\- IDC\_LIST\_LOG (日志列表)

\- IDC\_BTN\_CLEAR\_LOG (清空日志按钮)

\- IDC\_BTN\_EXPORT\_LOG (导出日志按钮)

\- IDC\_BTN\_DNS\_SETTINGS (DNS设置按钮)

\- IDC\_BTN\_DOMAIN\_RULES (域名规则按钮)

\- IDC\_BTN\_ADVANCED (高级选项按钮)

\- IDC\_BTN\_ABOUT (关于按钮)

\- IDC\_STATIC\_TOTAL (总查询数显示)

\- IDC\_STATIC\_CACHE\_RATE (缓存命中率显示)

\- IDC\_STATIC\_DOMESTIC (国内查询数显示)

\- IDC\_STATIC\_FOREIGN (国外查询数显示)

\- IDC\_STATIC\_RUNTIME (运行时长显示)

\- IDC\_STATIC\_AVG\_RESPONSE (平均响应时间显示)



\### 2. StupidDNS.rc

创建主对话框资源，尺寸 520x450 像素，包含以下控件：



\#### 服务状态区域 (GroupBox at 10,10,500,50)

\- GroupBox "服务状态"

\- Static Text "服务状态:" at (20,25)

\- Static Text "已停止" (IDC\_STATIC\_STATUS) at (80,25,100,12) - 动态更新

\- PushButton "启动服务" (IDC\_BTN\_START) at (200,22,80,20)

\- PushButton "停止服务" (IDC\_BTN\_STOP) at (290,22,80,20) - 初始禁用

\- Static Text "监听端口:" at (20,40)

\- EditText (IDC\_EDIT\_PORT) at (80,38,50,14) - ES\_NUMBER 样式，默认 "53"

\- Static Text "本地IP: 127.0.0.1" at (150,40)



\#### 统计信息区域 (GroupBox at 10,70,500,50)

\- GroupBox "统计信息"

\- Static "总查询数:" at (20,85) + Static (IDC\_STATIC\_TOTAL) "0" at (80,85)

\- Static "缓存命中率:" at (160,85) + Static (IDC\_STATIC\_CACHE\_RATE) "0%" at (230,85)

\- Static "国内查询:" at (20,100) + Static (IDC\_STATIC\_DOMESTIC) "0" at (80,100)

\- Static "国外查询:" at (160,100) + Static (IDC\_STATIC\_FOREIGN) "0" at (230,100)

\- Static "运行时长:" at (310,85) + Static (IDC\_STATIC\_RUNTIME) "00:00:00" at (370,85)

\- Static "平均响应:" at (310,100) + Static (IDC\_STATIC\_AVG\_RESPONSE) "0ms" at (370,100)



\#### 查询日志区域 (GroupBox at 10,130,500,260)

\- GroupBox "查询日志"

\- ListControl (IDC\_LIST\_LOG) at (20,145,480,215)

&nbsp; 样式: LVS\_REPORT | LVS\_SINGLESEL | WS\_BORDER | WS\_TABSTOP

\- PushButton "清空日志" (IDC\_BTN\_CLEAR\_LOG) at (20,365,70,20)

\- PushButton "导出日志" (IDC\_BTN\_EXPORT\_LOG) at (100,365,70,20)



\#### 底部按钮区域

\- PushButton "DNS服务器" (IDC\_BTN\_DNS\_SETTINGS) at (10,400,80,25)

\- PushButton "域名规则" (IDC\_BTN\_DOMAIN\_RULES) at (100,400,80,25)

\- PushButton "高级选项" (IDC\_BTN\_ADVANCED) at (190,400,80,25)

\- PushButton "关于" (IDC\_BTN\_ABOUT) at (430,400,80,25)



\### 3. StupidDNSDlg.h

类定义包含：



成员变量：

```cpp

CListCtrl m\_listLog;

BOOL m\_bServiceRunning;

int m\_nPort;

DWORD m\_dwStartTime;

int m\_nTotalQueries;

int m\_nCacheHits;

int m\_nDomesticQueries;

int m\_nForeignQueries;

```



消息处理函数：

\- afx\_msg void OnBnClickedStart();

\- afx\_msg void OnBnClickedStop();

\- afx\_msg void OnBnClickedClearLog();

\- afx\_msg void OnBnClickedExportLog();

\- afx\_msg void OnBnClickedDNSSettings();

\- afx\_msg void OnBnClickedDomainRules();

\- afx\_msg void OnBnClickedAdvanced();

\- afx\_msg void OnBnClickedAbout();

\- afx\_msg void OnTimer(UINT\_PTR nIDEvent);



辅助函数：

\- void InitLogList();

\- void AddLogEntry(CString time, CString domain, CString type, CString result, CString latency);

\- void UpdateStatistics();



\### 4. StupidDNSDlg.cpp



\#### OnInitDialog() 实现：

\- 调用基类 OnInitDialog()

\- 初始化成员变量（m\_bServiceRunning=FALSE, m\_nPort=53, 统计数据=0）

\- 设置端口编辑框默认值 53

\- 调用 InitLogList() 初始化日志列表

\- 设置按钮初始状态：启动=启用，停止=禁用

\- 启动定时器 SetTimer(1, 1000, NULL)

\- 返回 TRUE



\#### InitLogList() 实现：

\- 设置扩展样式：LVS\_EX\_FULLROWSELECT | LVS\_EX\_GRIDLINES

\- 插入列：时间(80), 域名(200), 类型(60), 结果IP(120), 耗时(60)



\#### OnBnClickedStart() 实现：

\- 从端口编辑框读取端口号到 m\_nPort

\- 设置 m\_bServiceRunning = TRUE

\- 记录启动时间：m\_dwStartTime = GetTickCount()

\- 更新按钮状态：启动=禁用，停止=启用，端口=禁用

\- 更新状态文本为 "● 运行中"

\- 调用 AddLogEntry 添加 "DNS服务已启动" 日志（类型=INFO）



\#### OnBnClickedStop() 实现：

\- 设置 m\_bServiceRunning = FALSE

\- 更新按钮状态：启动=启用，停止=禁用，端口=启用

\- 更新状态文本为 "○ 已停止"

\- 调用 AddLogEntry 添加 "DNS服务已停止" 日志



\#### OnTimer(UINT\_PTR nIDEvent) 实现：

\- 检查 nIDEvent == 1 且 m\_bServiceRunning == TRUE

\- 计算运行时长：(GetTickCount() - m\_dwStartTime) / 1000

\- 格式化为 HH:MM:SS 并更新 IDC\_STATIC\_RUNTIME

\- 调用 UpdateStatistics()



\#### UpdateStatistics() 实现：

\- 更新总查询数显示

\- 更新国内/国外查询数显示

\- 计算缓存命中率：(m\_nCacheHits \* 100) / m\_nTotalQueries

\- 更新所有统计文本控件



\#### AddLogEntry() 实现：

\- 在列表顶部插入新项（index=0）

\- 设置各列文本

\- 检查列表项数，超过 1000 则删除最后一项



\#### OnBnClickedClearLog() 实现：

\- 弹出确认对话框 AfxMessageBox

\- 如果确认，调用 m\_listLog.DeleteAllItems()



\#### OnBnClickedExportLog() 实现：

\- 创建文件保存对话框（.txt 过滤器）

\- 打开文件，写入表头

\- 遍历列表，写入每行数据（制表符分隔）

\- 关闭文件，显示成功消息



\#### 其他按钮处理函数（暂时）：

\- 显示 "功能开发中" 消息框



\## 代码规范

\- 使用 Unicode 字符集，所有字符串使用 \_T() 宏

\- 添加必要的消息映射宏

\- 在 DoDataExchange 中绑定控件变量

\- 添加详细的中文注释

\- 包含必要的头文件（afxwin.h, afxcmn.h）



\## 生成要求

请直接生成完整的文件内容，包括：

1\. resource.h 完整内容

2\. StupidDNS.rc 中对话框资源定义

3\. StupidDNSDlg.h 完整内容

4\. StupidDNSDlg.cpp 完整内容



文件格式正确，可以直接编译运行。





