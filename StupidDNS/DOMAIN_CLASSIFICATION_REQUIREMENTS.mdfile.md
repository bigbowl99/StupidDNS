# StupidDNS 域名分类与智能路由功能需求

## 模块概述
实现基于规则文件的域名智能分类，支持关联域名检测，动态决定使用国内或海外DNS服务器。

---

## 1. 规则文件说明

### 1.1 文件位置
```
程序目录\rules\
├── accelerated-domains.china.conf  (中国域名白名单)
├── bogus-nxdomain.china.conf       (污染IP黑名单)
└── gfw.txt                         (GFW域名黑名单)
```

### 1.2 文件格式

#### accelerated-domains.china.conf
```
格式：每行一个域名（可能带通配符）
示例：
server=/baidu.com/114.114.114.114
server=/taobao.com/114.114.114.114
server=/qq.com/114.114.114.114
server=/.cn/114.114.114.114

解析规则：
- 提取 server=/ 和 / 之间的域名部分
- 域名：baidu.com, taobao.com, qq.com, .cn
- .cn 表示所有 .cn 后缀的域名
```

#### bogus-nxdomain.china.conf
```
格式：污染IP地址列表
示例：
bogus-nxdomain=243.185.187.39
bogus-nxdomain=46.82.174.68
bogus-nxdomain=37.61.54.158
bogus-nxdomain=93.46.8.89

解析规则：
- 提取 bogus-nxdomain= 后面的IP地址
- 用于检测DNS污染
```

#### gfw.txt
```
格式：每行一个域名
示例：
google.com
youtube.com
facebook.com
twitter.com
github.com

解析规则：
- 纯文本，每行一个域名
- 可能有注释行（#开头），需要跳过
- 可能有空行，需要跳过
```

---

## 2. 规则加载器类

### 2.1 类定义
```cpp
// DomainRuleLoader.h
#include <string>
#include <set>
#include <vector>
#include <map>

class CDomainRuleLoader {
public:
    CDomainRuleLoader();
    ~CDomainRuleLoader();
    
    // 加载所有规则文件
    BOOL LoadAllRules();
    
    // 加载单个规则文件
    BOOL LoadChinaDomains(const CString& filePath);
    BOOL LoadPoisonIPs(const CString& filePath);
    BOOL LoadGFWList(const CString& filePath);
    
    // 查询接口
    BOOL IsChineseDomain(const CString& domain) const;
    BOOL IsGFWDomain(const CString& domain) const;
    BOOL IsPoisonIP(const CString& ip) const;
    
    // 获取规则数量
    int GetChinaDomainCount() const { return m_chinaDomains.size(); }
    int GetGFWDomainCount() const { return m_gfwDomains.size(); }
    int GetPoisonIPCount() const { return m_poisonIPs.size(); }
    
private:
    std::set<CString> m_chinaDomains;      // 中国域名集合
    std::set<CString> m_gfwDomains;        // GFW域名集合
    std::set<CString> m_poisonIPs;         // 污染IP集合
    
    // 域名匹配辅助函数
    BOOL MatchDomain(const CString& domain, const CString& pattern) const;
    
    // 提取主域名
    CString ExtractMainDomain(const CString& domain) const;
    
    // 解析dnsmasq格式的行
    CString ParseDnsmasqLine(const CString& line);
    
    // 规则文件路径
    CString GetRulesDirectory() const;
};
```

### 2.2 实现要点

