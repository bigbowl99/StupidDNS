// StupidDNSDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "StupidDNS.h"
#include "StupidDNSDlg.h"
#include "afxdialogex.h"

// 包含子对话框头文件
#include "DNSSettingsDlg.h"
#include "AdvancedDlg.h"
#include "AboutDlg.h"

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
    ON_MESSAGE(WM_USER + 100, &CStupidDNSDlg::OnDNSLogMessage)
END_MESSAGE_MAP()


// CStupidDNSDlg 消息处理程序

BOOL CStupidDNSDlg::OnInitDialog()
{
    // 调用基类 OnInitDialog()
    CDialogEx::OnInitDialog();

    // 将"关于..."菜单项添加到系统菜单中。
ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

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
    SetIcon(m_hIcon, TRUE);     // 设置大图标
    SetIcon(m_hIcon, FALSE);    // 设置小图标

    // 初始化成员变量（m_bServiceRunning=FALSE, m_nPort=53, 统计数据=0）
    m_bServiceRunning = FALSE;
    m_nPort = 53;
    m_dwStartTime = 0;
 m_nTotalQueries = 0;
    m_nCacheHits = 0;
    m_nDomesticQueries = 0;
    m_nForeignQueries = 0;

    // 设置端口编辑框默认值 53
    SetDlgItemInt(IDC_EDIT_PORT, 53);

    // 调用 InitLogList() 初始化日志列表
    InitLogList();

    // 设置按钮初始状态：启动=启用，停止=禁用
    GetDlgItem(IDC_BTN_START)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(FALSE);

  // 启动定时器 SetTimer(1, 1000, NULL)
    SetTimer(1, 1000, NULL);

    // 初始化DNS服务器（新增）
 InitializeDNSServers();
  
    // 初始化域名分类器（新增）
    InitializeDomainClassifier();
 
    // 更新系统DNS按钮文本
    UpdateSystemDNSButtonText();

    // 返回 TRUE
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 初始化DNS服务器
void CStupidDNSDlg::InitializeDNSServers()
{
    CTime currentTime = CTime::GetCurrentTime();
    CString timeStr = currentTime.Format(_T("%H:%M:%S"));
    
    AddLogEntry(timeStr, _T("正在初始化DNS服务器..."), _T("INFO"), _T("-"), _T("-"));
    
    // 加载配置或执行测速
    if (!m_dnsManager.LoadConfig() || m_dnsManager.IsConfigExpired()) {
        // 首次运行或配置过期，执行测速
        AddLogEntry(timeStr, _T("首次运行或配置过期，开始测速..."), _T("INFO"), _T("-"), _T("-"));
  
        // 执行测速（带进度回调）
        m_dnsManager.PerformSpeedTest([this](CString status) {
            ShowSpeedTestProgress(status);
        });
    } else {
        AddLogEntry(timeStr, _T("从配置文件加载DNS设置"), _T("INFO"), _T("-"), _T("-"));
    }
    
    // 显示当前使用的DNS
    DNSServerInfo domesticDNS = m_dnsManager.GetFastestDomesticDNS();
    if (!domesticDNS.ip.IsEmpty()) {
        CString msg;
        msg.Format(_T("国内DNS: %s (%s) - %dms"), 
            domesticDNS.name, domesticDNS.ip, domesticDNS.latency);
        
     // 格式化耗时
        CString latencyStr;
        latencyStr.Format(_T("%dms"), domesticDNS.latency);

        timeStr = CTime::GetCurrentTime().Format(_T("%H:%M:%S"));
        AddLogEntry(timeStr, msg, _T("INFO"), domesticDNS.ip, latencyStr);
    }
    
    auto overseasDNS = m_dnsManager.GetFastestOverseasDNS();
    for (auto& dns : overseasDNS) {
        CString msg;
  msg.Format(_T("海外DNS: %s (%s) - %dms"), 
     dns.name, dns.ip, dns.latency);
        
        // 格式化耗时
     CString latencyStr;
        latencyStr.Format(_T("%dms"), dns.latency);
    
        timeStr = CTime::GetCurrentTime().Format(_T("%H:%M:%S"));
        AddLogEntry(timeStr, msg, _T("INFO"), dns.ip, latencyStr);
    }
    
    // 添加测速完成日志
    timeStr = CTime::GetCurrentTime().Format(_T("%H:%M:%S"));
    AddLogEntry(timeStr, _T("DNS服务器初始化完成"), _T("INFO"), _T("-"), _T("-"));
}

// 初始化域名分类器
void CStupidDNSDlg::InitializeDomainClassifier()
{
    CTime currentTime = CTime::GetCurrentTime();
    CString timeStr = currentTime.Format(_T("%H:%M:%S"));

    AddLogEntry(timeStr, _T("正在加载域名规则..."), _T("INFO"), _T("-"), _T("-"));

    // 初始化域名分类器（加载规则文件）
    if (!m_domainClassifier.Initialize()) {
    CString msg = _T("警告: 部分规则文件加载失败，将使用有限规则");
     AddLogEntry(timeStr, msg, _T("WARN"), _T("-"), _T("-"));
    }
    
    // 初始化查询路由器
    if (!m_queryRouter.Initialize(&m_dnsManager, &m_domainClassifier)) {
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
}

void CStupidDNSDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
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

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

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

    // 插入列：时间(70), 域名(180), 类型(50), 结果IP(100), 耗时(50), 运行时长(70)
 m_listLog.InsertColumn(0, _T("时间"), LVCFMT_LEFT, 70);
m_listLog.InsertColumn(1, _T("域名/事件"), LVCFMT_LEFT, 180);
    m_listLog.InsertColumn(2, _T("类型"), LVCFMT_LEFT, 50);
 m_listLog.InsertColumn(3, _T("结果IP"), LVCFMT_LEFT, 100);
    m_listLog.InsertColumn(4, _T("耗时"), LVCFMT_LEFT, 50);
    
    // 插入统计信息行（置顶，使用不同背景色标识）
 int statsIndex = m_listLog.InsertItem(0, _T("统计"));
    m_listLog.SetItemText(statsIndex, 1, _T("总查询:0 国内:0 国外:0"));
    m_listLog.SetItemText(statsIndex, 2, _T("缓存:0%"));
    m_listLog.SetItemText(statsIndex, 3, _T("平均:0ms"));
    m_listLog.SetItemText(statsIndex, 4, _T("运行:00:00:00"));
    
    // 设置统计行为粗体（可选，需要自定义绘制）
    // 这里简单地用固定行表示，实际可以用LVN_GETDISPINFO自定义
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
        ::PostMessage(m_hWnd, WM_USER + 100, 0, (LPARAM)new CString(
       timeStr + _T("|") + domain + _T("|") + type + _T("|") + result + _T("|") + ip + _T("|") + latencyStr
        ));
    });
    
    // 启动DNS服务器
    if (!m_dnsServer.Start()) {
        CString errorMsg;
        errorMsg.Format(_T("无法绑定端口 %d！\n\n可能原因：\n1. 端口已被占用\n2. 需要管理员权限"), m_nPort);
    AfxMessageBox(errorMsg, MB_OK | MB_ICONERROR);
        return;
    }

    // 设置 m_bServiceRunning = TRUE
    m_bServiceRunning = TRUE;

    // 记录启动时间：m_dwStartTime = GetTickCount()
    m_dwStartTime = GetTickCount();

    // 更新按钮状态：启动=禁用，停止=启用，端口=禁用
    GetDlgItem(IDC_BTN_START)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(TRUE);
    GetDlgItem(IDC_EDIT_PORT)->EnableWindow(FALSE);

    // 更新状态文本为 "● 运行中"
