// DNSQueryRouter.h: DNS查询路由器头文件
//

#pragma once

#include "DNSServerManager.h"
#include "DomainClassifier.h"
#include "DNSSpeedTester.h"

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
    
private:
    CDNSServerManager* m_pServerManager;
    CDomainClassifier* m_pClassifier;
    CDNSSpeedTester m_tester;
    
    // 查询方法
  CString QueryDomesticDNS(const CString& domain, int& latency);
    CString QueryOverseasDNS(const CString& domain, int& latency);
    CString QueryBothAndValidate(const CString& domain, int& latency);
    
    // 污染检测
    BOOL ValidateResult(const CString& ip);
    BOOL IsPrivateIP(const CString& ip);
    
    // 实际DNS查询
    CString DoQuery(const CString& dnsServer, const CString& domain, int& latency);
};
