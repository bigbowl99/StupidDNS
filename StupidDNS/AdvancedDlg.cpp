// AdvancedDlg.cpp: 高级选项对话框实现文件
//

#include "pch.h"
#include "StupidDNS.h"
#include "AdvancedDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CAdvancedDlg, CDialogEx)

CAdvancedDlg::CAdvancedDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_ADVANCED_DIALOG, pParent)
, m_bEnableCache(TRUE)
    , m_nCacheSize(10000)
 , m_nCacheTTL(3600)
    , m_bPollutionDetect(TRUE)
    , m_nQueryTimeout(2000)
, m_nConcurrentQueries(5)
    , m_bEnableLog(TRUE)
    , m_strLogPath(_T("C:\\logs\\dns.log"))
    , m_bAutoStart(FALSE)
    , m_bMinimizeToTray(TRUE)
{
}

CAdvancedDlg::~CAdvancedDlg()
{
}

void CAdvancedDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_CHECK_ENABLE_CACHE, m_bEnableCache);
    DDX_Text(pDX, IDC_EDIT_CACHE_SIZE, m_nCacheSize);
    DDX_Text(pDX, IDC_EDIT_CACHE_TTL, m_nCacheTTL);
    DDX_Check(pDX, IDC_CHECK_POLLUTION_DETECT, m_bPollutionDetect);
    DDX_Text(pDX, IDC_EDIT_QUERY_TIMEOUT, m_nQueryTimeout);
    DDX_Text(pDX, IDC_EDIT_CONCURRENT_QUERIES, m_nConcurrentQueries);
    DDX_Check(pDX, IDC_CHECK_ENABLE_LOG, m_bEnableLog);
    DDX_Text(pDX, IDC_EDIT_LOG_PATH, m_strLogPath);
    DDX_Control(pDX, IDC_COMBO_LOG_LEVEL, m_comboLogLevel);
    DDX_Check(pDX, IDC_CHECK_AUTO_START, m_bAutoStart);
    DDX_Check(pDX, IDC_CHECK_MINIMIZE_TO_TRAY, m_bMinimizeToTray);
}

BEGIN_MESSAGE_MAP(CAdvancedDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BROWSE_LOG, &CAdvancedDlg::OnBnClickedBrowseLog)
    ON_BN_CLICKED(IDC_BTN_RESET_ADVANCED, &CAdvancedDlg::OnBnClickedResetAdvanced)
    ON_BN_CLICKED(IDOK, &CAdvancedDlg::OnBnClickedOk)
END_MESSAGE_MAP()

// CAdvancedDlg 消息处理程序

BOOL CAdvancedDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 初始化日志级别下拉框
    m_comboLogLevel.AddString(_T("简洁"));
    m_comboLogLevel.AddString(_T("正常"));
    m_comboLogLevel.AddString(_T("详细"));
    m_comboLogLevel.AddString(_T("调试"));
    m_comboLogLevel.SetCurSel(1);  // 默认选择"正常"

    // 加载设置
    LoadSettings();

    return TRUE;
}

// 设置默认值
void CAdvancedDlg::SetDefaultValues()
{
    m_bEnableCache = TRUE;
    m_nCacheSize = 10000;
    m_nCacheTTL = 3600;
    m_bPollutionDetect = TRUE;
    m_nQueryTimeout = 2000;
    m_nConcurrentQueries = 5;
    m_bEnableLog = TRUE;
    m_strLogPath = _T("C:\\logs\\dns.log");
    m_bAutoStart = FALSE;
    m_bMinimizeToTray = TRUE;
m_comboLogLevel.SetCurSel(1);

    UpdateData(FALSE);
}

// 加载设置
void CAdvancedDlg::LoadSettings()
{
    // TODO: 从注册表或INI文件加载设置
    // 这里使用默认值
    UpdateData(FALSE);
}

// 保存设置
void CAdvancedDlg::SaveSettings()
{
    // TODO: 保存到注册表或INI文件
    // 示例代码（使用注册表）：
    /*
    AfxGetApp()->WriteProfileInt(_T("Settings"), _T("EnableCache"), m_bEnableCache);
    AfxGetApp()->WriteProfileInt(_T("Settings"), _T("CacheSize"), m_nCacheSize);
    AfxGetApp()->WriteProfileInt(_T("Settings"), _T("CacheTTL"), m_nCacheTTL);
    // ... 其他设置
    */
}

// 验证输入
BOOL CAdvancedDlg::ValidateInput()
{
    UpdateData(TRUE);

    // 验证缓存大小
    if (m_nCacheSize <= 0 || m_nCacheSize > 1000000)
    {
        AfxMessageBox(_T("缓存大小必须在 1-1000000 之间"), MB_OK | MB_ICONWARNING);
        return FALSE;
    }

    // 验证缓存有效期
 if (m_nCacheTTL <= 0 || m_nCacheTTL > 86400)
    {
        AfxMessageBox(_T("缓存有效期必须在 1-86400 秒之间"), MB_OK | MB_ICONWARNING);
     return FALSE;
    }

    // 验证查询超时
    if (m_nQueryTimeout < 100 || m_nQueryTimeout > 10000)
    {
        AfxMessageBox(_T("查询超时必须在 100-10000 毫秒之间"), MB_OK | MB_ICONWARNING);
        return FALSE;
  }

    // 验证并发查询数
    if (m_nConcurrentQueries < 1 || m_nConcurrentQueries > 10)
    {
        AfxMessageBox(_T("并发查询数必须在 1-10 之间"), MB_OK | MB_ICONWARNING);
        return FALSE;
    }

    // 验证日志路径
    if (m_bEnableLog && m_strLogPath.IsEmpty())
    {
        AfxMessageBox(_T("请指定日志文件路径"), MB_OK | MB_ICONWARNING);
        return FALSE;
    }

    return TRUE;
}

// 浏览日志路径
void CAdvancedDlg::OnBnClickedBrowseLog()
{
    CFileDialog dlg(FALSE, _T("log"), _T("dns.log"),
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("日志文件 (*.log)|*.log|文本文件 (*.txt)|*.txt|所有文件 (*.*)|*.*||"));

    if (dlg.DoModal() == IDOK)
    {
   m_strLogPath = dlg.GetPathName();
        UpdateData(FALSE);
    }
}

// 恢复默认设置
void CAdvancedDlg::OnBnClickedResetAdvanced()
{
    if (AfxMessageBox(_T("确定要恢复默认设置吗？"), 
        MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        SetDefaultValues();
  }
}

// 确定按钮
void CAdvancedDlg::OnBnClickedOk()
{
    if (!ValidateInput())
    {
        return;
    }

    // 保存设置
    SaveSettings();

    CDialogEx::OnOK();
}