#### LoadChinaDomains 实现
```
功能：加载 accelerated-domains.china.conf 文件
文件格式：server=/domain/114.114.114.114

流程：
1. 构造文件路径
   CString filePath = GetRulesDirectory() + _T("\\accelerated-domains.china.conf");

2. 打开文件
   CStdioFile file;
   if (!file.Open(filePath, CFile::modeRead | CFile::typeText)) {
       TRACE(_T("无法打开中国域名列表: %s\n"), filePath);
       return FALSE;
   }

3. 逐行读取
   CString line;
   int count = 0;
   while (file.ReadString(line)) {
       line.Trim();
       
       // 跳过空行和注释
       if (line.IsEmpty() || line[0] == '#') continue;
       
       // 解析 server=/domain/ip 格式
       CString domain = ParseDnsmasqLine(line);
       if (!domain.IsEmpty()) {
           m_chinaDomains.insert(domain);
           count++;
       }
   }
   
4. 关闭文件
   file.Close();
   
5. 记录日志
   TRACE(_T("加载了 %d 个中国域名\n"), count);
   
6. 返回 TRUE

ParseDnsmasqLine 实现：
CString ParseDnsmasqLine(const CString& line) {
    // 查找 server=/ 开头
    int startPos = line.Find(_T("server=/"));
    if (startPos == -1) return _T("");
    
    // 跳过 "server=/"
    int domainStart = startPos + 8;
    
    // 查找结束的 /
    int domainEnd = line.Find('/', domainStart);
    if (domainEnd == -1) return _T("");
    
    // 提取域名
    CString domain = line.Mid(domainStart, domainEnd - domainStart);
    domain.Trim();
    
    return domain;
}
```

#### LoadGFWList 实现
```
功能：加载 gfw.txt 文件
文件格式：每行一个域名

流程：
1. 构造文件路径
   CString filePath = GetRulesDirectory() + _T("\\gfw.txt");

2. 打开文件
   CStdioFile file;
   if (!file.Open(filePath, CFile::modeRead | CFile::typeText)) {
       return FALSE;
   }

3. 逐行读取
   CString line;
   int count = 0;
   while (file.ReadString(line)) {
       line.Trim();
       
       // 跳过空行、注释、特殊字符开头的行
       if (line.IsEmpty() || 
           line[0] == '#' || 
           line[0] == '!' || 
           line[0] == '[') {
           continue;
       }
       
       // 去掉可能的协议前缀（http://, https://）
       if (line.Find(_T("://")) != -1) {
           int pos = line.Find(_T("://"));
           line = line.Mid(pos + 3);
       }
       
       // 去掉路径部分（/之后的内容）
       int slashPos = line.Find('/');
       if (slashPos != -1) {
           line = line.Left(slashPos);
       }
       
       // 添加到集合
       if (!line.IsEmpty()) {
           m_gfwDomains.insert(line.MakeLower());
           count++;
       }
   }
   
4. 关闭文件并返回
   file.Close();
   TRACE(_T("加载了 %d 个GFW域名\n"), count);
   return TRUE;
```

#### LoadPoisonIPs 实现
```
功能：加载 bogus-nxdomain.china.conf 文件
文件格式：bogus-nxdomain=IP

流程：
1. 构造文件路径
   CString filePath = GetRulesDirectory() + _T("\\bogus-nxdomain.china.conf");

2. 打开文件
   CStdioFile file;
   if (!file.Open(filePath, CFile::modeRead | CFile::typeText)) {
       return FALSE;
   }

3. 逐行读取
   CString line;
   int count = 0;
   while (file.ReadString(line)) {
       line.Trim();
       
       // 跳过空行和注释
       if (line.IsEmpty() || line[0] == '#') continue;
       
       // 查找 bogus-nxdomain=
       int pos = line.Find(_T("bogus-nxdomain="));
       if (pos != -1) {
           CString ip = line.Mid(pos + 15);  // 15 = strlen("bogus-nxdomain=")
           ip.Trim();
           
           if (!ip.IsEmpty()) {
               m_poisonIPs.insert(ip);
               count++;
           }
       }
   }
   
4. 关闭文件并返回
   file.Close();
   TRACE(_T("加载了 %d 个污染IP\n"), count);
   return TRUE;
```

#### IsChineseDomain 实现
```
功能：判断域名是否在中国域名白名单中
参数：domain - 要查询的域名（如 www.baidu.com）

流程：
1. 标准化域名（转小写）
   CString normalizedDomain = domain;
   normalizedDomain.MakeLower();

2. 直接匹配
   if (m_chinaDomains.count(normalizedDomain) > 0) {
       return TRUE;
   }

3. 提取主域名匹配（www.baidu.com -> baidu.com）
   CString mainDomain = ExtractMainDomain(normalizedDomain);
   if (m_chinaDomains.count(mainDomain) > 0) {
       return TRUE;
   }

4. 通配符匹配（.cn 后缀）
   for (const auto& pattern : m_chinaDomains) {
       if (pattern[0] == '.' && normalizedDomain.Right(pattern.GetLength()) == pattern) {
           return TRUE;  // 匹配 .cn, .com.cn 等
       }
       if (MatchDomain(normalizedDomain, pattern)) {
           return TRUE;
       }
   }

5. 返回 FALSE
```

