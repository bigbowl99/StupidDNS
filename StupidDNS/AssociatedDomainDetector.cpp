// AssociatedDomainDetector.cpp: 关联域名检测器实现
//

#include "pch.h"
#include "AssociatedDomainDetector.h"

// 构造函数
CAssociatedDomainDetector::CAssociatedDomainDetector()
{
    InitializeCriticalSection(&m_cs);
}

// 析构函数
CAssociatedDomainDetector::~CAssociatedDomainDetector()
{
    DeleteCriticalSection(&m_cs);
}

// 初始化
void CAssociatedDomainDetector::Initialize()
{
    InitEcosystemRules();
    LoadAssociations();
}

// 提取主域名
CString CAssociatedDomainDetector::ExtractMainDomain(const CString& domain)
{
    int lastDot = domain.ReverseFind(_T('.'));
    if (lastDot == -1) return domain;
    
    int secondLastDot = domain.Left(lastDot).ReverseFind(_T('.'));
  if (secondLastDot == -1) return domain;
    
    return domain.Mid(secondLastDot + 1);
}

// 通配符匹配
BOOL CAssociatedDomainDetector::WildcardMatch(const CString& domain, const CString& pattern)
{
    // *.google.com 匹配 www.google.com, api.google.com 等
  if (pattern.GetLength() > 0 && pattern[0] == _T('*')) {
   CString suffix = pattern.Mid(1);  // 去掉 *
        if (domain.GetLength() >= suffix.GetLength()) {
   if (domain.Right(suffix.GetLength()) == suffix) {
      return TRUE;
    }
  }
    }
    
    // 精确匹配
    return domain == pattern;
}

// 初始化预定义生态系统规则
void CAssociatedDomainDetector::InitEcosystemRules()
{
    // Google 生态系统
  m_ecosystemRules[_T("google.com")].push_back(_T("*.google.com"));
    m_ecosystemRules[_T("google.com")].push_back(_T("*.googleapis.com"));
    m_ecosystemRules[_T("google.com")].push_back(_T("*.gstatic.com"));
    m_ecosystemRules[_T("google.com")].push_back(_T("*.googleusercontent.com"));
    m_ecosystemRules[_T("google.com")].push_back(_T("*.ggpht.com"));
    m_ecosystemRules[_T("google.com")].push_back(_T("*.youtube.com"));
    m_ecosystemRules[_T("google.com")].push_back(_T("*.ytimg.com"));
    m_ecosystemRules[_T("google.com")].push_back(_T("*.googlevideo.com"));
  
    // Facebook 生态系统
    m_ecosystemRules[_T("facebook.com")].push_back(_T("*.facebook.com"));
    m_ecosystemRules[_T("facebook.com")].push_back(_T("*.fbcdn.net"));
    m_ecosystemRules[_T("facebook.com")].push_back(_T("*.facebook.net"));
    m_ecosystemRules[_T("facebook.com")].push_back(_T("*.instagram.com"));
    m_ecosystemRules[_T("facebook.com")].push_back(_T("*.cdninstagram.com"));
    m_ecosystemRules[_T("facebook.com")].push_back(_T("*.whatsapp.com"));
    m_ecosystemRules[_T("facebook.com")].push_back(_T("*.whatsapp.net"));
    
    // Twitter 生态系统
    m_ecosystemRules[_T("twitter.com")].push_back(_T("*.twitter.com"));
    m_ecosystemRules[_T("twitter.com")].push_back(_T("*.twimg.com"));
    m_ecosystemRules[_T("twitter.com")].push_back(_T("*.t.co"));
    
    // GitHub 生态系统
 m_ecosystemRules[_T("github.com")].push_back(_T("*.github.com"));
    m_ecosystemRules[_T("github.com")].push_back(_T("*.githubusercontent.com"));
    m_ecosystemRules[_T("github.com")].push_back(_T("*.githubassets.com"));
    m_ecosystemRules[_T("github.com")].push_back(_T("*.github.io"));
    
    // Cloudflare 生态系统
    m_ecosystemRules[_T("cloudflare.com")].push_back(_T("*.cloudflare.com"));
    m_ecosystemRules[_T("cloudflare.com")].push_back(_T("*.cloudflarestream.com"));
    m_ecosystemRules[_T("cloudflare.com")].push_back(_T("*.workers.dev"));
    
    // Amazon 生态系统
    m_ecosystemRules[_T("amazon.com")].push_back(_T("*.amazon.com"));
    m_ecosystemRules[_T("amazon.com")].push_back(_T("*.amazonaws.com"));
    m_ecosystemRules[_T("amazon.com")].push_back(_T("*.cloudfront.net"));
    
    TRACE(_T("初始化了 %d 个生态系统规则\n"), m_ecosystemRules.size());
}