SetDlgItemText(IDC_STATIC_STATUS, _T("● 运行中"));

    // 调用 AddLogEntry 添加 "DNS服务已启动" 日志（类型=INFO）
    CTime currentTime = CTime::GetCurrentTime();
    CString timeStr = currentTime.Format(_T("%H:%M:%S"));
    CString msg;
    msg.Format(_T("DNS服务已启动 (端口:%d)"), m_nPort);
  AddLogEntry(timeStr, msg, _T("INFO"), _T("-"), _T("-"));
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

    // 更新状态文本为 "○ 已停止"
    SetDlgItemText(IDC_STATIC_STATUS, _T("○ 已停止"));

    // 调用 AddLogEntry 添加 "DNS服务已停止" 日志
    CTime currentTime = CTime::GetCurrentTime();
    CString timeStr = currentTime.Format(_T("%H:%M:%S"));
    AddLogEntry(timeStr, _T("DNS服务已停止"), _T("INFO"), _T("-"), _T("-"));
}

// 定时器消息处理
void CStupidDNSDlg::OnTimer(UINT_PTR nIDEvent)
{
    // 检查 nIDEvent == 1 且 m_bServiceRunning == TRUE
    if (nIDEvent == 1 && m_bServiceRunning == TRUE)
    {
        // 计算运行时长：(GetTickCount() - m_dwStartTime) / 1000
   DWORD elapsedSeconds = (GetTickCount() - m_dwStartTime) / 1000;

      // 格式化为 HH:MM:SS
        int hours = elapsedSeconds / 3600;
        int minutes = (elapsedSeconds % 3600) / 60;
        int seconds = elapsedSeconds % 60;

  CString runtimeStr;
    runtimeStr.Format(_T("%02d:%02d:%02d"), hours, minutes, seconds);

        // 更新统计数据（从DNS服务器获取）
  m_nTotalQueries = m_dnsServer.GetTotalQueries();
        m_nDomesticQueries = m_dnsServer.GetDomesticQueries();
        m_nForeignQueries = m_dnsServer.GetForeignQueries();

    // 调用 UpdateStatistics()
        UpdateStatistics();
    }

    CDialogEx::OnTimer(nIDEvent);
}

