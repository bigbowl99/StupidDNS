// DNSSpeedTester.h: DNS测速类头文件
//

#pragma once

#include "DNSServer.h"
#include <vector>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// DNS测速类
class CDNSSpeedTester {
public:
    CDNSSpeedTester();
    ~CDNSSpeedTester();
    
    // 测试单个DNS服务器
    SpeedTestResult TestSingleDNS(const CString& dnsIP, int testCount = 3);
    
    // 测试多个DNS服务器（并发）
    std::vector<SpeedTestResult> TestMultipleDNS(
        const std::vector<CString>& dnsIPs, 
     int testCount = 3);
    
    // 测试DNS查询延迟
    int TestDNSQuery(const CString& dnsIP, 
       const CString& testDomain = _T("www.amazon.com"));
    
    // ICMP Ping测试
    int TestICMPPing(const CString& ip);
    
    // 设置测试超时时间
    void SetTimeout(int timeoutMS) { m_timeout = timeoutMS; }
    
    // 获取测试进度回调
    void SetProgressCallback(
        std::function<void(int current, int total, CString status)> callback) {
        m_progressCallback = callback;
    }
    
private:
    int m_timeout;          // 超时时间(毫秒)，默认2000
    BOOL m_wsaInitialized;  // WSA是否已初始化
    std::function<void(int, int, CString)> m_progressCallback;
    
    // 初始化Winsock
    BOOL InitializeWinsock();
    
    // 清理Winsock
    void CleanupWinsock();
    
    // 构造DNS查询包
int BuildDNSQuery(const CString& domain, char* buffer, int bufferSize);
    
    // 解析DNS响应包
    BOOL ParseDNSResponse(const char* response, int length);
    
    // 执行单次DNS查询
    int DoSingleDNSQuery(SOCKET sock, sockaddr_in& dnsAddr, 
                   const char* query, int queryLen);
    
    // 域名编码（转换为DNS协议格式）
    int EncodeDomainName(const CString& domain, char* buffer);
};
