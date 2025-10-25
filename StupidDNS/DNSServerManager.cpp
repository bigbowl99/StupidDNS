// DNSServerManager.cpp: DNS服务器管理类实现
//

#include "pch.h"
#include "DNSServerManager.h"
#include <algorithm>
#include <atlpath.h>

// 构造函数
CDNSServerManager::CDNSServerManager()
{
    // 设置配置文件路径（程序目录\dns_config.ini）
 TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
 CPath pathExe(szPath);
    pathExe.RemoveFileSpec();
 pathExe.Append(_T("dns_config.ini"));
    m_configFilePath = pathExe.m_strPath;
}

// 析构函数
CDNSServerManager::~CDNSServerManager()
{
}

// 初始化
BOOL CDNSServerManager::Initialize()
{
    // 先尝试加载配置
    if (LoadConfig() && !IsConfigExpired()) {
      // 配置有效，直接使用
        return TRUE;
    }
    
    // 配置无效或过期，执行测速
    return PerformSpeedTest();
}

// 初始化国内DNS服务器列表
void CDNSServerManager::InitDomesticDNSList()
{
    m_domesticServers.clear();
    
    // 1. 阿里DNS
    DNSServerInfo ali;
    ali.name = _T("阿里DNS");
    ali.ip = _T("223.5.5.5");
    ali.location = _T("中国");
  ali.provider = _T("Alibaba");
 ali.reliability = 99.9;
    m_domesticServers.push_back(ali);
 
    DNSServerInfo ali2;
    ali2.name = _T("阿里DNS备用");
    ali2.ip = _T("223.6.6.6");
    ali2.location = _T("中国");
    ali2.provider = _T("Alibaba");
  ali2.reliability = 99.9;
    m_domesticServers.push_back(ali2);
    
    // 2. 腾讯DNS
    DNSServerInfo tencent;
    tencent.name = _T("腾讯DNS");
    tencent.ip = _T("119.29.29.29");
    tencent.location = _T("中国");
    tencent.provider = _T("Tencent");
    tencent.reliability = 99.5;
    m_domesticServers.push_back(tencent);
    
    DNSServerInfo tencent2;
  tencent2.name = _T("腾讯DNS备用");
    tencent2.ip = _T("182.254.116.116");
    tencent2.location = _T("中国");
    tencent2.provider = _T("Tencent");
    tencent2.reliability = 99.5;
  m_domesticServers.push_back(tencent2);
    
    // 3. 114DNS
    DNSServerInfo dns114;
    dns114.name = _T("114DNS");
    dns114.ip = _T("114.114.114.114");
    dns114.location = _T("中国");
    dns114.provider = _T("114DNS");
    dns114.reliability = 98.0;
    m_domesticServers.push_back(dns114);
    
    DNSServerInfo dns114_2;
    dns114_2.name = _T("114DNS备用");
    dns114_2.ip = _T("114.114.115.115");
    dns114_2.location = _T("中国");
    dns114_2.provider = _T("114DNS");
    dns114_2.reliability = 98.0;
    m_domesticServers.push_back(dns114_2);
}

// 初始化海外DNS服务器列表
void CDNSServerManager::InitOverseasDNSList()
{
    m_overseasServers.clear();

    // 方案1：从 public-dns.info 动态获取（推荐）
    CDNSFetcher fetcher;
    
    // 设置进度回调
    fetcher.SetProgressCallback([this](CString msg) {
        TRACE(_T("%s\n"), msg);
    });
    
    std::vector<DNSServerInfo> allServers;
    
    if (fetcher.FetchDNSList(allServers)) {
        TRACE(_T("成功从 public-dns.info 获取 %d 个DNS服务器\n"), allServers.size());
        
        // 过滤可靠性>80%
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
        
 TRACE(_T("从 public-dns.info 获取了 %d 个海外DNS候选\n"), 
       m_overseasServers.size());
    }
 else {
        // 获取失败，使用备用硬编码列表
 TRACE(_T("无法从 public-dns.info 获取DNS，使用备用列表\n"));
        LoadFallbackOverseasDNS();
    }
}