#### MatchDomain 实现
```
功能：域名模式匹配（支持通配符）
示例：
  MatchDomain("www.google.com", "google.com") -> TRUE
  MatchDomain("api.google.com", "google.com") -> TRUE
  MatchDomain("www.google.com", ".com") -> TRUE

实现：
BOOL MatchDomain(const CString& domain, const CString& pattern) const {
    // 完全匹配
    if (domain == pattern) return TRUE;
    
    // 如果 pattern 以 . 开头，匹配后缀
    if (pattern[0] == '.') {
        return domain.Right(pattern.GetLength()) == pattern;
    }
    
    // 如果 domain 以 pattern 结尾（子域名匹配）
    // www.google.com 匹配 google.com
    if (domain.Right(pattern.GetLength()) == pattern) {
        // 确保是完整的域名组件匹配
        int pos = domain.GetLength() - pattern.GetLength() - 1;
        if (pos < 0 || domain[pos] == '.') {
            return TRUE;
        }
    }
    
    return FALSE;
}
```

#### ExtractMainDomain 实现
```
功能：提取主域名
示例：
  www.baidu.com -> baidu.com
  api.google.com -> google.com
  blog.example.co.uk -> example.co.uk (需要处理二级后缀)

实现：
CString ExtractMainDomain(const CString& domain) const {
    // 处理二级后缀（.com.cn, .co.uk 等）
    static std::set<CString> secondLevelTLDs = {
        _T("com.cn"), _T("net.cn"), _T("org.cn"), _T("gov.cn"),
        _T("co.uk"), _T("co.jp"), _T("co.kr")
    };
    
    // 检查是否有二级后缀
    for (const auto& tld : secondLevelTLDs) {
        if (domain.Right(tld.GetLength() + 1) == _T(".") + tld) {
            // 找到二级后缀，提取前面的部分
            int pos = domain.GetLength() - tld.GetLength() - 1;
            int prevDot = domain.Left(pos).ReverseFind('.');
            if (prevDot != -1) {
                return domain.Mid(prevDot + 1);
            }
            return domain;  // 已经是主域名
        }
    }
    
    // 普通后缀处理
    int lastDot = domain.ReverseFind('.');
    if (lastDot == -1) return domain;
    
    int secondLastDot = domain.Left(lastDot).ReverseFind('.');
    if (secondLastDot == -1) return domain;  // 已经是主域名
    
    return domain.Mid(secondLastDot + 1);
}
```

---

## 3. 关联域名检测器（增强版）

### 3.1 类定义
```cpp
// AssociatedDomainDetector.h
class CAssociatedDomainDetector {
public:
    CAssociatedDomainDetector();
    ~CAssociatedDomainDetector();
    
    // 初始化（加载预定义规则）
    void Initialize();
    
    // 记录查询并检测关联
    void RecordQuery(const CString& domain, BOOL isGFW);
    
    // 判断是否应该走海外DNS
    BOOL ShouldUseOverseasDNS(const CString& domain);
    
    // 手动学习关联规则
    void LearnAssociation(const CString& gfwDomain, 
                         const std::vector<CString>& relatedDomains);
    
    // 清理过期的临时关联
    void CleanupExpired();
    
    // 保存/加载持久化规则
    BOOL SaveAssociations();
    BOOL LoadAssociations();
    
private:
    // 查询记录
    struct QueryRecord {
        CString domain;
        DWORD timestamp;
        BOOL isGFW;
    };
    
    std::deque<QueryRecord> m_recentQueries;  // 滑动窗口（1秒）
    
    // 临时关联（短期有效）
    std::map<CString, DWORD> m_tempAssociations;  // domain -> expire_time
    
    // 永久关联规则
    std::map<CString, std::set<CString>> m_permanentRules;  // main_domain -> related_domains
    
    // 预定义生态系统规则
    std::map<CString, std::vector<CString>> m_ecosystemRules;
    
    CRITICAL_SECTION m_cs;
    
    // 初始化预定义生态系统规则
    void InitEcosystemRules();
    
    // 提取主域名
    CString ExtractMainDomain(const CString& domain);
    
    // 检查是否匹配生态系统规则
    BOOL MatchEcosystemRule(const CString& domain);
};
```