// 更新统计信息显示（更新列表第一行）
void CStupidDNSDlg::UpdateStatistics()
{
    if (m_listLog.GetItemCount() == 0) return;
    
// 计算缓存命中率
    int cacheRate = 0;
    if (m_nTotalQueries > 0) {
  cacheRate = (m_nCacheHits * 100) / m_nTotalQueries;
    }
    
    // 计算运行时长
    DWORD elapsedSeconds = m_bServiceRunning ? (GetTickCount() - m_dwStartTime) / 1000 : 0;
    int hours = elapsedSeconds / 3600;
    int minutes = (elapsedSeconds % 3600) / 60;
    int seconds = elapsedSeconds % 60;
    CString runtimeStr;
runtimeStr.Format(_T("%02d:%02d:%02d"), hours, minutes, seconds);
    
    // 更新统计行（第0行）
    CString statsInfo;
    statsInfo.Format(_T("总查询:%d 国内:%d 国外:%d"), 
   m_nTotalQueries, m_nDomesticQueries, m_nForeignQueries);
    
    CString cacheStr;
    cacheStr.Format(_T("缓存:%d%%"), cacheRate);
    
    m_listLog.SetItemText(0, 0, _T("【统计】"));
    m_listLog.SetItemText(0, 1, statsInfo);
    m_listLog.SetItemText(0, 2, cacheStr);
    m_listLog.SetItemText(0, 3, _T("平均:0ms"));  // 可以后续计算平均响应时间
    m_listLog.SetItemText(0, 4, runtimeStr);
}

// 添加日志条目
void CStupidDNSDlg::AddLogEntry(CString time, CString domain, CString type, CString result, CString latency)
{
    // 在列表第1行插入新项（统计行在第0行）
    int index = m_listLog.InsertItem(1, time);

    // 设置各列文本
    m_listLog.SetItemText(index, 1, domain);
    m_listLog.SetItemText(index, 2, type);
    m_listLog.SetItemText(index, 3, result);
m_listLog.SetItemText(index, 4, latency);

    // 检查列表项数，超过 1001 则删除最后一项（保留统计行）
    if (m_listLog.GetItemCount() > 1001)
 {
        m_listLog.DeleteItem(1001);
    }
}

// 清空日志按钮点击事件
void CStupidDNSDlg::OnBnClickedClearLog()
{
    // 弹出确认对话框 AfxMessageBox
    int result = AfxMessageBox(_T("确定要清空所有日志吗？\n（统计信息将保留）"), MB_YESNO | MB_ICONQUESTION);

    // 如果确认，删除除统计行外的所有项
    if (result == IDYES)
    {
        // 从后往前删除，保留第0行（统计行）
        int itemCount = m_listLog.GetItemCount();
        for (int i = itemCount - 1; i > 0; i--)
        {
 m_listLog.DeleteItem(i);
     }
    }
}

