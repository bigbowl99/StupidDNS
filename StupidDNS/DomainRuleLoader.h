// DomainRuleLoader.h: 域名规则加载器头文件
//

#pragma once

#include <set>
#include <vector>
#include <map>

// 域名规则加载器类
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
    int GetChinaDomainCount() const { return (int)m_chinaDomains.size(); }
    int GetGFWDomainCount() const { return (int)m_gfwDomains.size(); }
    int GetPoisonIPCount() const { return (int)m_poisonIPs.size(); }
    
private:
    std::set<CString> m_chinaDomains;      // 中国域名集合
 std::set<CString> m_gfwDomains;    // GFW域名集合
    std::set<CString> m_poisonIPs;    // 污染IP集合
    
    // 域名匹配辅助函数
    BOOL MatchDomain(const CString& domain, const CString& pattern) const;
    
    // 提取主域名
    CString ExtractMainDomain(const CString& domain) const;
    
    // 解析dnsmasq格式的行
    CString ParseDnsmasqLine(const CString& line);
    
  // 规则文件路径
    CString GetRulesDirectory() const;
};