### 3.2 实现要点

#### InitEcosystemRules 实现
```
功能：初始化预定义的生态系统关联规则

预定义规则：
m_ecosystemRules[_T("google.com")] = {
    _T("*.google.com"),
    _T("*.googleapis.com"),
    _T("*.gstatic.com"),
    _T("*.googleusercontent.com"),
    _T("*.ggpht.com"),
    _T("*.youtube.com"),
    _T("*.ytimg.com"),
    _T("*.googlevideo.com")
};

m_ecosystemRules[_T("facebook.com")] = {
    _T("*.facebook.com"),
    _T("*.fbcdn.net"),
    _T("*.facebook.net"),
    _T("*.instagram.com"),
    _T("*.cdninstagram.com"),
    _T("*.whatsapp.com"),
    _T("*.whatsapp.net")
};

m_ecosystemRules[_T("twitter.com")] = {
    _T("*.twitter.com"),
    _T("*.twimg.com"),
    _T("*.t.co")
};

m_ecosystemRules[_T("github.com")] = {
    _T("*.github.com"),
    _T("*.githubusercontent.com"),
    _T("*.githubassets.com"),
    _T("*.github.io")
};

m_ecosystemRules[_T("cloudflare.com")] = {
    _T("*.cloudflare.com"),
    _T("*.cloudflarestream.com"),
    _T("*.workers.dev")
};

m_ecosystemRules[_T("amazon.com")] = {
    _T("*.amazon.com"),
    _T("*.amazonaws.com"),
    _T("*.cloudfront.net")
};

// 添加更多生态系统...
```

#### RecordQuery 实现
```
功能：记录查询并检测时间窗口关联

流程：
1. 加锁
   EnterCriticalSection(&m_cs);

2. 记录查询
   DWORD now = GetTickCount();
   QueryRecord record;
   record.domain = domain;
   record.timestamp = now;
   record.isGFW = isGFW;
   m_recentQueries.push_back(record);

3. 清理超过1秒的旧记录
   while (!m_recentQueries.empty() && 
          now - m_recentQueries.front().timestamp > 1000) {
       m_recentQueries.pop_front();
   }

4. 检测关联：如果时间窗口内有GFW域名
   BOOL hasGFWInWindow = FALSE;
   for (const auto& rec : m_recentQueries) {
       if (rec.isGFW) {
           hasGFWInWindow = TRUE;
           break;
       }
   }

5. 如果有GFW域名，标记其他域名为临时关联
   if (hasGFWInWindow) {
       for (const auto& rec : m_recentQueries) {
           if (!rec.isGFW && rec.domain != domain) {
               // 临时关联，有效期30秒
               m_tempAssociations[rec.domain] = now + 30000;
               
               TRACE(_T("时间窗口关联: %s <-> GFW域名\n"), rec.domain);
           }
       }
   }

6. 解锁
   LeaveCriticalSection(&m_cs);
```

#### ShouldUseOverseasDNS 实现
```
功能：判断域名是否应该走海外DNS
优先级：生态系统规则 > 临时关联 > 永久规则

流程：
1. 加锁
   EnterCriticalSection(&m_cs);

2. 检查生态系统规则（最高优先级）
   if (MatchEcosystemRule(domain)) {
       LeaveCriticalSection(&m_cs);
       return TRUE;
   }

3. 检查临时关联
   if (m_tempAssociations.count(domain)) {
       DWORD now = GetTickCount();
       if (now < m_tempAssociations[domain]) {
           LeaveCriticalSection(&m_cs);
           return TRUE;  // 临时关联仍有效
       } else {
           m_tempAssociations.erase(domain);  // 过期删除
       }
   }

4. 检查永久规则
   CString mainDomain = ExtractMainDomain(domain);
   for (const auto& rule : m_permanentRules) {
       if (rule.second.count(domain) || rule.second.count(mainDomain)) {
           LeaveCriticalSection(&m_cs);
           return TRUE;
       }
   }

5. 解锁并返回
   LeaveCriticalSection(&m_cs);
   return FALSE;
```

