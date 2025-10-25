# StupidDNS 从 public-dns.info 抓取DNS服务器需求

## 模块概述
从 https://public-dns.info 网站抓取全球公共DNS服务器信息，筛选可靠性>80%的服务器，按地理位置分类。

---

## 1. 目标网站分析

### 1.1 public-dns.info 数据来源
网站提供两种获取方式：

#### 方式1：CSV文件（推荐，最简单）
```
URL: https://public-dns.info/nameservers.csv
格式: CSV (逗号分隔)
更新频率: 每小时

CSV列定义：
ip,name,country_id,city,version,error,dnssec,reliability,checked_at,created_at

示例数据：
1.1.1.1,Cloudflare,US,San Francisco,4,0.00,Yes,1.00,2024-01-24 10:30:00,2020-01-01 00:00:00
8.8.8.8,Google,US,Mountain View,4,0.00,Yes,0.99,2024-01-24 10:25:00,2009-12-03 00:00:00
223.5.5.5,AliDNS,CN,Hangzhou,4,0.00,No,0.98,2024-01-24 10:20:00,2014-06-06 00:00:00
```

#### 方式2：HTML页面抓取（备选）
```
URL: https://public-dns.info/nameserver/{country_code}.html
例如: https://public-dns.info/nameserver/us.html (美国)
     https://public-dns.info/nameserver/jp.html (日本)
```

**推荐使用方式1（CSV文件）**，更简单可靠。

---

## 2. DNS抓取类设计

### 2.1 类定义
```cpp
// DNSFetcher.h
#include <string>
#include <vector>
#include <winhttp.h>

class CDNSFetcher {
public:
    CDNSFetcher();
    ~CDNSFetcher();
    
    // 从 public-dns.info 获取DNS列表
    BOOL FetchDNSList(std::vector<DNSServerInfo>& outServers);
    
    // 下载CSV文件
    BOOL DownloadCSV(CString& outContent);
    
    // 解析CSV内容
    BOOL ParseCSV(const CString& csvContent, 
                  std::vector<DNSServerInfo>& outServers);
    
    // 过滤和分类
    void FilterByReliability(std::vector<DNSServerInfo>& servers, 
                             double minReliability = 0.8);
    
    void FilterByRegion(const std::vector<DNSServerInfo>& allServers,
                       std::vector<DNSServerInfo>& usServers,
                       std::vector<DNSServerInfo>& euServers,
                       std::vector<DNSServerInfo>& asiaServers);
    
    // 设置超时时间
    void SetTimeout(int timeoutMS) { m_timeout = timeoutMS; }
    
    // 设置User-Agent
    void SetUserAgent(const CString& ua) { m_userAgent = ua; }
    
private:
    int m_timeout;          // 超时时间（毫秒），默认10000
    CString m_userAgent;    // User-Agent
    
    // HTTP请求辅助函数
    BOOL HttpGet(const CString& url, CString& outContent);
    
    // CSV解析辅助函数
    std::vector<CString> SplitCSVLine(const CString& line);
    CString Trim(const CString& str);
    
    // 国家代码到地区映射
    CString GetRegionByCountryCode(const CString& countryCode);
};
```

### 2.2 实现要点

#### DownloadCSV 实现
```
功能：从 public-dns.info 下载CSV文件
URL: https://public-dns.info/nameservers.csv

流程：
1. 初始化 WinHTTP
   HINTERNET hSession = WinHttpOpen(
       L"StupidDNS/1.0",
       WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
       WINHTTP_NO_PROXY_NAME,
       WINHTTP_NO_PROXY_BYPASS,
       0);

2. 连接服务器
   HINTERNET hConnect = WinHttpConnect(
       hSession,
       L"public-dns.info",
       INTERNET_DEFAULT_HTTPS_PORT,
       0);

3. 创建请求
   HINTERNET hRequest = WinHttpOpenRequest(
       hConnect,
       L"GET",
       L"/nameservers.csv",
       NULL,
       WINHTTP_NO_REFERER,
       WINHTTP_DEFAULT_ACCEPT_TYPES,
       WINHTTP_FLAG_SECURE);

4. 发送请求
   WinHttpSendRequest(...)
   WinHttpReceiveResponse(...)

5. 读取数据
   char buffer[4096];
   DWORD bytesRead;
   CString result;
   while (WinHttpReadData(hRequest, buffer, 4096, &bytesRead)) {
       if (bytesRead == 0) break;
       result += CString(buffer, bytesRead);
   }

6. 清理资源
   WinHttpCloseHandle(...)

7. 返回 CSV 内容字符串

错误处理：
- 连接失败：返回 FALSE
- 超时：返回 FALSE
- HTTP错误码（非200）：返回 FALSE
```

