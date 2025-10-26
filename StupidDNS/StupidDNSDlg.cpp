// StupidDNSDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "StupidDNS.h"
#include "StupidDNSDlg.h"
#include "afxdialogex.h"
#include <thread> // 添加线程支持

// 包含子对话框头文件
#include "DNSSettingsDlg.h"
#include "AdvancedDlg.h"
#include "AboutDlg.h"
#include "DNSCacheHelper.h" // 添加DNS缓存辅助类

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CStupidDNSDlg 对话框

CStupidDNSDlg::CStupidDNSDlg(CWnd* pParent /*=nullptr*/)
 : CDialogEx(IDD_STUPIDDNS_DIALOG, pParent)
 , m_bServiceRunning(FALSE)
 , m_nPort(53)
 , m_dwStartTime(0)
 , m_nTotalQueries(0)
 , m_nCacheHits(0)
, m_nDomesticQueries(0)
 , m_nForeignQueries(0)
{
 m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


void CStupidDNSDlg::DoDataExchange(CDataExchange* pDX)
{
 CDialogEx::DoDataExchange(pDX);
 // 在 DoDataExchange 中绑定控件变量
 DDX_Control(pDX, IDC_LIST_LOG, m_listLog);
}


BEGIN_MESSAGE_MAP(CStupidDNSDlg, CDialogEx)
 ON_WM_SYSCOMMAND()
 ON_WM_PAINT()
 ON_WM_QUERYDRAGICON()
 ON_WM_TIMER()
 ON_BN_CLICKED(IDC_BTN_START, &CStupidDNSDlg::OnBnClickedStart)
 ON_BN_CLICKED(IDC_BTN_STOP, &CStupidDNSDlg::OnBnClickedStop)
 ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CStupidDNSDlg::OnBnClickedClearLog)
 ON_BN_CLICKED(IDC_BTN_EXPORT_LOG, &CStupidDNSDlg::OnBnClickedExportLog)
 ON_BN_CLICKED(IDC_BTN_DNS_SETTINGS, &CStupidDNSDlg::OnBnClickedDNSSettings)
 ON_BN_CLICKED(IDC_BTN_ADVANCED, &CStupidDNSDlg::OnBnClickedAdvanced)
 ON_BN_CLICKED(IDC_BTN_ABOUT, &CStupidDNSDlg::OnBnClickedAbout)
 ON_BN_CLICKED(IDC_BTN_SET_SYSTEM_DNS, &CStupidDNSDlg::OnBnClickedSetSystemDns)
 ON_MESSAGE(WM_USER +100, &CStupidDNSDlg::OnDNSLogMessage)
 ON_MESSAGE(WM_USER +101, &CStupidDNSDlg::OnUpdateDNSButtonText) // 添加新消息
END_MESSAGE_MAP()


// CStupidDNSDlg 消息处理程序

