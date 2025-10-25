// DNSFetcher.h: DNS抓取类头文件
// 从 public-dns.info 获取全球公共DNS服务器信息
//

#pragma once

#include "DNSServer.h"
#include <vector>
#include <map>
#include <functional>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

// DNS抓取类
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
    
    // 设置进度回调
    void SetProgressCallback(std::function<void(CString)> callback) {
        m_progressCallback = callback;
    }
    
private:
    int m_timeout;       // 超时时间（毫秒），默认10000
    CString m_userAgent;    // User-Agent
    std::function<void(CString)> m_progressCallback;
    
    // HTTP请求辅助函数
    BOOL HttpGet(const CString& url, CString& outContent);
    
    // CSV解析辅助函数
    std::vector<CString> SplitCSVLine(const CString& line);
    CString Trim(const CString& str);
    std::vector<CString> SplitByNewLine(const CString& text);
    
    // 国家代码到地区映射
    CString GetRegionByCountryCode(const CString& countryCode);
    void InitRegionMap();
std::map<CString, CString> m_regionMap;
  
    // 缓存相关
    BOOL IsCacheValid(const CString& cacheFile);
    BOOL ReadCacheFile(const CString& cacheFile, CString& outContent);
    BOOL SaveCacheFile(const CString& cacheFile, const CString& content);
    CString GetModulePath();
    
    // 进度更新
    void UpdateProgress(const CString& message);
};