#### ParseCSV 实现
```
功能：解析CSV内容，提取DNS服务器信息
参数：csvContent - CSV文件内容字符串

流程：
1. 按行分割 CSV 内容
   std::vector<CString> lines = SplitByNewLine(csvContent);

2. 跳过第一行（表头）
   for (int i = 1; i < lines.size(); i++)

3. 对每一行：
   a. 分割为字段：SplitCSVLine(line)
      字段顺序：ip, name, country_id, city, version, error, dnssec, reliability, checked_at, created_at
   
   b. 验证字段数量（至少8个字段）
   
   c. 创建 DNSServerInfo 对象：
      DNSServerInfo info;
      info.ip = fields[0];              // IP地址
      info.name = fields[1];            // 名称
      CString countryCode = fields[2];  // 国家代码
      info.location = GetRegionByCountryCode(countryCode);  // 转换为地区
      
      // 解析可靠性（reliability列）
      double reliability = _ttof(fields[7]);
      info.reliability = reliability * 100;  // 转换为百分比
   
   d. 添加到结果列表

4. 返回解析结果
```

#### SplitCSVLine 实现
```
功能：分割CSV行，处理引号包裹的字段
CSV格式特点：
- 字段用逗号分隔
- 字段可能被双引号包裹："field value"
- 字段内的引号用两个引号转义：""

实现逻辑：
std::vector<CString> SplitCSVLine(const CString& line) {
    std::vector<CString> fields;
    CString field;
    BOOL inQuotes = FALSE;
    
    for (int i = 0; i < line.GetLength(); i++) {
        TCHAR ch = line[i];
        
        if (ch == '\"') {
            // 处理引号
            if (inQuotes && i + 1 < line.GetLength() && line[i+1] == '\"') {
                // 转义的引号
                field += '\"';
                i++;  // 跳过下一个引号
            } else {
                // 引号开始/结束
                inQuotes = !inQuotes;
            }
        }
        else if (ch == ',' && !inQuotes) {
            // 字段分隔符（不在引号内）
            fields.push_back(Trim(field));
            field.Empty();
        }
        else {
            field += ch;
        }
    }
    
    // 添加最后一个字段
    fields.push_back(Trim(field));
    
    return fields;
}
```

#### GetRegionByCountryCode 实现
```
功能：将国家代码映射到地区
参数：countryCode - 两字母国家代码（如 US, JP, DE）

映射规则：
美国及北美：
US, CA, MX -> "美国"

欧洲：
GB, DE, FR, NL, SE, NO, FI, DK, IT, ES, CH, AT, BE, PL, CZ, IE, PT, GR, RO, HU
-> "欧洲"

亚洲（不包括中国）：
JP, KR, SG, TW, HK, IN, TH, MY, PH, ID, VN -> "亚洲"

中国：
CN -> "中国"

其他：
-> "其他"

实现：
CString GetRegionByCountryCode(const CString& code) {
    static std::map<CString, CString> regionMap;
    
    // 首次调用时初始化
    if (regionMap.empty()) {
        // 北美
        regionMap[_T("US")] = _T("美国");
        regionMap[_T("CA")] = _T("美国");
        
        // 欧洲
        regionMap[_T("GB")] = _T("欧洲");
        regionMap[_T("DE")] = _T("欧洲");
        regionMap[_T("FR")] = _T("欧洲");
        regionMap[_T("NL")] = _T("欧洲");
        regionMap[_T("SE")] = _T("欧洲");
        // ... 添加其他欧洲国家
        
        // 亚洲
        regionMap[_T("JP")] = _T("亚洲");
        regionMap[_T("KR")] = _T("亚洲");
        regionMap[_T("SG")] = _T("亚洲");
        regionMap[_T("TW")] = _T("亚洲");
        regionMap[_T("HK")] = _T("亚洲");
        // ... 添加其他亚洲国家
        
        // 中国
        regionMap[_T("CN")] = _T("中国");
    }
    
    auto it = regionMap.find(code.MakeUpper());
    if (it != regionMap.end()) {
        return it->second;
    }
    return _T("其他");
}
```

#### FilterByReliability 实现
```
功能：过滤出可靠性>=指定值的DNS服务器
参数：servers - DNS服务器列表（引用，会被修改）
      minReliability - 最低可靠性（0-1之间，默认0.8即80%）

流程：
1. 使用 std::remove_if 删除不符合条件的服务器
   servers.erase(
       std::remove_if(servers.begin(), servers.end(),
           [minReliability](const DNSServerInfo& info) {
               return info.reliability < minReliability * 100;
           }),
       servers.end()
   );

2. 记录过滤后的数量
```

