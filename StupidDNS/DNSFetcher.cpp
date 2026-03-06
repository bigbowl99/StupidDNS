// DNSFetcher.cpp: DNS抓取类实现
//

#include "pch.h"
#include "DNSFetcher.h"
#include <atlpath.h>
#include <sstream>

// 构造函数
CDNSFetcher::CDNSFetcher()
 : m_timeout(10000)  // 默认10秒超时
    , m_userAgent(_T("StupidDNS/1.0"))
{
    InitRegionMap();
}

// 析构函数
CDNSFetcher::~CDNSFetcher()
{
}

// 获取程序目录
CString CDNSFetcher::GetModulePath()
{
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    CPath pathExe(szPath);
    pathExe.RemoveFileSpec();
    return CString(pathExe);
}

// 更新进度
void CDNSFetcher::UpdateProgress(const CString& message)
{
    if (m_progressCallback) {
        m_progressCallback(message);
    }
    TRACE(_T("%s\n"), message);
}

// 初始化地区映射表
void CDNSFetcher::InitRegionMap()
{
    if (!m_regionMap.empty()) {
        return;
    }
    
    // 北美
    m_regionMap[_T("US")] = _T("美国");
    m_regionMap[_T("CA")] = _T("美国");
    m_regionMap[_T("MX")] = _T("美国");
    
    // 欧洲
    m_regionMap[_T("GB")] = _T("欧洲");
    m_regionMap[_T("DE")] = _T("欧洲");
    m_regionMap[_T("FR")] = _T("欧洲");
    m_regionMap[_T("NL")] = _T("欧洲");
    m_regionMap[_T("SE")] = _T("欧洲");
    m_regionMap[_T("NO")] = _T("欧洲");
    m_regionMap[_T("FI")] = _T("欧洲");
  m_regionMap[_T("DK")] = _T("欧洲");
    m_regionMap[_T("IT")] = _T("欧洲");
    m_regionMap[_T("ES")] = _T("欧洲");
    m_regionMap[_T("CH")] = _T("欧洲");
    m_regionMap[_T("AT")] = _T("欧洲");
    m_regionMap[_T("BE")] = _T("欧洲");
    m_regionMap[_T("PL")] = _T("欧洲");
    m_regionMap[_T("CZ")] = _T("欧洲");
    m_regionMap[_T("IE")] = _T("欧洲");
    m_regionMap[_T("PT")] = _T("欧洲");
    m_regionMap[_T("GR")] = _T("欧洲");
    m_regionMap[_T("RO")] = _T("欧洲");
    m_regionMap[_T("HU")] = _T("欧洲");
    
    // 亚洲（不包括中国）
    m_regionMap[_T("JP")] = _T("亚洲");
    m_regionMap[_T("KR")] = _T("亚洲");
    m_regionMap[_T("SG")] = _T("亚洲");
    m_regionMap[_T("TW")] = _T("亚洲");
    m_regionMap[_T("HK")] = _T("亚洲");
    m_regionMap[_T("IN")] = _T("亚洲");
    m_regionMap[_T("TH")] = _T("亚洲");
    m_regionMap[_T("MY")] = _T("亚洲");
    m_regionMap[_T("PH")] = _T("亚洲");
    m_regionMap[_T("ID")] = _T("亚洲");
    m_regionMap[_T("VN")] = _T("亚洲");
    
    // 中国
    m_regionMap[_T("CN")] = _T("中国");
}

// 根据国家代码获取地区
CString CDNSFetcher::GetRegionByCountryCode(const CString& countryCode)
{
    CString code = countryCode;
    code.MakeUpper();
    
    auto it = m_regionMap.find(code);
    if (it != m_regionMap.end()) {
        return it->second;
    }
    
    return _T("其他");
}

// 字符串去除首尾空格
CString CDNSFetcher::Trim(const CString& str)
{
    CString result = str;
    result.TrimLeft();
    result.TrimRight();
    return result;
}

// 按换行符分割字符串
std::vector<CString> CDNSFetcher::SplitByNewLine(const CString& text)
{
    std::vector<CString> lines;
    CString line;
    
    for (int i = 0; i < text.GetLength(); i++) {
    TCHAR ch = text[i];

  if (ch == _T('\r')) {
            // 跳过 \r
    if (i + 1 < text.GetLength() && text[i + 1] == _T('\n')) {
       i++;  // 跳过 \n
            }
   if (!line.IsEmpty()) {
           lines.push_back(line);
      line.Empty();
            }
        }
        else if (ch == _T('\n')) {
if (!line.IsEmpty()) {
       lines.push_back(line);
    line.Empty();
            }
     }
        else {
            line += ch;
        }
    }
    
// 添加最后一行
    if (!line.IsEmpty()) {
        lines.push_back(line);
    }
    
    return lines;
}

