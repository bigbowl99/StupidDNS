// DNSServerManager.h: DNS服务器管理类头文件
//

#pragma once

#include "DNSServer.h"
#include "DNSSpeedTester.h"
#include "DNSFetcher.h"  // 添加DNSFetcher
#include <vector>
#include <functional>

// DNS服务器管理类
class CDNSServerManager {
public:
    CDNSServerManager();
    ~CDNSServerManager();
    
  // 初始化（软件启动时调用）
    BOOL Initialize();
    
    // 获取国内最快的DNS
    DNSServerInfo GetFastestDomesticDNS();
    
    // 获取海外最快的N个DNS
  std::vector<DNSServerInfo> GetFastestOverseasDNS(int count = 4);
    
    // 执行完整的测速流程
    BOOL PerformSpeedTest(
   std::function<void(CString)> progressCallback = nullptr);
    
    // 保存/加载配置
    BOOL SaveConfig();
    BOOL LoadConfig();
    
    // 检查配置是否过期（>24小时）
    BOOL IsConfigExpired();
    
    // 获取所有DNS列表
  const std::vector<DNSServerInfo>& GetDomesticServers() const {
   return m_domesticServers;
 }
    const std::vector<DNSServerInfo>& GetOverseasServers() const {
  return m_overseasServers;
    }
    
private:
    std::vector<DNSServerInfo> m_domesticServers;   // 国内DNS列表
std::vector<DNSServerInfo> m_overseasServers;   // 海外DNS列表
    
    DNSServerInfo m_activeDomesticDNS;      // 当前使用的国内DNS
    std::vector<DNSServerInfo> m_activeOverseasDNS; // 当前使用的海外DNS
    
    CDNSSpeedTester m_tester;
    CString m_configFilePath;   // 配置文件路径
 
    // 初始化DNS服务器列表
 void InitDomesticDNSList();
    void InitOverseasDNSList();
  void LoadFallbackOverseasDNS();  // 加载备用海外DNS列表（硬编码）
 
    // 测速并选择最优服务器
 BOOL TestAndSelectDomesticDNS();
    BOOL TestAndSelectOverseasDNS();
    
    // 排序函数
    static BOOL CompareDNSByLatency(const DNSServerInfo& a, 
      const DNSServerInfo& b) {
   if (a.latency == -1) return false;
        if (b.latency == -1) return true;
     return a.latency < b.latency;
 }
    
    // 确保地理分布
    void EnsureGeographicDistribution(std::vector<DNSServerInfo>& selectedDNS);
};