BOOL CStupidDNSDlg::OnInitDialog()
{
 try {
 // 调用基类 OnInitDialog()
 CDialogEx::OnInitDialog();

 // 将"关于..."菜单项添加到系统菜单中。
 ASSERT((IDM_ABOUTBOX &0xFFF0) == IDM_ABOUTBOX);
 ASSERT(IDM_ABOUTBOX <0xF000);

 CMenu* pSysMenu = GetSystemMenu(FALSE);
 if (pSysMenu != nullptr)
 {
 BOOL bNameValid;
 CString strAboutMenu;
 bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
 ASSERT(bNameValid);
 if (!strAboutMenu.IsEmpty())
 {
 pSysMenu->AppendMenu(MF_SEPARATOR);
 pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
 }
 }

 // 设置此对话框的图标
 SetIcon(m_hIcon, TRUE); // 设置大图标
 SetIcon(m_hIcon, FALSE); // 设置小图标

 // 初始化成员变量（m_bServiceRunning=FALSE, m_nPort=53,统计数据=0）
 m_bServiceRunning = FALSE;
 m_nPort =53;
 m_dwStartTime =0;
 m_nTotalQueries =0;
 m_nCacheHits =0;
 m_nDomesticQueries =0;
 m_nForeignQueries =0;

 // 设置端口编辑框默认值53
 SetDlgItemInt(IDC_EDIT_PORT,53);

 // 调用 InitLogList() 初始化日志列表
 InitLogList();

 // 设置按钮初始状态：启动=启用，停止=禁用
 GetDlgItem(IDC_BTN_START)->EnableWindow(TRUE);
 GetDlgItem(IDC_BTN_STOP)->EnableWindow(FALSE);

 // 启动定时器 SetTimer(1,1000, NULL)
 SetTimer(1,1000, NULL);

 TRACE(_T("[OnInitDialog] 开始初始化DNS模块...\n"));
 
 // 初始化DNS服务器（使用try-catch保护）
 try {
 InitializeDNSServers();
 } catch (...) {
 TRACE(_T("[OnInitDialog] InitializeDNSServers异常\n"));
 AfxMessageBox(_T("DNS服务器初始化失败，将使用默认配置"), MB_OK | MB_ICONWARNING);
 }
 
 // 初始化域名分类器（使用try-catch保护）
 try {
 InitializeDomainClassifier();
 } catch (...) {
 TRACE(_T("[OnInitDialog] InitializeDomainClassifier异常\n"));
 AfxMessageBox(_T("域名分类器初始化失败，将使用默认规则"), MB_OK | MB_ICONWARNING);
 }
 
 // 更新系统DNS按钮文本
 try {
 UpdateSystemDNSButtonText();
 } catch (...) {
 TRACE(_T("[OnInitDialog] UpdateSystemDNSButtonText异常\n"));
 }

 TRACE(_T("[OnInitDialog] 初始化完成\n"));
 
 // 返回 TRUE
 return TRUE; // 除非将焦点设置到控件，否则返回 TRUE

 } catch (const std::exception& e) {
 TRACE(_T("[OnInitDialog] 标准异常: %S\n"), e.what());
 AfxMessageBox(_T("初始化失败：发生异常"), MB_OK | MB_ICONERROR);
 return FALSE;
 } catch (...) {
 TRACE(_T("[OnInitDialog] 未知异常\n"));
 AfxMessageBox(_T("初始化失败：未知错误"), MB_OK | MB_ICONERROR);
 return FALSE;
 }
}