// 加载备用海外DNS列表（硬编码）
void CDNSServerManager::LoadFallbackOverseasDNS()
{
    m_overseasServers.clear();
    
    // === 美国DNS (8个) ===
    
    // Cloudflare
    DNSServerInfo cf1;
    cf1.name = _T("Cloudflare");
    cf1.ip = _T("1.1.1.1");
    cf1.location = _T("美国");
    cf1.provider = _T("Cloudflare");
    cf1.reliability = 99.9;
    m_overseasServers.push_back(cf1);
    
    DNSServerInfo cf2;
    cf2.name = _T("Cloudflare备用");
    cf2.ip = _T("1.0.0.1");
    cf2.location = _T("美国");
    cf2.provider = _T("Cloudflare");
 cf2.reliability = 99.9;
    m_overseasServers.push_back(cf2);

    // Google
    DNSServerInfo google1;
    google1.name = _T("Google DNS");
    google1.ip = _T("8.8.8.8");
    google1.location = _T("美国");
    google1.provider = _T("Google");
    google1.reliability = 99.9;
    m_overseasServers.push_back(google1);
    
    DNSServerInfo google2;
    google2.name = _T("Google DNS备用");
    google2.ip = _T("8.8.4.4");
    google2.location = _T("美国");
  google2.provider = _T("Google");
  google2.reliability = 99.9;
    m_overseasServers.push_back(google2);
    
  // Quad9
    DNSServerInfo quad9_1;
    quad9_1.name = _T("Quad9");
    quad9_1.ip = _T("9.9.9.9");
    quad9_1.location = _T("美国");
    quad9_1.provider = _T("Quad9");
    quad9_1.reliability = 99.5;
    m_overseasServers.push_back(quad9_1);
    
    DNSServerInfo quad9_2;
    quad9_2.name = _T("Quad9备用");
    quad9_2.ip = _T("149.112.112.112");
  quad9_2.location = _T("美国");
    quad9_2.provider = _T("Quad9");
    quad9_2.reliability = 99.5;
    m_overseasServers.push_back(quad9_2);

    // OpenDNS
    DNSServerInfo opendns1;
 opendns1.name = _T("OpenDNS");
    opendns1.ip = _T("208.67.222.222");
    opendns1.location = _T("美国");
    opendns1.provider = _T("OpenDNS");
  opendns1.reliability = 99.0;
    m_overseasServers.push_back(opendns1);
    
    DNSServerInfo opendns2;
    opendns2.name = _T("OpenDNS备用");
    opendns2.ip = _T("208.67.220.220");
    opendns2.location = _T("美国");
    opendns2.provider = _T("OpenDNS");
    opendns2.reliability = 99.0;
    m_overseasServers.push_back(opendns2);
    
    // === 欧洲DNS (4个) ===
    
    // DNS.Watch (德国)
    DNSServerInfo dnswatch1;
    dnswatch1.name = _T("DNS.Watch(德国)");
    dnswatch1.ip = _T("84.200.69.80");
    dnswatch1.location = _T("欧洲");
    dnswatch1.provider = _T("DNS.Watch");
    dnswatch1.reliability = 98.0;
    m_overseasServers.push_back(dnswatch1);
    
    DNSServerInfo dnswatch2;
    dnswatch2.name = _T("DNS.Watch备用");
    dnswatch2.ip = _T("84.200.70.40");
    dnswatch2.location = _T("欧洲");
    dnswatch2.provider = _T("DNS.Watch");
    dnswatch2.reliability = 98.0;
    m_overseasServers.push_back(dnswatch2);
    
    // FreeDNS (奥地利)
    DNSServerInfo freedns1;
 freedns1.name = _T("FreeDNS(奥地利)");
    freedns1.ip = _T("37.235.1.174");
    freedns1.location = _T("欧洲");
 freedns1.provider = _T("FreeDNS");
    freedns1.reliability = 95.0;
    m_overseasServers.push_back(freedns1);
    
    DNSServerInfo freedns2;
    freedns2.name = _T("FreeDNS备用");
 freedns2.ip = _T("37.235.1.177");
    freedns2.location = _T("欧洲");
    freedns2.provider = _T("FreeDNS");
    freedns2.reliability = 95.0;
    m_overseasServers.push_back(freedns2);
    
  // === 亚洲DNS (不包括中国，6个) ===
    
    // 日本
 DNSServerInfo japan;
    japan.name = _T("日本OpenDNS");
    japan.ip = _T("210.141.113.7");
    japan.location = _T("亚洲");
    japan.provider = _T("OpenDNS");
    japan.reliability = 95.0;
 m_overseasServers.push_back(japan);
    
  // 韩国
    DNSServerInfo korea1;
    korea1.name = _T("韩国KT");
    korea1.ip = _T("168.126.63.1");
 korea1.location = _T("亚洲");
    korea1.provider = _T("KT");
    korea1.reliability = 98.0;
  m_overseasServers.push_back(korea1);
    
    DNSServerInfo korea2;
    korea2.name = _T("韩国KT备用");
    korea2.ip = _T("168.126.63.2");
    korea2.location = _T("亚洲");
    korea2.provider = _T("KT");
    korea2.reliability = 98.0;
    m_overseasServers.push_back(korea2);
    
    // 新加坡
    DNSServerInfo singapore1;
    singapore1.name = _T("新加坡SingNet");
    singapore1.ip = _T("165.21.83.88");
    singapore1.location = _T("亚洲");
  singapore1.provider = _T("SingNet");
    singapore1.reliability = 97.0;
    m_overseasServers.push_back(singapore1);

    DNSServerInfo singapore2;
    singapore2.name = _T("新加坡SingNet备用");
    singapore2.ip = _T("165.21.100.88");
    singapore2.location = _T("亚洲");
    singapore2.provider = _T("SingNet");
    singapore2.reliability = 97.0;
  m_overseasServers.push_back(singapore2);
    
    // 台湾
    DNSServerInfo taiwan;
    taiwan.name = _T("台湾HiNet");
    taiwan.ip = _T("168.95.1.1");
  taiwan.location = _T("亚洲");
    taiwan.provider = _T("HiNet");
    taiwan.reliability = 98.0;
    m_overseasServers.push_back(taiwan);
    
    TRACE(_T("使用备用硬编码DNS列表，共 %d 个\n"), m_overseasServers.size());
}