#### MatchEcosystemRule 实现
```
功能：检查域名是否匹配预定义生态系统规则

流程：
for (const auto& ecosystem : m_ecosystemRules) {
    // ecosystem.first 是主域名（如 google.com）
    // ecosystem.second 是关联域名模式列表
    
    for (const auto& pattern : ecosystem.second) {
        if (pattern[0] == '*') {
            // 通配符模式：*.google.com
            CString suffix = pattern.Mid(1);  // 去掉 *
            if (domain.Right(suffix.GetLength()) == suffix) {
                TRACE(_T("生态系统匹配: %s -> %s\n"), domain, ecosystem.first);
                return TRUE;
            }
        } else {
            // 精确匹配
            if (domain == pattern) {
                return TRUE;
            }
        }
    }
}
return FALSE;
```

---

## 4. 域名分类器（核心路由逻辑）

### 4.1 类定义
```cpp
// DomainClassifier.h
enum DomainType {
    DOMESTIC,      // 国内域名
    FOREIGN,       // 国外域名
    ASSOCIATED,    // 关联域名（跟随GFW域名）
    UNKNOWN        // 未知域名
};

class CDomainClassifier {
public:
    CDomainClassifier();
    ~CDomainClassifier();
    
    // 初始化（加载规则）
    BOOL Initialize();
    
    // 域名分类（核心函数）
    DomainType ClassifyDomain(const CString& domain);
    
    // 记录查询（用于关联检测）
    void RecordQuery(const CString& domain, DomainType type);
    
    // IP污染检测
    BOOL IsPoisonedIP(const CString& ip);
    
    // 获取统计信息
    int GetDomesticCount() const { return m_domesticCount; }
    int GetForeignCount() const { return m_foreignCount; }
    int GetUnknownCount() const { return m_unknownCount; }
    
private:
    CDomainRuleLoader m_ruleLoader;
    CAssociatedDomainDetector m_associationDetector;
    
    // 统计
    int m_domesticCount;
    int m_foreignCount;
    int m_unknownCount;
    
    CRITICAL_SECTION m_cs;
};
```

