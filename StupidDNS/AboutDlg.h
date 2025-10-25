// AboutDlg.h: 关于对话框头文件
//

#pragma once

#include <afxwin.h>

// CAboutDlg 对话框
class CAboutDlg : public CDialogEx
{
public:
 CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUT_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL OnInitDialog();

    // 消息处理函数
 afx_msg void OnNMClickLinkGithub(NMHDR *pNMHDR, LRESULT *pResult);

    DECLARE_MESSAGE_MAP()
};