// 测试并选择国内DNS
BOOL CDNSServerManager::TestAndSelectDomesticDNS()
{
    // 提取所有国内DNS的IP列表
    std::vector<CString> ipList;
    for (auto& dns : m_domesticServers) {
        ipList.push_back(dns.ip);
    }
    
    // 执行测速
    auto results = m_tester.TestMultipleDNS(ipList, 3);

    // 更新DNS信息
    for (size_t i = 0; i < results.size() && i < m_domesticServers.size(); i++) {
        m_domesticServers[i].latency = results[i].avgLatency;
        m_domesticServers[i].isOnline = (results[i].avgLatency > 0);
     m_domesticServers[i].lastTestTime = GetTickCount();
        
   if (results[i].avgLatency > 0) {
            m_domesticServers[i].failCount = 0;
        } else {
    m_domesticServers[i].failCount++;
        }
    }
    
    // 按延迟排序
    std::sort(m_domesticServers.begin(), m_domesticServers.end(), CompareDNSByLatency);
    
    // 选择最快的
    if (m_domesticServers.size() > 0 && m_domesticServers[0].latency > 0) {
 m_activeDomesticDNS = m_domesticServers[0];
        m_activeDomesticDNS.isActive = TRUE;
        
   // 记录日志
 CString msg;
   msg.Format(_T("选中国内DNS: %s (%s) - %dms"), 
            m_activeDomesticDNS.name, 
     m_activeDomesticDNS.ip, 
    m_activeDomesticDNS.latency);
        TRACE(_T("%s\n"), msg);
        
        // 警告：延迟过高
        if (m_activeDomesticDNS.latency > 500) {
            TRACE(_T("警告: 国内DNS延迟过高 (%dms)\n"), m_activeDomesticDNS.latency);
        }
        
        return TRUE;
    }
    
    return FALSE;
}