### 4.2 ClassifyDomain 实现（核心逻辑）
```
功能：根据规则对域名进行分类
这是整个系统最核心的函数！

流程（严格按优先级）：

DomainType ClassifyDomain(const CString& domain) {
    EnterCriticalSection(&m_cs);
    
    CString normalizedDomain = domain;
    normalizedDomain.MakeLower();
    normalizedDomain.Trim();
    
    // ========== 优先级1: 中国域名白名单 ==========
    // 这是最高优先级，因为国内域名必须走国内DNS
    if (m_ruleLoader.IsChineseDomain(normalizedDomain)) {
        m_domesticCount++;
        LeaveCriticalSection(&m_cs);
        TRACE(_T("[分类] %s -> 国内域名（白名单）\n"), domain);
        return DOMESTIC;
    }
    
    // ========== 优先级2: GFW黑名单 ==========
    // 明确在GFW列表中的域名
    if (m_ruleLoader.IsGFWDomain(normalizedDomain)) {
        m_foreignCount++;
        
        // 记录到关联检测器
        m_associationDetector.RecordQuery(normalizedDomain, TRUE);
        
        LeaveCriticalSection(&m_cs);
        TRACE(_T("[分类] %s -> 国外域名（GFW黑名单）\n"), domain);
        return FOREIGN;
    }
    
    // ========== 优先级3: 关联域名检测 ==========
    // 检查是否应该跟随GFW域名（时间窗口关联或生态系统规则）
    if (m_associationDetector.ShouldUseOverseasDNS(normalizedDomain)) {
        m_foreignCount++;
        
        // 记录非GFW查询
        m_associationDetector.RecordQuery(normalizedDomain, FALSE);
        
        LeaveCriticalSection(&m_cs);
        TRACE(_T("[分类] %s -> 关联域名（跟随海外）\n"), domain);
        return ASSOCIATED;
    }
    
    // ========== 优先级4: 特殊模式匹配 ==========
    // 某些明显的模式可以直接判断
    
    // 4.1 明显的中国域名
    if (normalizedDomain.Right(3) == _T(".cn") ||
        normalizedDomain.Find(_T(".gov.cn")) != -1 ||
        normalizedDomain.Find(_T(".edu.cn")) != -1) {
        m_domesticCount++;
        LeaveCriticalSection(&m_cs);
        TRACE(_T("[分类] %s -> 国内域名（.cn后缀）\n"), domain);
        return DOMESTIC;
    }
    
    // 4.2 本地域名
    if (normalizedDomain == _T("localhost") ||
        normalizedDomain.Find(_T(".local")) != -1 ||
        normalizedDomain.Find(_T(".lan")) != -1) {
        m_domesticCount++;
        LeaveCriticalSection(&m_cs);
        return DOMESTIC;
    }
    
    // ========== 优先级5: 未知域名 ==========
    m_unknownCount++;
    
    // 记录非GFW查询（用于时间窗口关联）
    m_associationDetector.RecordQuery(normalizedDomain, FALSE);
    
    LeaveCriticalSection(&m_cs);
    TRACE(_T("[分类] %s -> 未知域名（需要双向查询）\n"), domain);
    return UNKNOWN;
}
```

---

## 5. DNS查询路由器

### 5.1 类定义
```cpp
// DNSQueryRouter.h
class CDNSQueryRouter {
public:
    CDNSQueryRouter();
    ~CDNSQueryRouter();
    
    // 初始化
    BOOL Initialize(CDNSServerManager* pServerManager,
                   CDomainClassifier* pClassifier);
    
    // 路由查询（核心函数）
    CString RouteQuery(const CString& domain, int& outLatency);
    
private:
    CDNSServerManager* m_pServerManager;
    CDomainClassifier* m_pClassifier;
    
    // 查询方法
    CString QueryDomesticDNS(const CString& domain, int& latency);
    CString QueryOverseasDNS(const CString& domain, int& latency);
    CString QueryBothAndValidate(const CString& domain, int& latency);
    
    // 污染检测
    BOOL ValidateResult(const CString& ip);
    
    // 实际DNS查询
    CString DoQuery(const CString& dnsServer, const CString& domain, int& latency);
};
```

### 5.2 RouteQuery 实现
```
功能：根据域名分类路由DNS查询

CString RouteQuery(const CString& domain, int& outLatency) {
    // 1. 分类域名
    DomainType type = m_pClassifier->ClassifyDomain(domain);
    
    // 2. 根据分类选择查询方式
    CString result;
    
    switch (type) {
    case DOMESTIC:
        // 国内域名 -> 国内DNS
        result = QueryDomesticDNS(domain, outLatency);
        break;
        
    case FOREIGN:
    case ASSOCIATED:
        // 国外域名或关联域名 -> 海外DNS
        result = QueryOverseasDNS(domain, outLatency);
        break;
        
    case UNKNOWN:
        // 未知域名 -> 双向查询+验证
        result = QueryBothAndValidate(domain, outLatency);
        break;
    }
    
    return result;
}
```