// 导出日志按钮点击事件
void CStupidDNSDlg::OnBnClickedExportLog()
{
    // 创建文件保存对话框（.txt 过滤器）
    CFileDialog dlg(FALSE, _T("txt"), _T("dns_log.txt"),
   OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("文本文件 (*.txt)|*.txt|所有文件 (*.*)|*.*||"));

    if (dlg.DoModal() == IDOK)
    {
        CString filePath = dlg.GetPathName();

        // 打开文件，写入表头
        CStdioFile file;
        if (file.Open(filePath, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
        {
            file.WriteString(_T("时间\t域名/事件\t类型\t结果IP\t耗时\r\n"));
  file.WriteString(_T("------------------------------------------------\r\n"));

      // 遍历列表，写入每行数据（制表符分隔），跳过统计行
     int itemCount = m_listLog.GetItemCount();
            for (int i = itemCount - 1; i > 0; i--)  // 从后往前遍历，跳过第0行统计
       {
             CString line;
          line.Format(_T("%s\t%s\t%s\t%s\t%s\r\n"),
         m_listLog.GetItemText(i, 0),  // 时间
   m_listLog.GetItemText(i, 1),  // 域名
         m_listLog.GetItemText(i, 2),  // 类型
  m_listLog.GetItemText(i, 3),  // 结果IP
         m_listLog.GetItemText(i, 4)); // 耗时

      file.WriteString(line);
    }

            // 关闭文件，显示成功消息
            file.Close();
    AfxMessageBox(_T("日志导出成功！"), MB_OK | MB_ICONINFORMATION);
        }
        else
        {
      AfxMessageBox(_T("无法创建文件！"), MB_OK | MB_ICONERROR);
   }
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
    // 显示关于对话框
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
}

// 设置系统DNS按钮点击事件
void CStupidDNSDlg::OnBnClickedSetSystemDns()
{
    // 检查是否有管理员权限
    BOOL isAdmin = IsUserAnAdmin();
  
    if (!isAdmin) {
        AfxMessageBox(_T("需要管理员权限才能修改系统DNS设置！\n\n请右键点击程序，选择【以管理员身份运行】。"), 
       MB_OK | MB_ICONWARNING);
return;
    }
    
    // 获取主要网络适配器
    CString adapterName = CSystemDNSHelper::GetPrimaryAdapter();
  
    if (adapterName.IsEmpty()) {
        AfxMessageBox(_T("未找到可用的网络适配器！"), MB_OK | MB_ICONERROR);
        return;
    }

    // 询问用户操作
    CString msg;
    msg.Format(_T("检测到网络适配器: %s\n\n请选择操作：\n\n"), adapterName);
    msg += _T("【是】- 设置DNS为 127.0.0.1 (使用本程序)\n");
    msg += _T("【否】- 设置DNS为自动获取 (恢复默认)\n");
 
    int result = AfxMessageBox(msg, MB_YESNOCANCEL | MB_ICONQUESTION);
    
    if (result == IDCANCEL) {
        return;
    }
    
  BOOL success = FALSE;
    CString operation;
  
    if (result == IDYES) {
        // 设置为127.0.0.1
        success = CSystemDNSHelper::SetAllAdaptersDNS(TRUE);
   operation = _T("127.0.0.1");
    } else {
        // 设置为自动获取
     success = CSystemDNSHelper::SetAllAdaptersDNS(FALSE);
 operation = _T("自动获取");
    }
    
    if (success) {
     CString successMsg;
        successMsg.Format(_T("DNS设置成功！\n\n已将系统DNS设为: %s\n\n"), operation);
  
        if (result == IDYES) {
            successMsg += _T("现在可以点击【启动服务】按钮启动DNS服务器。\n");
            successMsg += _T("浏览器将通过本程序进行DNS解析。");
        } else {
       successMsg += _T("已恢复为自动获取DNS设置。");
        }
        
        AfxMessageBox(successMsg, MB_OK | MB_ICONINFORMATION);
  
        // 添加日志
        CTime currentTime = CTime::GetCurrentTime();
        CString timeStr = currentTime.Format(_T("%H:%M:%S"));
        CString logMsg;
        logMsg.Format(_T("系统DNS已设为: %s"), operation);
        AddLogEntry(timeStr, logMsg, _T("INFO"), _T("-"), _T("-"));
        
     // 更新按钮文本
      UpdateSystemDNSButtonText();
    } else {
    AfxMessageBox(_T("DNS设置失败！\n\n可能原因：\n1. 没有管理员权限\n2. 网络适配器名称有误\n3. 系统限制"), 
          MB_OK | MB_ICONERROR);
    }
}

// 更新系统DNS按钮文本
void CStupidDNSDlg::UpdateSystemDNSButtonText()
{
    // 简化实现：根据服务运行状态更新按钮文本
if (m_bServiceRunning) {
        SetDlgItemText(IDC_BTN_SET_SYSTEM_DNS, _T("设为系统DNS"));
    } else {
        SetDlgItemText(IDC_BTN_SET_SYSTEM_DNS, _T("设为系统DNS"));
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
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// 处理DNS日志消息（来自服务器线程）
LRESULT CStupidDNSDlg::OnDNSLogMessage(WPARAM wParam, LPARAM lParam)
{
    CString* pLogData = (CString*)lParam;
  
    if (pLogData) {
        // 解析日志数据: 时间|域名|类型|结果|IP|延迟
        int pos = 0;
        CString time = pLogData->Tokenize(_T("|"), pos);
        CString domain = pLogData->Tokenize(_T("|"), pos);
        CString type = pLogData->Tokenize(_T("|"), pos);
        CString result = pLogData->Tokenize(_T("|"), pos);
        CString ip = pLogData->Tokenize(_T("|"), pos);
     CString latency = pLogData->Tokenize(_T("|"), pos);
        
        // 添加到日志列表
    AddLogEntry(time, domain, type, ip, latency);
        
 delete pLogData;
    }
    
    return 0;
}