// 记录查询并检测关联
void CAssociatedDomainDetector::RecordQuery(const CString& domain, BOOL isGFW)
{
EnterCriticalSection(&m_cs);
    
    DWORD now = GetTickCount();
    
    // 记录查询
    QueryRecord record;
    record.domain = domain;
    record.domain.MakeLower();
    record.timestamp = now;
    record.isGFW = isGFW;
    m_recentQueries.push_back(record);
    
    // 清理超过1秒的旧记录
    while (!m_recentQueries.empty() && 
   now - m_recentQueries.front().timestamp > 1000) {
        m_recentQueries.pop_front();
    }
    
    // 检测关联：如果时间窗口内有GFW域名
 BOOL hasGFWInWindow = FALSE;
    for (const auto& rec : m_recentQueries) {
        if (rec.isGFW) {
      hasGFWInWindow = TRUE;
    break;
  }
    }
    
    // 如果有GFW域名，标记其他域名为临时关联
    if (hasGFWInWindow) {
        for (const auto& rec : m_recentQueries) {
  if (!rec.isGFW && rec.domain != domain) {
        // 临时关联，有效期30秒
    m_tempAssociations[rec.domain] = now + 30000;
       
     TRACE(_T("[时间窗口关联] %s <-> GFW域名\n"), rec.domain);
            }
        }
    }
    
    LeaveCriticalSection(&m_cs);
}

// 检查是否匹配生态系统规则
BOOL CAssociatedDomainDetector::MatchEcosystemRule(const CString& domain)
{
 CString normalizedDomain = domain;
    normalizedDomain.MakeLower();
    
    for (const auto& ecosystem : m_ecosystemRules) {
// ecosystem.first 是主域名（如 google.com）
   // ecosystem.second 是关联域名模式列表
     
        for (const auto& pattern : ecosystem.second) {
    if (WildcardMatch(normalizedDomain, pattern)) {
           TRACE(_T("[生态系统匹配] %s -> %s\n"), domain, ecosystem.first);
        return TRUE;
    }
        }
}
    
    return FALSE;
}

// 判断是否应该走海外DNS
BOOL CAssociatedDomainDetector::ShouldUseOverseasDNS(const CString& domain)
{
    EnterCriticalSection(&m_cs);
 
 CString normalizedDomain = domain;
    normalizedDomain.MakeLower();
    
    // 检查生态系统规则（最高优先级）
    if (MatchEcosystemRule(normalizedDomain)) {
        LeaveCriticalSection(&m_cs);
        return TRUE;
    }
    
    // 检查临时关联
    auto it = m_tempAssociations.find(normalizedDomain);
    if (it != m_tempAssociations.end()) {
        DWORD now = GetTickCount();
        if (now < it->second) {
            LeaveCriticalSection(&m_cs);
            TRACE(_T("[临时关联] %s 走海外DNS\n"), domain);
    return TRUE;  // 临时关联仍有效
        } else {
       m_tempAssociations.erase(it);  // 过期删除
        }
    }
    
    // 检查永久规则
    CString mainDomain = ExtractMainDomain(normalizedDomain);
    for (const auto& rule : m_permanentRules) {
        if (rule.second.count(normalizedDomain) > 0 || 
          rule.second.count(mainDomain) > 0) {
    LeaveCriticalSection(&m_cs);
            TRACE(_T("[永久规则] %s 走海外DNS\n"), domain);
       return TRUE;
        }
    }
 
    LeaveCriticalSection(&m_cs);
return FALSE;
}

// 手动学习关联规则
void CAssociatedDomainDetector::LearnAssociation(const CString& gfwDomain, 
      const std::vector<CString>& relatedDomains)
{
    EnterCriticalSection(&m_cs);
    
    CString mainDomain = ExtractMainDomain(gfwDomain);
 mainDomain.MakeLower();
    
    for (const auto& related : relatedDomains) {
        CString normalizedRelated = related;
        normalizedRelated.MakeLower();
        m_permanentRules[mainDomain].insert(normalizedRelated);
    }
 
    LeaveCriticalSection(&m_cs);
    
    // 保存到文件
    SaveAssociations();
}

// 清理过期的临时关联
void CAssociatedDomainDetector::CleanupExpired()
{
    EnterCriticalSection(&m_cs);
    
    DWORD now = GetTickCount();
    
    for (auto it = m_tempAssociations.begin(); it != m_tempAssociations.end(); ) {
 if (now >= it->second) {
 it = m_tempAssociations.erase(it);
        } else {
            ++it;
        }
    }
    
 LeaveCriticalSection(&m_cs);
}

// 保存关联规则
BOOL CAssociatedDomainDetector::SaveAssociations()
{
    // TODO: 实现持久化（JSON或二进制格式）
    return TRUE;
}

// 加载关联规则
BOOL CAssociatedDomainDetector::LoadAssociations()
{
    // TODO: 实现从文件加载
    return TRUE;
}