#### FilterByRegion 实现
```
功能：将DNS列表按地区分类
参数：allServers - 所有DNS服务器
      usServers - 输出：美国DNS列表
      euServers - 输出：欧洲DNS列表
      asiaServers - 输出：亚洲DNS列表

流程：
usServers.clear();
euServers.clear();
asiaServers.clear();

for (const auto& server : allServers) {
    if (server.location == _T("美国")) {
        usServers.push_back(server);
    }
    else if (server.location == _T("欧洲")) {
        euServers.push_back(server);
    }
    else if (server.location == _T("亚洲")) {
        asiaServers.push_back(server);
    }
    // 忽略中国和其他地区
}
```

---

## 3. 集成到 DNSServerManager

### 3.1 修改 InitOverseasDNSList
```cpp
void CDNSServerManager::InitOverseasDNSList() {
    m_overseasServers.clear();
    
    // 方案1：从 public-dns.info 动态获取（推荐）
    CDNSFetcher fetcher;
    std::vector<DNSServerInfo> allServers;
    
    if (fetcher.FetchDNSList(allServers)) {
        // 成功获取，过滤可靠性>80%
        fetcher.FilterByReliability(allServers, 0.8);
        
        // 按地区分类
        std::vector<DNSServerInfo> usServers, euServers, asiaServers;
        fetcher.FilterByRegion(allServers, usServers, euServers, asiaServers);
        
        // 添加到候选列表
        // 美国DNS：取前8个
        int count = min(8, (int)usServers.size());
        for (int i = 0; i < count; i++) {
            m_overseasServers.push_back(usServers[i]);
        }
        
        // 欧洲DNS：取前4个
        count = min(4, (int)euServers.size());
        for (int i = 0; i < count; i++) {
            m_overseasServers.push_back(euServers[i]);
        }
        
        // 亚洲DNS：取前4个
        count = min(4, (int)asiaServers.size());
        for (int i = 0; i < count; i++) {
            m_overseasServers.push_back(asiaServers[i]);
        }
        
        TRACE(_T("从 public-dns.info 获取了 %d 个海外DNS\n"), 
              m_overseasServers.size());
    }
    else {
        // 获取失败，使用备用硬编码列表
        TRACE(_T("无法从 public-dns.info 获取DNS，使用备用列表\n"));
        LoadFallbackOverseasDNS();
    }
}

void CDNSServerManager::LoadFallbackOverseasDNS() {
    // 备用硬编码列表（与之前的 InitOverseasDNSList 相同）
    DNSServerInfo cloudflare;
    cloudflare.name = _T("Cloudflare");
    cloudflare.ip = _T("1.1.1.1");
    cloudflare.location = _T("美国");
    cloudflare.reliability = 99.9;
    m_overseasServers.push_back(cloudflare);
    
    // ... 添加其他备用DNS
}
```

---

## 4. 缓存机制

### 4.1 缓存策略
```
为了避免每次启动都下载CSV（耗时且占带宽），实现缓存：

缓存文件：dns_cache.csv
缓存路径：程序目录\dns_cache.csv
缓存有效期：24小时

逻辑：
1. 检查缓存文件是否存在
2. 检查缓存文件修改时间是否<24小时
3. 如果缓存有效：直接读取缓存文件
4. 如果缓存无效：下载新CSV，保存到缓存文件
```

### 4.2 实现
```cpp
BOOL CDNSFetcher::FetchDNSList(std::vector<DNSServerInfo>& outServers) {
    CString cacheFile = GetModulePath() + _T("\\dns_cache.csv");
    CString csvContent;
    
    // 检查缓存
    if (IsCacheValid(cacheFile)) {
        // 从缓存读取
        if (ReadCacheFile(cacheFile, csvContent)) {
            TRACE(_T("使用缓存的DNS列表\n"));
            return ParseCSV(csvContent, outServers);
        }
    }
    
    // 下载新数据
    if (DownloadCSV(csvContent)) {
        // 保存到缓存
        SaveCacheFile(cacheFile, csvContent);
        return ParseCSV(csvContent, outServers);
    }
    
    return FALSE;
}

BOOL CDNSFetcher::IsCacheValid(const CString& cacheFile) {
    // 检查文件是否存在
    if (GetFileAttributes(cacheFile) == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }
    
    // 检查文件修改时间
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(cacheFile, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    FindClose(hFind);
    
    // 转换为时间戳
    FILETIME ftNow;
    GetSystemTimeAsFileTime(&ftNow);
    
    ULARGE_INTEGER now, modified;
    now.LowPart = ftNow.dwLowDateTime;
    now.HighPart = ftNow.dwHighDateTime;
    modified.LowPart = findData.ftLastWriteTime.dwLowDateTime;
    modified.HighPart = findData.ftLastWriteTime.dwHighDateTime;
    
    // 计算时间差（100纳秒单位）
    ULONGLONG diff = now.QuadPart - modified.QuadPart;
    ULONGLONG hours = diff / 10000000 / 3600;  // 转换为小时
    
    return hours < 24;  // 24小时内有效
}
```

