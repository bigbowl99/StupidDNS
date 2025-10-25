// AboutDlg.cpp: 关于对话框实现文件
//

#include "pch.h"
#include "StupidDNS.h"
#include "AboutDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUT_DIALOG)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
    ON_NOTIFY(NM_CLICK, IDC_LINK_GITHUB, &CAboutDlg::OnNMClickLinkGithub)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置图标（使用应用程序图标）
    HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    if (hIcon)
    {
        CStatic* pStatic = (CStatic*)GetDlgItem(IDC_STATIC_LOGO);
        if (pStatic)
        {
            pStatic->SetIcon(hIcon);
     }
    }

    return TRUE;
}

// GitHub链接点击事件
void CAboutDlg::OnNMClickLinkGithub(NMHDR *pNMHDR, LRESULT *pResult)
{
    // 打开默认浏览器访问GitHub
 ShellExecute(NULL, _T("open"), _T("https://github.com/yourname/stupiddns"), 
             NULL, NULL, SW_SHOWNORMAL);

    *pResult = 0;
}