// 分割CSV行（处理引号）
std::vector<CString> CDNSFetcher::SplitCSVLine(const CString& line)
{
    std::vector<CString> fields;
    CString field;
    BOOL inQuotes = FALSE;
    
 for (int i = 0; i < line.GetLength(); i++) {
        TCHAR ch = line[i];
  
        if (ch == _T('"')) {
    // 处理引号
          if (inQuotes && i + 1 < line.GetLength() && line[i + 1] == _T('"')) {
        // 转义的引号
       field += _T('"');
i++;  // 跳过下一个引号
         } else {
// 引号开始/结束
           inQuotes = !inQuotes;
      }
        }
        else if (ch == _T(',') && !inQuotes) {
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

// HTTP GET 请求
BOOL CDNSFetcher::HttpGet(const CString& url, CString& outContent)
{
    UpdateProgress(_T("正在连接到 public-dns.info..."));
    
    // 初始化 WinHTTP
    HINTERNET hSession = WinHttpOpen(
        m_userAgent,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    
    if (!hSession) {
     UpdateProgress(_T("无法初始化HTTP会话"));
        return FALSE;
    }
    
    // 设置超时
    WinHttpSetTimeouts(hSession, m_timeout, m_timeout, m_timeout, m_timeout);
    
    // 连接服务器
    HINTERNET hConnect = WinHttpConnect(
  hSession,
      L"public-dns.info",
        INTERNET_DEFAULT_HTTPS_PORT,
        0);
    
    if (!hConnect) {
        UpdateProgress(_T("无法连接到服务器"));
        WinHttpCloseHandle(hSession);
        return FALSE;
    }
    
 UpdateProgress(_T("正在请求DNS列表..."));
    
    // 创建请求
    HINTERNET hRequest = WinHttpOpenRequest(
      hConnect,
        L"GET",
        L"/nameservers.csv",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    
    if (!hRequest) {
   UpdateProgress(_T("无法创建HTTP请求"));
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return FALSE;
    }
    
    // 发送请求
    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
0,
        0);
    
    if (!bResults) {
        UpdateProgress(_T("发送请求失败"));
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return FALSE;
    }
    
    // 接收响应
    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults) {
        UpdateProgress(_T("接收响应失败"));
        WinHttpCloseHandle(hRequest);
   WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return FALSE;
    }
    
 // 检查HTTP状态码
    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL,
        &dwStatusCode,
        &dwSize,
        NULL);
    
    if (dwStatusCode != 200) {
        CString msg;
     msg.Format(_T("HTTP错误: %d"), dwStatusCode);
        UpdateProgress(msg);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
     WinHttpCloseHandle(hSession);
    return FALSE;
    }
    
    UpdateProgress(_T("正在下载数据..."));
    
 // 读取数据
    outContent.Empty();
    DWORD dwBytesRead = 0;
    char buffer[4096];
  
    do {
      dwSize = 0;
    if (WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            if (dwSize == 0) {
    break;
      }
    
     DWORD dwRead = min(dwSize, sizeof(buffer));
       if (WinHttpReadData(hRequest, buffer, dwRead, &dwBytesRead)) {
            if (dwBytesRead == 0) {
              break;
       }
           
   // 转换为CString（假设是UTF-8编码）
         CStringA strA(buffer, dwBytesRead);
              outContent += CString(strA);
        }
     }
} while (dwSize > 0);
    
    // 清理资源
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    UpdateProgress(_T("下载完成"));
  return TRUE;
}

// 下载CSV文件（带重试）
BOOL CDNSFetcher::DownloadCSV(CString& outContent)
{
    int retryCount = 3;
  int retryDelay = 2000;  // 2秒
    
    for (int i = 0; i < retryCount; i++) {
        if (i > 0) {
            CString msg;
msg.Format(_T("下载失败，重试 %d/%d..."), i, retryCount - 1);
          UpdateProgress(msg);
  Sleep(retryDelay);
        }
        
        if (HttpGet(_T("https://public-dns.info/nameservers.csv"), outContent)) {
  return TRUE;
        }
    }
    
    UpdateProgress(_T("下载失败，已重试3次"));
    return FALSE;
}

// 检查缓存是否有效
BOOL CDNSFetcher::IsCacheValid(const CString& cacheFile)
{
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

// 读取缓存文件
BOOL CDNSFetcher::ReadCacheFile(const CString& cacheFile, CString& outContent)
{
    CStdioFile file;
    if (!file.Open(cacheFile, CFile::modeRead | CFile::typeText)) {
        return FALSE;
    }
  
    outContent.Empty();
    CString line;
    while (file.ReadString(line)) {
        outContent += line + _T("\n");
    }
    
    file.Close();
    return TRUE;
}

// 保存缓存文件
BOOL CDNSFetcher::SaveCacheFile(const CString& cacheFile, const CString& content)
{
    CStdioFile file;
    if (!file.Open(cacheFile, CFile::modeCreate | CFile::modeWrite | CFile::typeText)) {
        return FALSE;
    }
    
    file.WriteString(content);
    file.Close();
    return TRUE;
}

// 解析CSV内容
BOOL CDNSFetcher::ParseCSV(const CString& csvContent, std::vector<DNSServerInfo>& outServers)
{
    UpdateProgress(_T("正在解析DNS数据..."));
    
    outServers.clear();
    
    // 按行分割
    std::vector<CString> lines = SplitByNewLine(csvContent);
    
    if (lines.empty()) {
      UpdateProgress(_T("CSV内容为空"));
        return FALSE;
    }
    
    // 跳过第一行（表头）
    int parsedCount = 0;
  for (size_t i = 1; i < lines.size(); i++) {
   std::vector<CString> fields = SplitCSVLine(lines[i]);
     
        // CSV列定义：ip,name,country_id,city,version,error,dnssec,reliability,checked_at,created_at
        if (fields.size() < 8) {
   continue;  // 跳过无效行
        }
      
        DNSServerInfo info;
   info.ip = fields[0];      // IP地址
        info.name = fields[1];  // 名称
        
   // 如果名称为空，使用IP作为名称
     if (info.name.IsEmpty()) {
      info.name = info.ip;
        }
        
        CString countryCode = fields[2];  // 国家代码
        info.location = GetRegionByCountryCode(countryCode);
    info.provider = fields[1];  // 提供商名称
        
        // 解析可靠性（reliability列，格式：0.00-1.00）
        double reliability = _ttof(fields[7]);
        info.reliability = reliability * 100;  // 转换为百分比
        
        // 初始化其他字段
     info.latency = -1;
        info.isActive = FALSE;
     info.isOnline = FALSE;
        info.lastTestTime = 0;
        info.failCount = 0;

        outServers.push_back(info);
  parsedCount++;
    }
    
    CString msg;
    msg.Format(_T("解析完成，共 %d 个DNS服务器"), parsedCount);
    UpdateProgress(msg);
    
    return parsedCount > 0;
}

// 按可靠性过滤
void CDNSFetcher::FilterByReliability(std::vector<DNSServerInfo>& servers, double minReliability)
{
    int originalCount = servers.size();
    
 servers.erase(
        std::remove_if(servers.begin(), servers.end(),
     [minReliability](const DNSServerInfo& info) {
                return info.reliability < minReliability * 100;
          }),
        servers.end()
    );
    
    CString msg;
    msg.Format(_T("过滤后剩余 %d 个DNS (可靠性>%.0f%%)"), 
        servers.size(), minReliability * 100);
    UpdateProgress(msg);
}

// 按地区分类
void CDNSFetcher::FilterByRegion(const std::vector<DNSServerInfo>& allServers,
std::vector<DNSServerInfo>& usServers,
    std::vector<DNSServerInfo>& euServers,
    std::vector<DNSServerInfo>& asiaServers)
{
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
    
    CString msg;
    msg.Format(_T("地区分类: 美国=%d, 欧洲=%d, 亚洲=%d"), 
        usServers.size(), euServers.size(), asiaServers.size());
    UpdateProgress(msg);
}

// 获取DNS列表（主入口）
BOOL CDNSFetcher::FetchDNSList(std::vector<DNSServerInfo>& outServers)
{
    CString cacheFile = GetModulePath() + _T("\\dns_cache.csv");
    CString csvContent;
    
    // 检查缓存
    if (IsCacheValid(cacheFile)) {
        UpdateProgress(_T("使用缓存的DNS列表"));
        if (ReadCacheFile(cacheFile, csvContent)) {
 return ParseCSV(csvContent, outServers);
        }
    }
    
    // 下载新数据
    UpdateProgress(_T("缓存无效，开始下载新数据"));
    if (DownloadCSV(csvContent)) {
    // 保存到缓存
        SaveCacheFile(cacheFile, csvContent);
  return ParseCSV(csvContent, outServers);
    }
    
    UpdateProgress(_T("无法获取DNS列表"));
    return FALSE;
}