---

## 5. 错误处理与重试

### 5.1 下载失败处理
```cpp
BOOL CDNSFetcher::DownloadCSV(CString& outContent) {
    int retryCount = 3;
    int retryDelay = 2000;  // 2秒
    
    for (int i = 0; i < retryCount; i++) {
        if (HttpGet(_T("https://public-dns.info/nameservers.csv"), 
                    outContent)) {
            return TRUE;
        }
        
        TRACE(_T("下载失败，重试 %d/%d\n"), i + 1, retryCount);
        Sleep(retryDelay);
    }
    
    return FALSE;
}
```

### 5.2 网络错误提示
```
如果下载失败且没有缓存：
1. 记录详细错误日志
2. 使用备用硬编码DNS列表
3. 向用户显示提示："无法连接到 public-dns.info，使用内置DNS列表"
```

---

## 6. 进度回调

### 6.1 支持进度显示
```cpp
class CDNSFetcher {
public:
    void SetProgressCallback(
        std::function<void(CString)> callback) {
        m_progressCallback = callback;
    }

private:
    std::function<void(CString)> m_progressCallback;
    
    void UpdateProgress(const CString& message) {
        if (m_progressCallback) {
            m_progressCallback(message);
        }
    }
};

// 使用示例
fetcher.SetProgressCallback([this](CString msg) {
    SetDlgItemText(IDC_STATIC_STATUS, msg);
});

// 在 DownloadCSV 中调用
UpdateProgress(_T("正在连接 public-dns.info..."));
UpdateProgress(_T("正在下载DNS列表..."));
UpdateProgress(_T("正在解析数据..."));
```

---

## 7. 主对话框启动流程优化

### 7.1 异步加载
```cpp
// 避免阻塞UI线程
void CStupidDNSDlg::InitializeDNSServers() {
    // 显示加载提示
    SetDlgItemText(IDC_STATIC_STATUS, _T("正在初始化DNS服务器..."));
    
    // 在后台线程执行
    std::thread([this]() {
        BOOL success = m_dnsManager.PerformSpeedTest([this](CString status) {
            // 更新UI（需要PostMessage到主线程）
            PostMessage(WM_UPDATE_STATUS, 0, (LPARAM)new CString(status));
        });
        
        // 完成后通知主线程
        PostMessage(WM_DNS_INIT_COMPLETE, success, 0);
    }).detach();
}

// 处理消息
LRESULT CStupidDNSDlg::OnUpdateStatus(WPARAM wParam, LPARAM lParam) {
    CString* pStatus = (CString*)lParam;
    SetDlgItemText(IDC_STATIC_STATUS, *pStatus);
    delete pStatus;
    return 0;
}
```

---

## 8. 依赖库
```cpp
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
```

---

## 9. 文件清单

新增文件：
1. DNSFetcher.h
2. DNSFetcher.cpp

修改文件：
3. DNSServerManager.h - 添加 LoadFallbackOverseasDNS() 方法
4. DNSServerManager.cpp - 修改 InitOverseasDNSList() 实现

---

## 10. 测试

### 10.1 单元测试
```cpp
void TestDNSFetcher() {
    CDNSFetcher fetcher;
    std::vector<DNSServerInfo> servers;
    
    // 测试下载
    BOOL result = fetcher.FetchDNSList(servers);
    ASSERT(result == TRUE);
    ASSERT(servers.size() > 0);
    
    // 测试过滤
    fetcher.FilterByReliability(servers, 0.8);
    for (const auto& server : servers) {
        ASSERT(server.reliability >= 80.0);
    }
    
    // 测试分类
    std::vector<DNSServerInfo> us, eu, asia;
    fetcher.FilterByRegion(servers, us, eu, asia);
    ASSERT(us.size() > 0 || eu.size() > 0 || asia.size() > 0);
}
```

---

请根据此需求文档生成完整的DNS抓取模块代码。
```

---

现在把两个文档一起发给 Copilot：
```
@workspace 请根据以下两个需求文档生成完整代码：
1. DNS_SERVER_TESTING_REQUIREMENTS.md - DNS测速模块
2. DNS_FETCHER_REQUIREMENTS.md - DNS抓取模块

要求：
1. DNSFetcher 从 public-dns.info 动态获取DNS列表
2. DNSServerManager 集成 DNSFetcher，优先使用动态列表
3. 如果网络失败，回退到硬编码备用列表
4. 支持缓存机制，避免频繁下载
5. 所有代码可以直接编译运行

请生成所有相关的 .h 和 .cpp 文件。