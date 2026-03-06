// DomainClassifier.cpp: 域名分类器实现
//

#include "pch.h"
#include "DomainClassifier.h"

// 构造函数
CDomainClassifier::CDomainClassifier()
    : m_domesticCount(0)
    , m_foreignCount(0)
    , m_unknownCount(0)
{
    InitializeCriticalSection(&m_cs);
}

// 析构函数
CDomainClassifier::~CDomainClassifier()
{
    DeleteCriticalSection(&m_cs);
}

// 初始化
BOOL CDomainClassifier::Initialize()
{
    // 加载规则文件
    if (!m_ruleLoader.LoadAllRules()) {
  TRACE(_T("警告: 规则加载部分失败\n"));
    }
    
 // 初始化关联检测器
    m_associationDetector.Initialize();
    
    return TRUE;
}

// 域名分类（核心函数）
// 这是整个系统最核心的函数！
DomainType CDomainClassifier::ClassifyDomain(const CString& domain)
{
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
        LeaveCriticalSection(&m_cs);
        m_associationDetector.RecordQuery(normalizedDomain, TRUE);
        
  TRACE(_T("[分类] %s -> 国外域名（GFW黑名单）\n"), domain);
        return FOREIGN;
    }
    
  // ========== 优先级3: 关联域名检测 ==========
    // 检查是否应该跟随GFW域名（时间窗口关联或生态系统规则）
    if (m_associationDetector.ShouldUseOverseasDNS(normalizedDomain)) {
        m_foreignCount++;
    
  // 记录非GFW查询
        LeaveCriticalSection(&m_cs);
     m_associationDetector.RecordQuery(normalizedDomain, FALSE);
        
  TRACE(_T("[分类] %s -> 关联域名（跟随海外）\n"), domain);
 return ASSOCIATED;
    }
    
    // ========== 优先级4: 特殊模式匹配 ==========
 // 某些明显的模式可以直接判断
    
    // 4.1 明显的中国域名
    if (normalizedDomain.GetLength() >= 3 && normalizedDomain.Right(3) == _T(".cn")) {
   m_domesticCount++;
  LeaveCriticalSection(&m_cs);
      TRACE(_T("[分类] %s -> 国内域名（.cn后缀）\n"), domain);
        return DOMESTIC;
    }
    
    if (normalizedDomain.Find(_T(".gov.cn")) != -1 ||
        normalizedDomain.Find(_T(".edu.cn")) != -1) {
        m_domesticCount++;
   LeaveCriticalSection(&m_cs);
     TRACE(_T("[分类] %s -> 国内域名（政府/教育域名）\n"), domain);
        return DOMESTIC;
    }
    
    // 4.2 本地域名
    if (normalizedDomain == _T("localhost") ||
   normalizedDomain.Find(_T(".local")) != -1 ||
        normalizedDomain.Find(_T(".lan")) != -1) {
        m_domesticCount++;
  LeaveCriticalSection(&m_cs);
  TRACE(_T("[分类] %s -> 国内域名（本地域名）\n"), domain);
        return DOMESTIC;
    }
    
    // ========== 优先级5: 未知域名 ==========
    m_unknownCount++;
    
    // 记录非GFW查询（用于时间窗口关联）
    LeaveCriticalSection(&m_cs);
    m_associationDetector.RecordQuery(normalizedDomain, FALSE);
    
    TRACE(_T("[分类] %s -> 未知域名（需要双向查询）\n"), domain);
    return UNKNOWN;
}

// 记录查询
void CDomainClassifier::RecordQuery(const CString& domain, DomainType type)
{
    // 记录到关联检测器
    BOOL isGFW = (type == FOREIGN);
    m_associationDetector.RecordQuery(domain, isGFW);
}

// IP污染检测
BOOL CDomainClassifier::IsPoisonedIP(const CString& ip)
{
    return m_ruleLoader.IsPoisonIP(ip);
}
