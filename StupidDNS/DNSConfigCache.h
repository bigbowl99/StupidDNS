// DNSConfigCache.h - DNS配置缓存管理
//

#pragma once

#include <vector>
#include <atlstr.h>
#include <time.h>
#include "DNSServerManager.h"  // 使用DNSServerManager中定义的DNSServerInfo

// 配置缓存管理器
class CDNSConfigCache {
public:
    CDNSConfigCache();
    ~CDNSConfigCache();

    // 加载配置
    BOOL LoadConfig();
    
    // 保存配置
    BOOL SaveConfig();
    
    // 检查配置是否过期
    BOOL IsConfigExpired();
    
    // 获取国内DNS列表（最快的2个）
    std::vector<DNSServerInfo> GetDomesticDNS();
    
    // 获取海外DNS列表（最快的5个）
    std::vector<DNSServerInfo> GetOverseasDNS();
    
    // 更新DNS测速结果
    void UpdateDNSResult(const CString& ip, int latency, bool success);
    
    // 清除配置缓存
    void ClearConfig();
    
    // 设置配置过期天数（默认7天）
    void SetExpireDays(int days) { m_nExpireDays = days; }

private:
    std::vector<DNSServerInfo> m_domesticDNS;   // 国内DNS列表
    std::vector<DNSServerInfo> m_overseasDNS;   // 海外DNS列表
    
    CString m_strConfigFile;  // 配置文件路径
    time_t m_lastUpdateTime;  // 最后更新时间
  int m_nExpireDays;  // 配置过期天数

    // 获取配置文件路径
    CString GetConfigFilePath();
    
    // 解析JSON配置
    BOOL ParseJSON(const CString& json);
    
    // 生成JSON配置
    CString GenerateJSON();
    
    // 读取文件内容
    CString ReadFile(const CString& filePath);
    
    // 写入文件内容
    BOOL WriteFile(const CString& filePath, const CString& content);
};
