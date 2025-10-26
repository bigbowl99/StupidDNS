// DNSSettingsDlg.h: DNS服务器管理对话框头文件
//

#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include "DNSConfigCache.h"  // 添加配置缓存

// CDNSSettingsDlg 对话框
class CDNSSettingsDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CDNSSettingsDlg)

public:
    CDNSSettingsDlg(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~CDNSSettingsDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DNS_SETTINGS_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()

public:
    // 控件变量
    CListCtrl m_listDNS;     // DNS服务器列表

    // 消息处理函数
    virtual BOOL OnInitDialog();
  afx_msg void OnBnClickedTestAllDns();    // 全部测速按钮
    afx_msg void OnBnClickedOk();    // 应用按钮

private:
    // 辅助函数
    void InitDNSList();        // 初始化DNS列表控件
  void LoadDefaultDNS();             // 加载默认DNS服务器
    void TestDNSSpeed(int index);  // 测试DNS速度
    void UpdateDNSItem(int index, const CString& name, const CString& ip, const CString& latency, const CString& status);
    void ApplyFastestDNS();  // 应用最快的DNS服务器
    
    // 配置缓存相关
    void LoadCachedConfig();  // 加载缓存配置
    void SaveConfigCache();   // 保存配置缓存
    
    // 成员变量
    CDNSConfigCache m_configCache;  // 配置缓存管理器
};
