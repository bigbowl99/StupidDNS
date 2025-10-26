# DNS双向查询与污染检测实现方案

## 核心逻辑

### 1. 双向查询类 (DNSDualQuery)

```cpp
// DNSDualQuery.h
#pragma once
#include <vector>
#include <functional>

// 查询结果
struct DNSQueryResult {
    CString ip;
    int latency;  // 毫秒
    BOOL success;
    CString dnsServer;  // 使用的DNS服务器
    time_t timestamp;
};

// 污染检测结果
enum PollutionStatus {
    CLEAN,          // 干净
  POLLUTED,       // 被污染
    UNCERTAIN// 不确定
};

class CDNSDualQuery {
public:
    CDNSDualQuery();
    ~CDNSDualQuery();

// 执行双向查询
BOOL PerformDualQuery(
  const CString& domain,
        const std::vector<CString>& domesticDNS,
    const std::vector<CString>& overseasDNS,
        DNSQueryResult& domesticResult,
        DNSQueryResult& overseasResult
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

private:
    // 查询单个DNS服务器
    BOOL QueryDNSServer(
        const CString& domain,
        const CString& dnsServer,
 DNSQueryResult& result
    );

    // 检查是否是污染IP
    BOOL IsPollutedIP(const CString& ip);

    // TTL检查
    BOOL IsSuspiciousTTL(DWORD ttl);

    // 预置污染IP列表
    std::vector<CString> m_pollutedIPs;

    // 初始化污染IP列表
 void InitPollutedIPList();
};
```

### 2. 污染IP列表

```cpp
// DNSDualQuery.cpp
void CDNSDualQuery::InitPollutedIPList() {
    // GFW常见污染IP
    m_pollutedIPs = {
        _T("8.7.198.45"),
        _T("37.61.54.158"),
        _T("46.82.174.68"),
      _T("59.24.3.173"),
        _T("64.33.88.161"),
     _T("64.33.99.47"),
        _T("64.66.163.251"),
        _T("65.104.202.252"),
        _T("65.160.219.113"),
        _T("66.45.252.237"),
        _T("72.14.205.104"),
        _T("72.14.205.99"),
        _T("78.16.49.15"),
     _T("93.46.8.89"),
     _T("128.121.126.139"),
        _T("159.106.121.75"),
        _T("169.132.13.103"),
 _T("192.67.198.6"),
     _T("202.106.1.2"),
        _T("202.181.7.85"),
      _T("203.161.230.171"),
_T("203.98.7.65"),
  _T("207.12.88.98"),
   _T("208.56.31.43"),
        _T("209.145.54.50"),
 _T("209.220.30.174"),
        _T("209.36.73.33"),
        _T("211.94.66.147"),
        _T("213.169.251.35"),
        _T("216.221.188.182"),
        _T("216.234.179.13"),
      _T("243.185.187.39"),
        _T("247.157.15.213"),
        _T("253.157.14.165")
    };
}

BOOL CDNSDualQuery::IsPollutedIP(const CString& ip) {
    for (const auto& pollutedIP : m_pollutedIPs) {
     if (ip == pollutedIP) {
      return TRUE;
        }
    }
  return FALSE;
}

BOOL CDNSDualQuery::IsSuspiciousTTL(DWORD ttl) {
    // GFW污染通常返回很短的TTL (< 100秒)
    return (ttl > 0 && ttl < 100);
}
```

### 3. 双向查询实现

```cpp
BOOL CDNSDualQuery::PerformDualQuery(
    const CString& domain,
    const std::vector<CString>& domesticDNS,
    const std::vector<CString>& overseasDNS,
    DNSQueryResult& domesticResult,
    DNSQueryResult& overseasResult
) {
    // 并发查询国内DNS
  HANDLE hDomesticThread = CreateThread(NULL, 0, 
     [](LPVOID param) -> DWORD {
    // 查询国内DNS（选择最快的）
       return 0;
        }, 
        &domesticResult, 0, NULL);

    // 并发查询海外DNS
    HANDLE hOverseasThread = CreateThread(NULL, 0,
        [](LPVOID param) -> DWORD {
        // 查询海外DNS（选择最快的）
         return 0;
      },
        &overseasResult, 0, NULL);

    // 等待两个查询完成
    HANDLE handles[] = { hDomesticThread, hOverseasThread };
    WaitForMultipleObjects(2, handles, TRUE, 5000);  // 5秒超时

    CloseHandle(hDomesticThread);
    CloseHandle(hOverseasThread);

    return (domesticResult.success || overseasResult.success);
}
```

### 4. 污染检测逻辑

```cpp
PollutionStatus CDNSDualQuery::DetectPollution(
    const DNSQueryResult& domesticResult,
    const DNSQueryResult& overseasResult
) {
  // 1. 检查国内DNS是否返回污染IP
    if (domesticResult.success) {
      if (IsPollutedIP(domesticResult.ip)) {
         return POLLUTED;
    }
    }

    // 2. 检查TTL异常
    // (需要从DNS响应中获取TTL值)
 // if (IsSuspiciousTTL(domesticResult.ttl)) {
    //     return POLLUTED;
    // }

    // 3. 比较国内外结果
    if (domesticResult.success && overseasResult.success) {
        if (domesticResult.ip == overseasResult.ip) {
          // 结果一致，认为是干净的
            return CLEAN;
      } else {
      // 结果不一致，需要进一步判断
            // 如果国内返回的是污染IP，则认为被污染
            if (IsPollutedIP(domesticResult.ip)) {
     return POLLUTED;
            }
            // 否则不确定
return UNCERTAIN;
        }
    }

    return UNCERTAIN;
}
```

