// StupidDNSDlg.h: 头文件
//

#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include "DNSServerManager.h"  // 添加DNS管理器
#include "DomainClassifier.h"  // 添加域名分类器
#include "DNSQueryRouter.h"    // 添加DNS查询路由器
#include "DNSServerCore.h"     // 添加DNS服务器核心
#include "SystemDNSHelper.h"   // 添加系统DNS设置辅助类

// CStupidDNSDlg 对话框
class CStupidDNSDlg : public CDialogEx
{
// 构造
public:
	CStupidDNSDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STUPIDDNS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	// 控件变量
	CListCtrl m_listLog;       // 日志列表控件

	// 成员变量
	BOOL m_bServiceRunning;    // 服务运行状态
	int m_nPort;      // 监听端口
	DWORD m_dwStartTime;            // 服务启动时间
	int m_nTotalQueries;      // 总查询数
	int m_nCacheHits;        // 缓存命中数
	int m_nDomesticQueries;           // 国内查询数
	int m_nForeignQueries;// 国外查询数

	// DNS管理器和路由器
	CDNSServerManager m_dnsManager;   // DNS服务器管理器
	CDomainClassifier m_domainClassifier;  // 域名分类器
	CDNSQueryRouter m_queryRouter;    // DNS查询路由器
	CDNSServerCore m_dnsServer; // DNS服务器核心

	// 消息处理函数
	afx_msg void OnBnClickedStart();        // 启动服务按钮
	afx_msg void OnBnClickedStop();     // 停止服务按钮
	afx_msg void OnBnClickedClearLog();   // 清空日志按钮
	afx_msg void OnBnClickedExportLog(); // 导出日志按钮
	afx_msg void OnBnClickedDNSSettings();    // DNS设置按钮
	afx_msg void OnBnClickedDomainRules();  // 域名规则按钮
	afx_msg void OnBnClickedAdvanced();   // 高级选项按钮
	afx_msg void OnBnClickedAbout();    // 关于按钮
	afx_msg void OnTimer(UINT_PTR nIDEvent);  // 定时器消息
	afx_msg LRESULT OnDNSLogMessage(WPARAM wParam, LPARAM lParam);  // DNS日志消息
	afx_msg void OnBnClickedSetSystemDns(); // 设置系统DNS按钮

	// 辅助函数（public，供回调使用）
	void InitLogList();     // 初始化日志列表
	void AddLogEntry(CString time, CString domain, CString type, CString result, CString latency);  // 添加日志条目
	void UpdateStatistics();    // 更新统计信息显示
	void ShowSpeedTestProgress(CString status);  // 显示测速进度（改为public）
	void UpdateSystemDNSButtonText();  // 更新系统DNS按钮文本
	
private:
	// DNS初始化函数
	void InitializeDNSServers();      // 初始化DNS服务器
	void InitializeDomainClassifier();  // 初始化域名分类器
};