// 测试并选择海外DNS
BOOL CDNSServerManager::TestAndSelectOverseasDNS()
{
    // 提取所有海外DNS的IP列表
    std::vector<CString> ipList;
    for (auto& dns : m_overseasServers) {
        ipList.push_back(dns.ip);
    }
    
    // 执行测速
  auto results = m_tester.TestMultipleDNS(ipList, 3);
    
    // 更新DNS信息
    for (size_t i = 0; i < results.size() && i < m_overseasServers.size(); i++) {
        m_overseasServers[i].latency = results[i].avgLatency;
    m_overseasServers[i].isOnline = (results[i].avgLatency > 0);
   m_overseasServers[i].lastTestTime = GetTickCount();
        
        if (results[i].avgLatency > 0) {
  m_overseasServers[i].failCount = 0;
        } else {
     m_overseasServers[i].failCount++;
        }
    }
    
    // 过滤掉失败的DNS
    std::vector<DNSServerInfo> validDNS;
    for (auto& dns : m_overseasServers) {
        if (dns.latency > 0) {
            validDNS.push_back(dns);
      }
    }
    
    if (validDNS.empty()) {
 return FALSE;
    }
  
    // 按延迟排序
    std::sort(validDNS.begin(), validDNS.end(), CompareDNSByLatency);
    
    // 选择前4个，但要确保地理分布
    m_activeOverseasDNS.clear();
    
    // 简化版：直接选择最快的4个
 int selectCount = min(4, (int)validDNS.size());
    for (int i = 0; i < selectCount; i++) {
      validDNS[i].isActive = TRUE;
        m_activeOverseasDNS.push_back(validDNS[i]);
   
        // 记录日志
        CString msg;
        msg.Format(_T("选中海外DNS: %s (%s) - %dms"), 
            validDNS[i].name, 
        validDNS[i].ip, 
            validDNS[i].latency);
        TRACE(_T("%s\n"), msg);
    }
    
    // 尝试确保地理分布
    EnsureGeographicDistribution(m_activeOverseasDNS);
    
    return TRUE;
}

// 确保地理分布
void CDNSServerManager::EnsureGeographicDistribution(std::vector<DNSServerInfo>& selectedDNS)
{
    // 统计各地区数量
    int usaCount = 0, europeCount = 0, asiaCount = 0;
    
    for (auto& dns : selectedDNS) {
if (dns.location == _T("美国")) usaCount++;
        else if (dns.location == _T("欧洲")) europeCount++;
        else if (dns.location == _T("亚洲")) asiaCount++;
    }
 
    // 简单记录日志
    TRACE(_T("地理分布: 美国=%d, 欧洲=%d, 亚洲=%d\n"), usaCount, europeCount, asiaCount);
}

// 执行完整的测速流程
BOOL CDNSServerManager::PerformSpeedTest(std::function<void(CString)> progressCallback)
{
    // 初始化DNS列表
    InitDomesticDNSList();
    InitOverseasDNSList();
    
    // 设置进度回调
    if (progressCallback) {
      progressCallback(_T("正在测试国内DNS服务器..."));
    }
    
    m_tester.SetProgressCallback([progressCallback](int current, int total, CString status) {
        if (progressCallback) {
            progressCallback(status);
        }
    });
    
    // 测试国内DNS
    BOOL domesticOK = TestAndSelectDomesticDNS();
    
    if (progressCallback) {
  progressCallback(_T("正在测试海外DNS服务器..."));
    }
    
    // 测试海外DNS
    BOOL overseasOK = TestAndSelectOverseasDNS();
  
    if (progressCallback) {
    progressCallback(_T("测速完成"));
    }
    
    // 保存配置
    SaveConfig();
    
    return domesticOK && overseasOK;
}

// 获取国内最快的DNS
DNSServerInfo CDNSServerManager::GetFastestDomesticDNS()
{
    return m_activeDomesticDNS;
}

// 获取海外最快的N个DNS
std::vector<DNSServerInfo> CDNSServerManager::GetFastestOverseasDNS(int count)
{
    std::vector<DNSServerInfo> result;
    
    int selectCount = min(count, (int)m_activeOverseasDNS.size());
    for (int i = 0; i < selectCount; i++) {
      result.push_back(m_activeOverseasDNS[i]);
    }
 
    return result;
}

// 检查配置是否过期
BOOL CDNSServerManager::IsConfigExpired()
{
    if (m_activeDomesticDNS.lastTestTime == 0) {
   return TRUE;
    }
    
    DWORD currentTime = GetTickCount();
    DWORD elapsed = currentTime - m_activeDomesticDNS.lastTestTime;
 
    // 24小时 = 86400000毫秒
    return (elapsed > 86400000);
}