### 5. 选择最佳结果

```cpp
DNSQueryResult CDNSDualQuery::SelectBestResult(
    const DNSQueryResult& domesticResult,
    const DNSQueryResult& overseasResult,
    PollutionStatus pollution
) {
    // 如果被污染，直接使用海外结果
    if (pollution == POLLUTED) {
        return overseasResult;
  }

    // 如果干净，比较延迟，选择更快的
    if (pollution == CLEAN) {
    if (domesticResult.success && overseasResult.success) {
    return (domesticResult.latency < overseasResult.latency) 
        ? domesticResult : overseasResult;
        }
if (domesticResult.success) return domesticResult;
        if (overseasResult.success) return overseasResult;
    }

    // 不确定的情况，优先使用海外结果（更保险）
    if (overseasResult.success) return overseasResult;
    return domesticResult;
}
```

## 集成到DNSQueryRouter

```cpp
// DNSQueryRouter.cpp
CString CDNSQueryRouter::QueryDomain(const CString& domain) {
    // 1. 检查缓存
    CString cachedIP = m_cache.Get(domain);
    if (!cachedIP.IsEmpty()) {
        return cachedIP;
    }

    // 2. 域名分类
    DomainType type = m_classifier.Classify(domain);

    CString result;
    switch (type) {
        case DOMAIN_TYPE_CHINA:
       // 使用国内DNS
   result = QueryWithDNS(domain, m_dnsManager.GetDomesticDNS());
   break;

        case DOMAIN_TYPE_GFW:
     // 使用海外DNS
      result = QueryWithDNS(domain, m_dnsManager.GetOverseasDNS());
         break;

    case DOMAIN_TYPE_UNKNOWN:
   // 双向查询 + 污染检测
            DNSQueryResult domesticResult, overseasResult;
            m_dualQuery.PerformDualQuery(
             domain,
      m_dnsManager.GetDomesticDNS(),
             m_dnsManager.GetOverseasDNS(),
   domesticResult,
    overseasResult
        );

PollutionStatus pollution = m_dualQuery.DetectPollution(
     domesticResult, overseasResult
            );

            DNSQueryResult bestResult = m_dualQuery.SelectBestResult(
             domesticResult, overseasResult, pollution
     );

            result = bestResult.ip;

  // 更新动态规则
     if (pollution == POLLUTED) {
    m_dynamicRules.AddGFWDomain(domain);
        } else if (pollution == CLEAN && 
      bestResult.dnsServer == domesticResult.dnsServer) {
       m_dynamicRules.AddChinaDomain(domain);
            }
            break;
    }

    // 3. 缓存结果
    m_cache.Put(domain, result);

    return result;
}
```

## 测试用例

### 测试1：检测污染IP
```cpp
void TestPollutedIP() {
    CDNSDualQuery query;
    
    DNSQueryResult domestic;
    domestic.ip = _T("8.7.198.45");  // 污染IP
    domestic.success = TRUE;

    DNSQueryResult overseas;
    overseas.ip = _T("172.217.160.78");  // 正常IP
    overseas.success = TRUE;

    PollutionStatus status = query.DetectPollution(domestic, overseas);
    ASSERT(status == POLLUTED);
}
```

### 测试2：正常域名
```cpp
void TestCleanDomain() {
    CDNSDualQuery query;
    
    DNSQueryResult domestic;
    domestic.ip = _T("110.242.68.66");  // baidu.com
    domestic.latency = 20;
    domestic.success = TRUE;

    DNSQueryResult overseas;
    overseas.ip = _T("110.242.68.66");  // 相同
    overseas.latency = 150;
    overseas.success = TRUE;

    PollutionStatus status = query.DetectPollution(domestic, overseas);
 ASSERT(status == CLEAN);

  DNSQueryResult best = query.SelectBestResult(domestic, overseas, status);
    ASSERT(best.ip == domestic.ip);  // 选择延迟更低的
}
```

## 性能优化

### 1. 并发查询
- 使用线程池
- 超时控制 (2-5秒)
- 取最快响应

### 2. 缓存策略
- 成功结果缓存 24小时
- 失败结果缓存 5分钟（避免重复查询）
- LRU淘汰策略

### 3. 动态规则
- 置信度阈值 (>0.8)
- 查询次数阈值 (>3次)
- 定期清理低置信度规则

## 注意事项

1. **线程安全**：缓存和动态规则需要加锁
2. **超时处理**：DNS查询必须有超时
3. **错误处理**：网络错误、DNS服务器不可用
4. **日志记录**：记录污染检测结果，便于调试
5. **用户反馈**：允许用户手动标记域名类型
