// AdvancedDlg.h: 高级选项对话框头文件
//

#pragma once

#include <afxwin.h>
#include <afxcmn.h>

// CAdvancedDlg 对话框
class CAdvancedDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CAdvancedDlg)

public:
    CAdvancedDlg(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~CAdvancedDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_ADVANCED_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()

public:
    // 成员变量
    BOOL m_bEnableCache;     // 启用缓存
    int m_nCacheSize;           // 缓存大小
    int m_nCacheTTL;   // 缓存有效期
    BOOL m_bPollutionDetect;  // 启用污染检测
    int m_nQueryTimeout;           // 查询超时
    int m_nConcurrentQueries;      // 并发查询数
    BOOL m_bEnableLog;             // 启用日志
    CString m_strLogPath;          // 日志路径
    CComboBox m_comboLogLevel;     // 日志级别
    BOOL m_bAutoStart;   // 开机自启
    BOOL m_bMinimizeToTray;        // 最小化到托盘

    // 消息处理函数
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedBrowseLog();
    afx_msg void OnBnClickedResetAdvanced();
afx_msg void OnBnClickedOk();

private:
    // 辅助函数
    void LoadSettings();   // 加载设置
    void SaveSettings();   // 保存设置
    void SetDefaultValues();       // 设置默认值
    BOOL ValidateInput();      // 验证输入
};