// 初始化DNS服务器
void CStupidDNSDlg::InitializeDNSServers()
{
 try {
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 
 AddLogEntry(timeStr, _T("正在检查DNS配置..."), _T("INFO"), _T("-"), _T("-"));

 // 将分类器的污染IP规则下发给测速器，避免被污染IP的上游被选中
 m_dnsManager.SetPoisonIPFilter([this](const CString& ip) {
 return m_domainClassifier.IsPoisonedIP(ip);
 });

 //先尝试读取本地缓存配置
 BOOL hasCache = m_dnsManager.LoadConfig();
 BOOL cacheValid = hasCache && !m_dnsManager.IsConfigExpired();

 if (cacheValid) {
 AddLogEntry(CTime::GetCurrentTime().Format(_T("%H:%M:%S")), _T("已加载本地DNS缓存配置"), _T("INFO"), _T("-"), _T("-"));
 } else {
 // 无缓存或已过期 -> 执行测速并自动配置
 AddLogEntry(CTime::GetCurrentTime().Format(_T("%H:%M:%S")), _T("未找到有效缓存，开始测速与自动配置..."), _T("INFO"), _T("-"), _T("-"));
 // 同步测速，日志通过回调实时输出到列表
 m_dnsManager.PerformSpeedTest([this](CString status){ this->ShowSpeedTestProgress(status); });
 AddLogEntry(CTime::GetCurrentTime().Format(_T("%H:%M:%S")), _T("测速完成，已自动选择最快DNS并写入缓存"), _T("INFO"), _T("-"), _T("-"));
 }
 
 // 显示当前使用的DNS（不论来自缓存还是测速）
 DNSServerInfo domesticDNS = m_dnsManager.GetFastestDomesticDNS();
 if (!domesticDNS.ip.IsEmpty()) {
 CString msg;
 msg.Format(_T("国内DNS: %s (%s)"), 
 domesticDNS.name, domesticDNS.ip);
 
 AddLogEntry(CTime::GetCurrentTime().Format(_T("%H:%M:%S")), msg, _T("INFO"), _T("-"), _T("-"));
 } else {
 TRACE(_T("[初始化] 警告：国内DNS列表为空\n"));
 AddLogEntry(CTime::GetCurrentTime().Format(_T("%H:%M:%S")), _T("警告：国内DNS为空，将在查询时使用内置兜底"), _T("WARN"), _T("-"), _T("-"));
 }
 
 auto overseasDNS = m_dnsManager.GetFastestOverseasDNS();
 if (!overseasDNS.empty()) {
 CString msg;
 msg.Format(_T("海外DNS: %s (%s)"), 
 overseasDNS[0].name, overseasDNS[0].ip);
 
 AddLogEntry(CTime::GetCurrentTime().Format(_T("%H:%M:%S")), msg, _T("INFO"), _T("-"), _T("-"));
 } else {
 TRACE(_T("[初始化] 警告：海外DNS列表为空\n"));
 AddLogEntry(CTime::GetCurrentTime().Format(_T("%H:%M:%S")), _T("警告：海外DNS为空，将在查询时使用内置兜底"), _T("WARN"), _T("-"), _T("-"));
 }
 
 // 添加初始化完成日志
 AddLogEntry(CTime::GetCurrentTime().Format(_T("%H:%M:%S")), _T("DNS服务器初始化完成"), _T("INFO"), _T("-"), _T("-"));
 
 } catch (...) {
 TRACE(_T("[初始化] DNS服务器初始化异常\n"));
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, _T("DNS服务器初始化失败"), _T("ERROR"), _T("-"), _T("-"));
 }
}

// 初始化域名分类器
void CStupidDNSDlg::InitializeDomainClassifier()
{
 try {
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));

 AddLogEntry(timeStr, _T("正在加载域名规则..."), _T("INFO"), _T("-"), _T("-"));

 // 初始化域名分类器（加载规则文件）
 // 即使失败也继续，使用内置规则
 BOOL initSuccess = m_domainClassifier.Initialize();
 if (!initSuccess) {
 TRACE(_T("[初始化] 域名分类器初始化失败，将使用内置规则\n"));
 }
 
 // 初始化查询路由器
 if (!m_queryRouter.Initialize(&m_dnsManager, &m_domainClassifier)) {
 TRACE(_T("[初始化] DNS查询路由器初始化失败\n"));
 AddLogEntry(timeStr, _T("错误: DNS查询路由器初始化失败"), _T("ERROR"), _T("-"), _T("-"));
 return;
 }
 
 // 显示加载的规则数量
 CString msg;
 msg.Format(_T("规则加载完成: 中国域名 %d 条, GFW域名 %d 条, 污染IP %d 个"),
 m_domainClassifier.GetRuleLoader().GetChinaDomainCount(),
 m_domainClassifier.GetRuleLoader().GetGFWDomainCount(),
 m_domainClassifier.GetRuleLoader().GetPoisonIPCount());
 
 timeStr = CTime::GetCurrentTime().Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, msg, _T("INFO"), _T("-"), _T("-"));
 
 AddLogEntry(timeStr, _T("域名分类器初始化完成"), _T("INFO"), _T("-"), _T("-"));
 
 } catch (...) {
 TRACE(_T("[初始化] 域名分类器初始化异常\n"));
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, _T("域名分类器初始化失败（将使用默认规则）"), _T("ERROR"), _T("-"), _T("-"));
 }
}

void CStupidDNSDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
 if ((nID &0xFFF0) == IDM_ABOUTBOX)
 {
 CAboutDlg dlgAbout;
 dlgAbout.DoModal();
 }
 else
 {
 CDialogEx::OnSysCommand(nID, lParam);
 }
}

void CStupidDNSDlg::OnPaint()
{
 if (IsIconic())
 {
 CPaintDC dc(this); // 用于绘制的设备上下文

 SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()),0);

 //使图标在工作区矩形中居中
 int cxIcon = GetSystemMetrics(SM_CXICON);
 int cyIcon = GetSystemMetrics(SM_CYICON);
 CRect rect;
 GetClientRect(&rect);
 int x = (rect.Width() - cxIcon +1) /2;
 int y = (rect.Height() - cyIcon +1) /2;

 // 绘制图标
 dc.DrawIcon(x, y, m_hIcon);
 }
 else
{
 CDialogEx::OnPaint();
 }
}

HCURSOR CStupidDNSDlg::OnQueryDragIcon()
{
 return static_cast<HCURSOR>(m_hIcon);
}

// 初始化日志列表
void CStupidDNSDlg::InitLogList()
{
 // 设置扩展样式：LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES
 m_listLog.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

 // 插入列：时间(70), 域名(180), 类型(50),结果IP(100), 耗时(50),运行时长(70)
 m_listLog.InsertColumn(0, _T("时间"), LVCFMT_LEFT,70);
m_listLog.InsertColumn(1, _T("域名/事件"), LVCFMT_LEFT,180);
 m_listLog.InsertColumn(2, _T("类型"), LVCFMT_LEFT,50);
 m_listLog.InsertColumn(3, _T("结果IP"), LVCFMT_LEFT,100);
 m_listLog.InsertColumn(4, _T("耗时"), LVCFMT_LEFT,50);
 
 // 插入统计信息行（置顶，使用不同背景色标识）
 int statsIndex = m_listLog.InsertItem(0, _T("统计"));
 m_listLog.SetItemText(statsIndex,1, _T("总查询:0 国内:0 国外:0"));
 m_listLog.SetItemText(statsIndex,2, _T("缓存:0%"));
 m_listLog.SetItemText(statsIndex,3, _T("平均:0ms"));
 m_listLog.SetItemText(statsIndex,4, _T("运行:00:00:00"));
 
 // 设置统计行为粗体（可选，需要自定义绘制）
 //这里简单地用固定行表示，实际可以用LVN_GETDISPINFO自定义
}

// 添加日志条目
void CStupidDNSDlg::AddLogEntry(CString time, CString domain, CString type, CString result, CString latency)
{
 // 在列表第1行插入新项（统计行在第0行）
 int index = m_listLog.InsertItem(1, time);

 // 设置各列文本
 m_listLog.SetItemText(index,1, domain);
 m_listLog.SetItemText(index,2, type);
 m_listLog.SetItemText(index,3, result);
 m_listLog.SetItemText(index,4, latency);

 // 检查列表项数，超过1001 则删除最后一项（保留统计行）
 if (m_listLog.GetItemCount() >1001)
 {
 m_listLog.DeleteItem(1001);
 }
}

// 显示测速进度
void CStupidDNSDlg::ShowSpeedTestProgress(CString status)
{
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, status, _T("INFO"), _T("-"), _T("-"));
 
 // 强制更新界面
 UpdateData(FALSE);
 MSG msg;
 while (PeekMessage(&msg, NULL,0,0, PM_REMOVE)) {
 TranslateMessage(&msg);
 DispatchMessage(&msg);
 }
}