### 5.3 QueryBothAndValidate 实现
```
功能：对未知域名进行双向查询和污染检测

流程：
1. 同时向国内DNS和海外DNS查询（并发）
   std::future<CString> domesticResult = std::async([&]() {
       int latency;
       return QueryDomesticDNS(domain, latency);
   });
   
   std::future<CString> overseasResult = std::async([&]() {
       int latency;
       return QueryOverseasDNS(domain, latency);
   });

2. 等待结果（超时3秒）
   auto domesticIP = domesticResult.get();
   auto overseasIP = overseasResult.get();

3. 优先使用国内结果（速度快）
   if (!domesticIP.IsEmpty()) {
       // 验证是否污染
       if (ValidateResult(domesticIP)) {
           // 未污染，使用国内结果
           TRACE(_T("[路由] %s -> 国内DNS结果: %s\n"), domain, domesticIP);
           return domesticIP;
       }
   }

4. 国内结果污染或失败，使用海外结果
   if (!overseasIP.IsEmpty()) {
       TRACE(_T("[路由] %s -> 海外DNS结果: %s (国内结果被污染)\n"), 
             domain, overseasIP);
       
       // 标记该域名为海外域名（学习）
       // 下次直接走海外DNS
       
       return overseasIP;
   }

5. 都失败，返回空
   return _T("");
```

### 5.4 ValidateResult 实现
```
功能：验证DNS查询结果是否被污染

BOOL ValidateResult(const CString& ip) {
    // 1. 检查是否在污染IP黑名单中
    if (m_pClassifier->IsPoisonedIP(ip)) {
        TRACE(_T("[污染检测] IP %s 在黑名单中\n"), ip);
        return FALSE;
    }
    
    // 2. 检查是否是保留IP（通常是污染）
    // 10.x.x.x, 192.168.x.x, 172.16.x.x 等内网IP
    if (IsPrivateIP(ip)) {
        TRACE(_T("[污染检测] IP %s 是内网地址\n"), ip);
        return FALSE;
    }
    
    // 3. 检查是否是0.0.0.0或255.255.255.255
    if (ip == _T("0.0.0.0") || ip == _T("255.255.255.255")) {
        return FALSE;
    }
    
    return TRUE;
}
```

---

## 6. 主对话框集成

### 6.1 在 CStupidDNSDlg 中添加
```cpp
class CStupidDNSDlg : public CDialogEx {
private:
    CDNSServerManager m_dnsManager;
    CDomainClassifier m_domainClassifier;
    CDNSQueryRouter m_queryRouter;
    
    // DNS服务线程
    static UINT DNSServerThread(LPVOID pParam);
    void HandleDNSQuery(const char* queryPacket, int queryLen,
                       sockaddr_in& clientAddr);
};
```

### 6.2 初始化流程
```cpp
BOOL CStupidDNSDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    
    // ... 其他初始化 ...
    
    // 1. 初始化域名分类器（加载规则文件）
    if (!m_domainClassifier.Initialize()) {
        AfxMessageBox(_T("加载域名规则失败！"));
    }
    
    // 2. 初始化DNS服务器管理器
    InitializeDNSServers();
    
    // 3. 初始化查询路由器
    m_queryRouter.Initialize(&m_dnsManager, &m_domainClassifier);
    
    // 4. 显示加载的规则数量
    CString msg;
    msg.Format(_T("加载规则：中国域名 %d 条，GFW域名 %d 条，污染IP %d 个"),
        m_domainClassifier.GetRuleLoader().GetChinaDomainCount(),
        m_domainClassifier.GetRuleLoader().GetGFWDomainCount(),
        m_domainClassifier.GetRuleLoader().GetPoisonIPCount());
    AddLogEntry(_T("系统"), msg, _T("INFO"), _T("-"), _T("-"));
    
    return TRUE;
}
```

---

## 7. 配置和缓存

### 7.1 关联规则持久化
```
文件：associated_domains.dat
格式：二进制或JSON

内容：
- 临时关联规则（过期时间）
- 学习到的永久规则
- 用户自定义规则
```

### 7.2 自动保存
```
定时保存（每5分钟）：
- 关联规则
- 统计信息
- 域名分类缓存
```

---

## 8. 测试用例

### 8.1 规则加载测试
```cpp
void TestRuleLoader() {
    CDomainRuleLoader loader;
    loader.LoadAllRules();
    
    // 测试中国域名
    ASSERT(loader.IsChineseDomain(_T("baidu.com")) == TRUE);
    ASSERT(loader.IsChineseDomain(_T("qq.com")) == TRUE);
    ASSERT(loader.IsChineseDomain(_T("taobao.com")) == TRUE);
    
    // 测试GFW域名
    ASSERT(loader.IsGFWDomain(_T("google.com")) == TRUE);
    ASSERT(loader.IsGFWDomain(_T("facebook.com")) == TRUE);
    
    // 测试污染IP
    ASSERT(loader.IsPoisonIP(_T("243.185.187.39")) == TRUE);
}
```

