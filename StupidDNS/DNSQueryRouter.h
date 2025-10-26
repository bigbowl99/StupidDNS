// DNSQueryRouter.h: DNS查询路由器头文件
//

#pragma once

#include "DNSServerManager.h"
#include "DomainClassifier.h"
#include "DNSSpeedTester.h"
#include "DNSDualQuery.h"  // 添加双向查询

// DNS查询路由器类
class CDNSQueryRouter {
public:
    CDNSQueryRouter();
  ~CDNSQueryRouter();
    
    // 初始化
    BOOL Initialize(CDNSServerManager* pServerManager,
  CDomainClassifier* pClassifier);
    
 // 路由查询（核心函数）
    CString RouteQuery(const CString& domain, int& outLatency);
    
    // 解析DNS响应包，提取IP地址
    CString ExtractIPFromResponse(const char* response, int length);
 
    // 获取统计信息
    int GetDomesticQueryCount() const { return m_nDomesticQueries; }
    int GetOverseasQueryCount() const { return m_nOverseasQueries; }
    int GetDualQueryCount() const { return m_nDualQueries; }
 int GetPollutionDetectedCount() const { return m_nPollutionDetected; }

private:
    CDNSServerManager* m_pServerManager;
    CDomainClassifier* m_pClassifier;
    CDNSSpeedTester m_tester;
    CDNSDualQuery m_dualQuery;  // 双向查询器
    
    // 统计信息
    int m_nDomesticQueries;      // 国内查询数
    int m_nOverseasQueries;      // 海外查询数
    int m_nDualQueries;          // 双向查询数
    int m_nPollutionDetected;    // 检测到污染的次数
    
  // 查询方法
  CString QueryDomesticDNS(const CString& domain, int& latency);
    CString QueryOverseasDNS(const CString& domain, int& latency);
  CString QueryBothAndValidate(const CString& domain, int& latency);
    
    // 污染检测
 BOOL ValidateResult(const CString& ip);
    BOOL IsPrivateIP(const CString& ip);
    
    // 实际DNS查询
    CString DoQuery(const CString& dnsServer, const CString& domain, int& latency);
    
    // 获取DNS服务器列表
    std::vector<CString> GetDomesticDNSList();
    std::vector<CString> GetOverseasDNSList();
};
