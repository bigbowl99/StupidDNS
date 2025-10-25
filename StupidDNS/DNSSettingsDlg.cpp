#define _WINSOCK_DEPRECATED_NO_WARNINGS

// DNSSettingsDlg.cpp: DNS服务器管理对话框实现文件
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS  // 必须在包含 winsock 头文件之前定义

#include "pch.h"
#include "StupidDNS.h"
#include "DNSSettingsDlg.h"
#include "afxdialogex.h"
#include <iphlpapi.h>
#include <icmpapi.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CDNSSettingsDlg, CDialogEx)

CDNSSettingsDlg::CDNSSettingsDlg(CWnd* pParent /*=nullptr*/)
  : CDialogEx(IDD_DNS_SETTINGS_DIALOG, pParent)
{
}

CDNSSettingsDlg::~CDNSSettingsDlg()
{
}

void CDNSSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_DNS_SERVERS, m_listDNS);
}

BEGIN_MESSAGE_MAP(CDNSSettingsDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_TEST_ALL_DNS, &CDNSSettingsDlg::OnBnClickedTestAllDns)
    ON_BN_CLICKED(IDOK, &CDNSSettingsDlg::OnBnClickedOk)
END_MESSAGE_MAP()

// CDNSSettingsDlg 消息处理程序

BOOL CDNSSettingsDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 初始化DNS列表
    InitDNSList();

    // 加载默认DNS
    LoadDefaultDNS();

    return TRUE;
}

// 初始化DNS列表控件
void CDNSSettingsDlg::InitDNSList()
{
    // 设置扩展样式
    m_listDNS.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // 插入列：名称(120), IP地址(150), 延迟(80), 状态(80)
    m_listDNS.InsertColumn(0, _T("名称"), LVCFMT_LEFT, 120);
    m_listDNS.InsertColumn(1, _T("IP地址"), LVCFMT_LEFT, 150);
    m_listDNS.InsertColumn(2, _T("延迟"), LVCFMT_CENTER, 80);
    m_listDNS.InsertColumn(3, _T("状态"), LVCFMT_CENTER, 80);
}

// 加载默认DNS服务器
void CDNSSettingsDlg::LoadDefaultDNS()
{
    // 预置亚洲DNS服务器列表
    struct DNSServer {
        LPCTSTR name;
        LPCTSTR ip;
    } defaultServers[] = {
        // 香港DNS
        { _T("香港HKNet"), _T("202.45.84.58") },
        { _T("香港PCCW"), _T("203.80.96.10") },
 
   // 台湾DNS
        { _T("台湾HiNet"), _T("168.95.1.1") },
  { _T("台湾SeedNet"), _T("139.175.10.20") },
 
        // 日本DNS
        { _T("日本OpenDNS"), _T("210.141.113.7") },
   { _T("日本OCN"), _T("61.112.144.2") },
  
        // 新加坡DNS
      { _T("新加坡SingNet"), _T("165.21.83.88") },
 { _T("新加坡StarHub"), _T("202.156.160.2") },
        
        // 韩国DNS
    { _T("韩国KT"), _T("168.126.63.1") },
        { _T("韩国SK"), _T("210.220.163.82") },
      
  // 国际DNS
        { _T("Google DNS"), _T("8.8.8.8") },
        { _T("Cloudflare"), _T("1.1.1.1") },
        { _T("OpenDNS"), _T("208.67.222.222") },
    };

    // 添加到列表
    for (int i = 0; i < _countof(defaultServers); i++)
    {
  int index = m_listDNS.InsertItem(i, defaultServers[i].name);
        m_listDNS.SetItemText(index, 1, defaultServers[i].ip);
    m_listDNS.SetItemText(index, 2, _T("-"));
     m_listDNS.SetItemText(index, 3, _T("未测试"));
    }
}

// 更新DNS项
void CDNSSettingsDlg::UpdateDNSItem(int index, const CString& name, const CString& ip, 
    const CString& latency, const CString& status)
{
    if (index >= 0 && index < m_listDNS.GetItemCount())
    {
        m_listDNS.SetItemText(index, 0, name);
        m_listDNS.SetItemText(index, 1, ip);
        m_listDNS.SetItemText(index, 2, latency);
      m_listDNS.SetItemText(index, 3, status);
    }
}