### 8.2 分类测试
```cpp
void TestClassifier() {
    CDomainClassifier classifier;
    classifier.Initialize();
    
    // 国内域名
    ASSERT(classifier.ClassifyDomain(_T("baidu.com")) == DOMESTIC);
    ASSERT(classifier.ClassifyDomain(_T("www.gov.cn")) == DOMESTIC);
    
    // 国外域名
    ASSERT(classifier.ClassifyDomain(_T("google.com")) == FOREIGN);
    ASSERT(classifier.ClassifyDomain(_T("youtube.com")) == FOREIGN);
    
    // 关联域名（需要先查询GFW域名）
    classifier.ClassifyDomain(_T("google.com"));  // GFW域名
    Sleep(100);
    // 1秒内查询关联域名
    ASSERT(classifier.ClassifyDomain(_T("gstatic.com")) == ASSOCIATED);
}
```

---

## 9. 日志输出示例
```
[2025-01-24 14:30:00] [系统] 加载规则：中国域名 5234 条，GFW域名 3421 条，污染IP 78 个
[2025-01-24 14:30:01] [分类] www.baidu.com -> 国内域名（白名单）
[2025-01-24 14:30:02] [路由] www.baidu.com -> 国内DNS: 110.242.68.66 (15ms)
[2025-01-24 14:30:05] [分类] www.google.com -> 国外域名（GFW黑名单）
[2025-01-24 14:30:06] [路由] www.google.com -> 海外DNS: 142.250.185.46 (52ms)
[2025-01-24 14:30:06] [分类] gstatic.com -> 关联域名（生态系统规则）
[2025-01-24 14:30:07] [路由] gstatic.com -> 海外DNS: 142.251.42.99 (48ms)
[2025-01-24 14:30:10] [分类] unknown-site.com -> 未知域名（需要双向查询）
[2025-01-24 14:30:11] [污染检测] 国内DNS返回: 243.185.187.39（污染IP）
[2025-01-24 14:30:12] [路由] unknown-site.com -> 海外DNS: 185.199.108.153 (78ms)
```

---

## 10. 文件清单

需要创建的文件：
1. DomainRuleLoader.h / DomainRuleLoader.cpp
2. AssociatedDomainDetector.h / AssociatedDomainDetector.cpp
3. DomainClassifier.h / DomainClassifier.cpp
4. DNSQueryRouter.h / DNSQueryRouter.cpp

修改的文件：
5. StupidDNSDlg.h / StupidDNSDlg.cpp - 集成所有模块

规则文件（已下载）：
6. rules\accelerated-domains.china.conf
7. rules\bogus-nxdomain.china.conf
8. rules\gfw.txt

---

## 11. 性能要求

- 规则加载时间：< 2秒
- 域名分类时间：< 1ms
- 内存占用：规则文件 < 50MB
- 并发查询支持：> 100 QPS

---

## 12. 代码规范

- Unicode 字符集
- 线程安全（CRITICAL_SECTION）
- 详细的中文注释
- 错误处理完善
- 日志记录关键操作

---

请根据此需求文档生成完整的域名分类和路由模块代码。
```

---

把这个发给 Copilot：
```
@workspace 请根据 DOMAIN_CLASSIFICATION_REQUIREMENTS.md 生成完整的域名分类和智能路由模块代码，包括：
1. 规则文件加载器
2. 关联域名检测器（增强版）
3. 域名分类器（核心路由逻辑）
4. DNS查询路由器
5. 主对话框集成代码

要求：
1. 严格按照文档中的优先级逻辑实现
2. 支持时间窗口关联检测（1秒内）
3. 支持生态系统规则（Google、Facebook等）
4. 支持双向查询+污染检测
5. 线程安全，性能优化
6. 代码可以直接编译运行

请生成所有相关的 .h 和 .cpp 文件。