// 保存配置
BOOL CDNSServerManager::SaveConfig()
{
    // 使用WritePrivateProfileString保存到INI文件

    // [DomesticDNS]
    WritePrivateProfileString(_T("DomesticDNS"), _T("ActiveDNS"), 
     m_activeDomesticDNS.ip, m_configFilePath);
    WritePrivateProfileString(_T("DomesticDNS"), _T("Name"), 
        m_activeDomesticDNS.name, m_configFilePath);
    
    CString latency;
    latency.Format(_T("%d"), m_activeDomesticDNS.latency);
    WritePrivateProfileString(_T("DomesticDNS"), _T("Latency"), 
        latency, m_configFilePath);
    
    CString lastTest;
    lastTest.Format(_T("%u"), m_activeDomesticDNS.lastTestTime);
    WritePrivateProfileString(_T("DomesticDNS"), _T("LastTest"), 
        lastTest, m_configFilePath);
    
    // [OverseasDNS]
    CString count;
    count.Format(_T("%d"), (int)m_activeOverseasDNS.size());
    WritePrivateProfileString(_T("OverseasDNS"), _T("Count"), 
        count, m_configFilePath);
    
    for (int i = 0; i < (int)m_activeOverseasDNS.size(); i++) {
     CString key;
        
        key.Format(_T("DNS%d"), i + 1);
        WritePrivateProfileString(_T("OverseasDNS"), key, 
            m_activeOverseasDNS[i].ip, m_configFilePath);
        
      key.Format(_T("DNS%dName"), i + 1);
        WritePrivateProfileString(_T("OverseasDNS"), key, 
            m_activeOverseasDNS[i].name, m_configFilePath);
        
  key.Format(_T("DNS%dLatency"), i + 1);
    CString lat;
        lat.Format(_T("%d"), m_activeOverseasDNS[i].latency);
        WritePrivateProfileString(_T("OverseasDNS"), key, 
lat, m_configFilePath);
    }
    
    // [Settings]
    CString currentTime;
    currentTime.Format(_T("%u"), GetTickCount());
    WritePrivateProfileString(_T("Settings"), _T("LastFullTest"), 
        currentTime, m_configFilePath);
    
    return TRUE;
}

// 加载配置
BOOL CDNSServerManager::LoadConfig()
{
    // 检查文件是否存在
    if (GetFileAttributes(m_configFilePath) == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }
    
    // 加载国内DNS
    TCHAR buffer[256];
    
    GetPrivateProfileString(_T("DomesticDNS"), _T("ActiveDNS"), _T(""), 
        buffer, 256, m_configFilePath);
    m_activeDomesticDNS.ip = buffer;
    
 if (m_activeDomesticDNS.ip.IsEmpty()) {
        return FALSE;
    }
    
 GetPrivateProfileString(_T("DomesticDNS"), _T("Name"), _T(""), 
        buffer, 256, m_configFilePath);
    m_activeDomesticDNS.name = buffer;
    
    m_activeDomesticDNS.latency = GetPrivateProfileInt(_T("DomesticDNS"), 
        _T("Latency"), -1, m_configFilePath);
    m_activeDomesticDNS.lastTestTime = GetPrivateProfileInt(_T("DomesticDNS"), 
        _T("LastTest"), 0, m_configFilePath);
    m_activeDomesticDNS.isActive = TRUE;
    
 // 加载海外DNS
  int count = GetPrivateProfileInt(_T("OverseasDNS"), _T("Count"), 
        0, m_configFilePath);
    
    m_activeOverseasDNS.clear();
    for (int i = 0; i < count; i++) {
        DNSServerInfo dns;
    
        CString key;
        key.Format(_T("DNS%d"), i + 1);
        GetPrivateProfileString(_T("OverseasDNS"), key, _T(""), 
     buffer, 256, m_configFilePath);
  dns.ip = buffer;
     
        key.Format(_T("DNS%dName"), i + 1);
        GetPrivateProfileString(_T("OverseasDNS"), key, _T(""), 
 buffer, 256, m_configFilePath);
    dns.name = buffer;
        
     key.Format(_T("DNS%dLatency"), i + 1);
 dns.latency = GetPrivateProfileInt(_T("OverseasDNS"), key, 
            -1, m_configFilePath);
    
        dns.isActive = TRUE;
        m_activeOverseasDNS.push_back(dns);
    }
    
    return TRUE;
}