// 启动服务按钮点击事件
void CStupidDNSDlg::OnBnClickedStart()
{
 // 从端口编辑框读取端口号到 m_nPort
 m_nPort = GetDlgItemInt(IDC_EDIT_PORT);
 
 
 // 初始化DNS服务器
 if (!m_dnsServer.Initialize(m_nPort, &m_dnsManager, &m_domainClassifier)) {
 AfxMessageBox(_T("DNS服务器初始化失败！"), MB_OK | MB_ICONERROR);
 return;
 }
 
 // 设置日志回调
 m_dnsServer.SetLogCallback([this](CString domain, CString type, CString result, CString ip, int latency) {
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 
 CString latencyStr;
 latencyStr.Format(_T("%dms"), latency);
 
 // 使用PostMessage确保线程安全
 ::PostMessage(m_hWnd, WM_USER +100,0, (LPARAM)new CString(
 timeStr + _T("|") + domain + _T("|") + type + _T("|") + result + _T("|") + ip + _T("|") + latencyStr
));
 });

 // 启动DNS服务器
 if (!m_dnsServer.Start()) {
 CString errorMsg;
 errorMsg.Format(_T("无法绑定端口 %d！\n\n可能原因：\n1.端口已被占用\n2.需要管理员权限"), m_nPort);
 AfxMessageBox(errorMsg, MB_OK | MB_ICONERROR);
 return;
 }

 // ===== 清除DNS缓存 =====
 TRACE(_T("[启动服务] 正在清除DNS缓存...\n"));
 if (CDNSCacheHelper::FlushDNSCache()) {
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, _T("已清除DNS缓存"), _T("INFO"), _T("-"), _T("-"));
} else {
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, _T("警告：清除DNS缓存失败（可能需要管理员权限）"), _T("WARN"), _T("-"), _T("-"));
 }
 
 // 设置 m_bServiceRunning = TRUE
 m_bServiceRunning = TRUE;

 //记录启动时间：m_dwStartTime = GetTickCount()
m_dwStartTime = GetTickCount();

 // 更新按钮状态：启动=禁用，停止=启用，端口=禁用
 GetDlgItem(IDC_BTN_START)->EnableWindow(FALSE);
 GetDlgItem(IDC_BTN_STOP)->EnableWindow(TRUE);
 GetDlgItem(IDC_EDIT_PORT)->EnableWindow(FALSE);

 // 更新状态文本为 "●运行中"
SetDlgItemText(IDC_STATIC_STATUS, _T("●运行中"));

 // 调用 AddLogEntry 添加 "DNS服务已启动" 日志（类型=INFO）
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 CString msg;
 msg.Format(_T("DNS服务已启动 (端口:%d)"), m_nPort);
 AddLogEntry(timeStr, msg, _T("INFO"), _T("-"), _T("-"));
 
 // **如果使用非53端口，提示用户如何测试**
 if (m_nPort !=53) {
 CString portMsg;
portMsg.Format(
 _T("✓ DNS服务已在端口 %d 启动成功！\n\n")
 _T("测试方法：\n")
 _T("1. 使用dig命令：\n")
 _T(" dig @127.0.0.1 -p %d www.baidu.com\n\n")
 _T("2. 或使用Python：\n")
 _T(" import dns.resolver\n")
 _T(" resolver = dns.resolver.Resolver()\n")
 _T(" resolver.nameservers = ['127.0.0.1']\n")
 _T(" resolver.port = %d\n")
 _T(" resolver.query('www.baidu.com')\n\n")
 _T("注意：nslookup无法指定端口，不能直接测试。"),
 m_nPort, m_nPort, m_nPort);
 
 AfxMessageBox(portMsg, MB_OK | MB_ICONINFORMATION);
 }
}

