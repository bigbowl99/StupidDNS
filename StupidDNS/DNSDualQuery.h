// DNSDualQuery.h - DNS双向查询类
//

#pragma once

#include <vector>
#include <functional>
#include <atlstr.h>
#include "PollutedIPLoader.h"

// 查询结果
struct DNSQueryResult {
    CString ip;           // 返回的IP地址
    int latency;     // 延迟（毫秒）
    BOOL success;         // 是否成功
    CString dnsServer;    // 使用的DNS服务器
    time_t timestamp;     // 查询时间戳
    
    DNSQueryResult() : latency(0), success(FALSE), timestamp(0) {}
};

// 污染检测结果
enum PollutionStatus {
    POLLUTION_CLEAN,      // 干净
    POLLUTION_DETECTED,   // 检测到污染
    POLLUTION_UNCERTAIN   // 不确定
};

// DNS双向查询类
class CDNSDualQuery {
public:
    CDNSDualQuery();
    ~CDNSDualQuery();

// 初始化（加载污染IP库）
    BOOL Initialize();
    
    // 执行双向查询（核心功能）
    BOOL PerformDualQuery(
        const CString& domain,
        const std::vector<CString>& domesticDNS,
        const std::vector<CString>& overseasDNS,
    DNSQueryResult& outDomesticResult,
        DNSQueryResult& outOverseasResult
    );
  
    // 污染检测
    PollutionStatus DetectPollution(
    const DNSQueryResult& domesticResult,
        const DNSQueryResult& overseasResult
    );
    
    // 选择最佳结果
    DNSQueryResult SelectBestResult(
        const DNSQueryResult& domesticResult,
        const DNSQueryResult& overseasResult,
        PollutionStatus pollution
    );
    
    // 设置查询超时（毫秒）
    void SetQueryTimeout(int timeoutMS) { m_queryTimeout = timeoutMS; }
    
    // 获取污染IP数量
    int GetPollutedIPCount() const { return m_pollutedIPLoader.GetPollutedIPCount(); }

private:
    CPollutedIPLoader m_pollutedIPLoader;  // 污染IP加载器
    int m_queryTimeout;// 查询超时（毫秒）
    
    // 查询单个DNS服务器
    BOOL QueryDNSServer(
const CString& domain,
        const CString& dnsServer,
     DNSQueryResult& result
    );
    
    // 并发查询多个DNS（选择最快的）
    BOOL QueryMultipleDNS(
      const CString& domain,
        const std::vector<CString>& dnsServers,
        DNSQueryResult& result
    );
    
    // 构建DNS查询包
    int BuildDNSQuery(const CString& domain, char* buffer, int bufferSize);
    
    // 解析DNS响应包
    CString ParseDNSResponse(const char* response, int length);
    
    // 编码域名（转换为DNS协议格式）
    int EncodeDomainName(const CString& domain, char* buffer);
};