// 测试DNS速度
void CDNSSettingsDlg::TestDNSSpeed(int index)
{
    CString ip = m_listDNS.GetItemText(index, 1);
    
    // 将IP字符串转换为地址
    ULONG ipAddr = inet_addr(CT2A(ip));
    if (ipAddr == INADDR_NONE)
    {
        m_listDNS.SetItemText(index, 2, _T("-"));
        m_listDNS.SetItemText(index, 3, _T("无效IP"));
        return;
    }

    // 创建ICMP句柄
    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE)
    {
     m_listDNS.SetItemText(index, 2, _T("-"));
   m_listDNS.SetItemText(index, 3, _T("失败"));
      return;
    }

    // 发送ICMP请求
    char sendData[32] = "PING";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
    void* replyBuffer = malloc(replySize);
    
    if (replyBuffer)
    {
      DWORD result = IcmpSendEcho(hIcmp, ipAddr, sendData, sizeof(sendData),
            NULL, replyBuffer, replySize, 2000);
    
        if (result != 0)
        {
            PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer;
        CString latency;
            latency.Format(_T("%dms"), pEchoReply->RoundTripTime);
    m_listDNS.SetItemText(index, 2, latency);
      m_listDNS.SetItemText(index, 3, _T("正常"));
        }
   else
        {
            m_listDNS.SetItemText(index, 2, _T("-"));
 m_listDNS.SetItemText(index, 3, _T("超时"));
        }
        
      free(replyBuffer);
    }
 
    IcmpCloseHandle(hIcmp);
}

// 全部测速按钮
void CDNSSettingsDlg::OnBnClickedTestAllDns()
{
    int itemCount = m_listDNS.GetItemCount();
    
    if (itemCount == 0) {
        AfxMessageBox(_T("DNS服务器列表为空！"), MB_OK | MB_ICONWARNING);
        return;
    }
    
    // 显示进度提示
    CString progressMsg;
    progressMsg.Format(_T("正在测试 %d 个DNS服务器，请稍候..."), itemCount);
    SetDlgItemText(IDOK, progressMsg);
    GetDlgItem(IDOK)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_TEST_ALL_DNS)->EnableWindow(FALSE);

    for (int i = 0; i < itemCount; i++)
    {
    // 更新进度
        CString status;
        status.Format(_T("测试中 %d/%d"), i + 1, itemCount);
        m_listDNS.SetItemText(i, 3, status);
        
        // 强制刷新进度
        m_listDNS.RedrawItems(i, i);
    UpdateData(FALSE);
        
        // 处理消息队列，让界面流畅
        MSG msg;
     while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
   TranslateMessage(&msg);
     DispatchMessage(&msg);
        }
    
        // 执行测试
  TestDNSSpeed(i);
    }
    
    // 恢复按钮状态
    SetDlgItemText(IDOK, _T("应用"));
    GetDlgItem(IDOK)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_TEST_ALL_DNS)->EnableWindow(TRUE);
    
    CString msg;
    msg.Format(_T("测速完成！\n已测试 %d 个DNS服务器\n\n点击【应用】按钮将最快的DNS设为海外基础服务器"), itemCount);
    AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
}

// 应用最快的DNS服务器
void CDNSSettingsDlg::ApplyFastestDNS()
{
    // 收集所有测试成功且有效的DNS
    struct DNSItem {
        int index;
        int latency;
     CString name;
     CString ip;
    };
    
    std::vector<DNSItem> validDNS;
    
    for (int i = 0; i < m_listDNS.GetItemCount(); i++) {
        CString status = m_listDNS.GetItemText(i, 3);
        CString latencyStr = m_listDNS.GetItemText(i, 2);
        
        // 只处理状态为"正常"的DNS
  if (status == _T("正常") && latencyStr != _T("-")) {
   // 解析延迟，去掉"ms"后缀
   latencyStr.Replace(_T("ms"), _T(""));
        int latency = _ttoi(latencyStr);
    
         if (latency > 0) {
     DNSItem item;
           item.index = i;
item.latency = latency;
            item.name = m_listDNS.GetItemText(i, 0);
   item.ip = m_listDNS.GetItemText(i, 1);
         validDNS.push_back(item);
            }
        }
    }
    
    if (validDNS.empty()) {
        AfxMessageBox(_T("没有可用的DNS服务器！\n请先执行【全部测速】。"), MB_OK | MB_ICONWARNING);
      return;
    }
    
    // 按延迟排序
    std::sort(validDNS.begin(), validDNS.end(), [](const DNSItem& a, const DNSItem& b) {
        return a.latency < b.latency;
    });
    
    // 取最快的前5个（如果不足5个则全部使用）
    int selectCount = min(5, (int)validDNS.size());
    
    CString msg;
    msg = _T("已选择以下DNS服务器作为海外基础服务器：\n\n");

    for (int i = 0; i < selectCount; i++) {
 CString line;
        line.Format(_T("%d. %s (%s) - %dms\n"), 
    i + 1, validDNS[i].name, validDNS[i].ip, validDNS[i].latency);
      msg += line;
    }
    
    msg += _T("\n这些DNS服务器将用于解析海外域名。");
    
    AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
    
    // TODO: 实际应用逻辑 - 将选中的DNS保存到配置
    // 这里可以调用 DNSServerManager 保存配置
}

// 应用按钮
void CDNSSettingsDlg::OnBnClickedOk()
{
    // 应用最快的DNS
    ApplyFastestDNS();
    
    // 关闭对话框
    CDialogEx::OnOK();
}