// 停止服务按钮点击事件
void CStupidDNSDlg::OnBnClickedStop()
{
 // 停止DNS服务器
 m_dnsServer.Stop();

 // 设置 m_bServiceRunning = FALSE
 m_bServiceRunning = FALSE;

 // 更新按钮状态：启动=启用，停止=禁用，端口=启用
 GetDlgItem(IDC_BTN_START)->EnableWindow(TRUE);
 GetDlgItem(IDC_BTN_STOP)->EnableWindow(FALSE);
 GetDlgItem(IDC_EDIT_PORT)->EnableWindow(TRUE);

 // 更新状态文本为 "● 已停止"
SetDlgItemText(IDC_STATIC_STATUS, _T("● 已停止"));

 // 添加日志 "DNS服务已停止"
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, _T("DNS服务已停止"), _T("INFO"), _T("-"), _T("-"));
}

// 定时器消息处理
void CStupidDNSDlg::OnTimer(UINT_PTR nIDEvent)
{
 // 检查 nIDEvent ==1 且 m_bServiceRunning == TRUE
 if (nIDEvent ==1 && m_bServiceRunning == TRUE)
 {
 //计算运行时长：(GetTickCount() - m_dwStartTime) /1000
 DWORD elapsedSeconds = (GetTickCount() - m_dwStartTime) /1000;

 // 格式化为 HH:MM:SS
 int hours = elapsedSeconds /3600;
 int minutes = (elapsedSeconds %3600) /60;
 int seconds = elapsedSeconds %60;

 CString runtimeStr;
 runtimeStr.Format(_T("%02d:%02d:%02d"), hours, minutes, seconds);

 // 更新统计数据（从DNS服务器获取）
//统计数据将在 OnDNSLogMessage 中更新
 }

 CDialogEx::OnTimer(nIDEvent);
}

// 更新统计信息显示（更新列表第一行）
void CStupidDNSDlg::UpdateStatistics()
{
 if (m_listLog.GetItemCount() ==0) return;
 
//计算缓存命中率
 int cacheRate =0;
 if (m_nTotalQueries >0) {
 cacheRate = (m_nCacheHits *100) / m_nTotalQueries;
 }
 
 //计算运行时长
DWORD elapsedSeconds = m_bServiceRunning ? (GetTickCount() - m_dwStartTime) /1000 :0;
 int hours = elapsedSeconds /3600;
 int minutes = (elapsedSeconds %3600) /60;
 int seconds = elapsedSeconds %60;
 CString runtimeStr;
runtimeStr.Format(_T("%02d:%02d:%02d"), hours, minutes, seconds);
 
 // 更新统计行（第0行）
 CString statsInfo;
 statsInfo.Format(_T("总查询:%d 国内:%d 国外:%d"), 
 m_nTotalQueries, m_nDomesticQueries, m_nForeignQueries);
 
 CString cacheStr;
 cacheStr.Format(_T("缓存:%d%%"), cacheRate);
 
 m_listLog.SetItemText(0,0, _T("【统计】"));
 m_listLog.SetItemText(0,1, statsInfo);
 m_listLog.SetItemText(0,2, cacheStr);
 m_listLog.SetItemText(0,3, _T("平均:0ms")); // 可以后续计算平均响应时间
 m_listLog.SetItemText(0,4, runtimeStr);
}

// 清除日志按钮点击事件
void CStupidDNSDlg::OnBnClickedClearLog()
{
 m_listLog.DeleteAllItems();
}

// 导出日志按钮点击事件
void CStupidDNSDlg::OnBnClickedExportLog()
{
 // 弹出文件保存对话框
 CFileDialog fileDlg(FALSE, _T("txt"), _T("DNSLog_*.txt"), 
 OFN_OVERWRITEPROMPT, _T("文本文件 (*.txt)|*.txt||"), this);
 if (fileDlg.DoModal() == IDOK) {
 CString filePath = fileDlg.GetPathName();

 // 打开文件
 CStdioFile file;
 if (!file.Open(filePath, CFile::modeCreate | CFile::modeWrite | CFile::typeText)) {
 AfxMessageBox(_T("无法打开文件进行写入"), MB_OK | MB_ICONERROR);
 return;
 }

 // 写入日志
 for (int i =0; i < m_listLog.GetItemCount(); ++i) {
 CString line;
 for (int j =0; j < m_listLog.GetHeaderCtrl()->GetItemCount(); ++j) {
 if (j >0) line += _T("\t"); // 制表符分隔
 line += m_listLog.GetItemText(i, j);
 }
 file.WriteString(line + _T("\n"));
 }

 file.Close();

 AfxMessageBox(_T("日志已成功导出到文件"), MB_OK | MB_ICONINFORMATION);
 }
}

