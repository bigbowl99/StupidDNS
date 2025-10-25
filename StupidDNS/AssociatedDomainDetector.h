// AssociatedDomainDetector.h: 关联域名检测器头文件
//

#pragma once

#include <string>
#include <deque>
#include <map>
#include <set>
#include <vector>

// 关联域名检测器类（增强版）
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
    
  // 通配符匹配
    BOOL WildcardMatch(const CString& domain, const CString& pattern);
};
