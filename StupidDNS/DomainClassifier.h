// DomainClassifier.h: 域名分类器头文件（核心路由逻辑）
//

#pragma once

#include "DomainRuleLoader.h"
#include "AssociatedDomainDetector.h"

// 域名类型枚举
enum DomainType {
    DOMESTIC,// 国内域名
    FOREIGN,       // 国外域名
  ASSOCIATED,    // 关联域名（跟随GFW域名）
    UNKNOWN // 未知域名
};

// 域名分类器类（核心路由逻辑）
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
    
    // 获取规则加载器（用于统计）
    const CDomainRuleLoader& GetRuleLoader() const { return m_ruleLoader; }
    
    // 获取关联检测器
    CAssociatedDomainDetector& GetAssociationDetector() { return m_associationDetector; }
 
private:
    CDomainRuleLoader m_ruleLoader;
    CAssociatedDomainDetector m_associationDetector;
    
    // 统计
  int m_domesticCount;
    int m_foreignCount;
    int m_unknownCount;
    
    CRITICAL_SECTION m_cs;
};