// DNS设置按钮点击事件
void CStupidDNSDlg::OnBnClickedDNSSettings()
{
 // 显示DNS服务器管理对话框
 CDNSSettingsDlg dlg;
 dlg.DoModal();
}

// 高级选项按钮点击事件
void CStupidDNSDlg::OnBnClickedAdvanced()
{
 // 显示高级选项对话框
 CAdvancedDlg dlg;
 dlg.DoModal();
}

// 关于按钮点击事件
void CStupidDNSDlg::OnBnClickedAbout()
{
 // 创建 About 对话框的实例
 CAboutDlg aboutDlg;
 
 // 显示 About 对话框
 aboutDlg.DoModal();
}

// 设置系统DNS按钮点击事件
void CStupidDNSDlg::OnBnClickedSetSystemDns()
{
 // **立即更新按钮文本，避免后续调用IsSetToLocalhost导致卡顿**
 //先假设当前状态，点击后立即切换按钮文本
 CString currentButtonText;
 GetDlgItemText(IDC_BTN_SET_SYSTEM_DNS, currentButtonText);
 BOOL isCurrentlyLocalhost = (currentButtonText == _T("恢复自动DNS"));

 // 如果已经是127.0.0.1，则恢复为自动；否则设置为127.0.0.1
 if (isCurrentlyLocalhost) {
 // 恢复为自动获取
 int result = AfxMessageBox(_T("当前系统DNS已设为127.0.0.1\n\n确定要恢复为自动获取DNS吗？"), 
 MB_YESNO | MB_ICONQUESTION);
 
 if (result != IDYES) {
 return;
}

 TRACE(_T("[OnBnClickedSetSystemDns] 开始恢复DNS设置...\n"));
 BOOL success = CSystemDNSHelper::SetAllAdaptersDNS(FALSE);
 TRACE(_T("[OnBnClickedSetSystemDns] SetAllAdaptersDNS返回: %d\n"), success);
 
 if (success) {
 AfxMessageBox(_T("已恢复为自动获取DNS设置。"), MB_OK | MB_ICONINFORMATION);
 
 // 添加日志
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, _T("系统DNS已恢复为自动获取"), _T("INFO"), _T("-"), _T("-"));
 
 //立即更新按钮文本（不调用IsSetToLocalhost）
 SetDlgItemText(IDC_BTN_SET_SYSTEM_DNS, _T("设为系统DNS"));
 } else {
 TRACE(_T("[OnBnClickedSetSystemDns] 恢复DNS失败\n"));
 AfxMessageBox(_T("DNS设置失败！\n\n可能原因：\n1. 没有管理员权限\n2. 网络适配器不可用\n3. 系统限制"), 
 MB_OK | MB_ICONERROR);
 }
 } else {
 // 设置为127.0.0.1
 // 检查是否有管理员权限
 BOOL isAdmin = IsUserAnAdmin();
 
 if (!isAdmin) {
 AfxMessageBox(_T("需要管理员权限才能修改系统DNS设置！\n\n请右键点击程序，选择【以管理员身份运行】。"), 
 MB_OK | MB_ICONWARNING);
 return;
 }
 
 int result = AfxMessageBox(_T("将系统DNS设置为127.0.0.1\n\n设置后，请启动DNS服务，浏览器将通过本程序解析域名。\n\n确定继续吗？"), 
 MB_YESNO | MB_ICONQUESTION);

 if (result != IDYES) {
 return;
 }
 
 TRACE(_T("[OnBnClickedSetSystemDns] 开始设置DNS为127.0.0.1...\n"));
 BOOL success = CSystemDNSHelper::SetAllAdaptersDNS(TRUE);
 TRACE(_T("[OnBnClickedSetSystemDns] SetAllAdaptersDNS返回: %d\n"), success);
 
 if (success) {
 // ===== 清除DNS缓存 =====
 TRACE(_T("[设置系统DNS] 正在清除DNS缓存...\n"));
CDNSCacheHelper::FlushDNSCache();
 
 CString successMsg = _T("DNS设置成功！\n\n已将系统DNS设为:127.0.0.1\n\n");
 successMsg += _T("DNS缓存已清除，现在可以点击【启动服务】按钮。\n");
 successMsg += _T("浏览器将通过本程序进行DNS解析。");

 AfxMessageBox(successMsg, MB_OK | MB_ICONINFORMATION);

 // 添加日志
 CTime currentTime = CTime::GetCurrentTime();
 CString timeStr = currentTime.Format(_T("%H:%M:%S"));
 AddLogEntry(timeStr, _T("系统DNS已设为:127.0.0.1，DNS缓存已清除"), _T("INFO"), _T("-"), _T("-"));
 
 //立即更新按钮文本（不调用IsSetToLocalhost）
 SetDlgItemText(IDC_BTN_SET_SYSTEM_DNS, _T("恢复自动DNS"));
 } else {
 TRACE(_T("[OnBnClickedSetSystemDns] 设置DNS失败\n"));
 AfxMessageBox(_T("DNS设置失败！\n\n可能原因：\n1. 没有管理员权限\n2. 网络适配器不可用\n3. 系统限制"), 
 MB_OK | MB_ICONERROR);
}
 }
}

// 更新系统DNS按钮文本（仅在启动时调用一次）
void CStupidDNSDlg::UpdateSystemDNSButtonText()
{
 // 在后台线程检查，避免阻塞UI
 std::thread([this]() {
 BOOL isLocalhost = CSystemDNSHelper::IsSetToLocalhost(_T(""));
 HWND h = this->GetSafeHwnd();
 // 使用PostMessage更新UI
 ::PostMessage(h, WM_USER +101, isLocalhost,0);
 }).detach();
}

//处理DNS日志消息（来自服务器线程）
LRESULT CStupidDNSDlg::OnDNSLogMessage(WPARAM wParam, LPARAM lParam)
{
 CString* pLogData = (CString*)lParam;
 
 if (pLogData) {
 //解析日志数据: 时间|域名|类型|结果状态|IP|延迟
 int pos =0;
 CString time = pLogData->Tokenize(_T("|"), pos);
 CString domain = pLogData->Tokenize(_T("|"), pos);
 CString type = pLogData->Tokenize(_T("|"), pos);
 CString resultStatus = pLogData->Tokenize(_T("|"), pos); // "成功"或"失败"
 CString ip = pLogData->Tokenize(_T("|"), pos);
 CString latency = pLogData->Tokenize(_T("|"), pos);
 
 // 添加到日志列表：显示IP地址而不是结果状态
 // AddLogEntry参数: 时间, 域名, 类型,结果IP, 延迟
 AddLogEntry(time, domain, type, ip, latency);
 
 delete pLogData;
 }
 
 return 0;
}

//处理DNS按钮文本更新消息（来自后台线程）
LRESULT CStupidDNSDlg::OnUpdateDNSButtonText(WPARAM wParam, LPARAM lParam)
{
 BOOL isLocalhost = (BOOL)wParam;
 
 if (isLocalhost) {
 SetDlgItemText(IDC_BTN_SET_SYSTEM_DNS, _T("恢复自动DNS"));
 } else {
 SetDlgItemText(IDC_BTN_SET_SYSTEM_DNS, _T("设为系统DNS"));
 }
 
 return 0;
}

