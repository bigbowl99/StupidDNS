// DomainRuleLoader.cpp: 域名规则加载器实现
//

#include "pch.h"
#include "DomainRuleLoader.h"
#include <atlpath.h>

// 构造函数
CDomainRuleLoader::CDomainRuleLoader()
{
}

// 析构函数
CDomainRuleLoader::~CDomainRuleLoader()
{
}

// 获取规则文件目录
CString CDomainRuleLoader::GetRulesDirectory() const
{
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    CPath pathExe(szPath);
    pathExe.RemoveFileSpec();
    pathExe.Append(_T("rules"));
    return CString(pathExe);
}

// 解析dnsmasq格式的行：server=/domain/ip
CString CDomainRuleLoader::ParseDnsmasqLine(const CString& line)
{
    // 查找 server=/ 开头
    int startPos = line.Find(_T("server=/"));
    if (startPos == -1) return _T("");
    
  // 跳过 "server=/"
  int domainStart = startPos + 8;
    
    // 查找结束的 /
    int domainEnd = line.Find(_T('/'), domainStart);
    if (domainEnd == -1) return _T("");
    
    // 提取域名
    CString domain = line.Mid(domainStart, domainEnd - domainStart);
    domain.Trim();
    
    return domain;
}

// 提取主域名
CString CDomainRuleLoader::ExtractMainDomain(const CString& domain) const
{
    // 处理二级后缀（.com.cn, .co.uk 等）
    static const LPCTSTR secondLevelTLDs[] = {
        _T("com.cn"), _T("net.cn"), _T("org.cn"), _T("gov.cn"),
        _T("co.uk"), _T("co.jp"), _T("co.kr")
    };
    
    // 检查是否有二级后缀
    for (int i = 0; i < _countof(secondLevelTLDs); i++) {
      CString tld = secondLevelTLDs[i];
        CString suffix = _T(".") + tld;
        
        if (domain.GetLength() > suffix.GetLength() &&
            domain.Right(suffix.GetLength()) == suffix) {
     // 找到二级后缀，提取前面的部分
            int pos = domain.GetLength() - suffix.GetLength();
            int prevDot = domain.Left(pos).ReverseFind(_T('.'));
    if (prevDot != -1) {
      return domain.Mid(prevDot + 1);
            }
  return domain;  // 已经是主域名
        }
    }
    
    // 普通后缀处理
    int lastDot = domain.ReverseFind(_T('.'));
    if (lastDot == -1) return domain;
    
    int secondLastDot = domain.Left(lastDot).ReverseFind(_T('.'));
    if (secondLastDot == -1) return domain;  // 已经是主域名
    
    return domain.Mid(secondLastDot + 1);
}

// 域名模式匹配
BOOL CDomainRuleLoader::MatchDomain(const CString& domain, const CString& pattern) const
{
    // 完全匹配
    if (domain == pattern) return TRUE;
    
    // 如果 pattern 以 . 开头，匹配后缀
    if (pattern.GetLength() > 0 && pattern[0] == _T('.')) {
        if (domain.Right(pattern.GetLength()) == pattern) {
            return TRUE;
        }
    }
 
    // 如果 domain 以 pattern 结尾（子域名匹配）
    // www.google.com 匹配 google.com
    if (domain.GetLength() >= pattern.GetLength()) {
   if (domain.Right(pattern.GetLength()) == pattern) {
       // 确保是完整的域名组件匹配
        int pos = domain.GetLength() - pattern.GetLength() - 1;
      if (pos < 0 || domain[pos] == _T('.')) {
       return TRUE;
         }
        }
    }
    
    return FALSE;
}

// 加载中国域名白名单
BOOL CDomainRuleLoader::LoadChinaDomains(const CString& filePath)
{
    m_chinaDomains.clear();
    
    CStdioFile file;
    if (!file.Open(filePath, CFile::modeRead | CFile::typeText)) {
     TRACE(_T("无法打开中国域名列表: %s\n"), filePath);
        return FALSE;
}
    
    CString line;
    int count = 0;
    
    while (file.ReadString(line)) {
        line.Trim();

        // 跳过空行和注释
if (line.IsEmpty() || line[0] == _T('#')) continue;
        
    // 解析 server=/domain/ip 格式
        CString domain = ParseDnsmasqLine(line);
        if (!domain.IsEmpty()) {
     domain.MakeLower();
            m_chinaDomains.insert(domain);
            count++;
        }
    }
    
    file.Close();
    
    TRACE(_T("加载了 %d 个中国域名\n"), count);
    return TRUE;
}

// 加载GFW域名黑名单
BOOL CDomainRuleLoader::LoadGFWList(const CString& filePath)
{
    m_gfwDomains.clear();
    
    CStdioFile file;
    if (!file.Open(filePath, CFile::modeRead | CFile::typeText)) {
    TRACE(_T("无法打开GFW域名列表: %s\n"), filePath);
  return FALSE;
    }
    
    CString line;
    int count = 0;
    
 while (file.ReadString(line)) {
        line.Trim();
        
        // 跳过空行、注释、特殊字符开头的行
        if (line.IsEmpty() || 
  line[0] == _T('#') || 
     line[0] == _T('!') || 
     line[0] == _T('[')) {
       continue;
        }
   
        // 去掉可能的协议前缀（http://, https://）
        int protocolPos = line.Find(_T("://"));
        if (protocolPos != -1) {
            line = line.Mid(protocolPos + 3);
      }
        
        // 去掉路径部分（/之后的内容）
        int slashPos = line.Find(_T('/'));
        if (slashPos != -1) {
    line = line.Left(slashPos);
      }
        
        // 添加到集合
        if (!line.IsEmpty()) {
        line.MakeLower();
        m_gfwDomains.insert(line);
       count++;
        }
    }
    
 file.Close();
    
    TRACE(_T("加载了 %d 个GFW域名\n"), count);
    return TRUE;
}

// 加载污染IP黑名单
BOOL CDomainRuleLoader::LoadPoisonIPs(const CString& filePath)
{
    m_poisonIPs.clear();
    
    CStdioFile file;
    if (!file.Open(filePath, CFile::modeRead | CFile::typeText)) {
        TRACE(_T("无法打开污染IP列表: %s\n"), filePath);
        return FALSE;
    }
    
    CString line;
    int count = 0;
    
    while (file.ReadString(line)) {
        line.Trim();
        
  // 跳过空行和注释
        if (line.IsEmpty() || line[0] == _T('#')) continue;
        
        // 查找 bogus-nxdomain=
        int pos = line.Find(_T("bogus-nxdomain="));
 if (pos != -1) {
     CString ip = line.Mid(pos + 15);  // 15 = strlen("bogus-nxdomain=")
ip.Trim();
            
   if (!ip.IsEmpty()) {
 m_poisonIPs.insert(ip);
       count++;
    }
        }
    }
    
file.Close();
    
    TRACE(_T("加载了 %d 个污染IP\n"), count);
    return TRUE;
}

// 加载所有规则文件
BOOL CDomainRuleLoader::LoadAllRules()
{
    CString rulesDir = GetRulesDirectory();
    
    // 检查目录是否存在
 if (GetFileAttributes(rulesDir) == INVALID_FILE_ATTRIBUTES) {
     TRACE(_T("规则目录不存在: %s\n"), rulesDir);
   return FALSE;
    }
    
    BOOL success = TRUE;
    
    // 加载中国域名
    CString chinaFile = rulesDir + _T("\\accelerated-domains.china.conf");
    if (!LoadChinaDomains(chinaFile)) {
     TRACE(_T("警告: 加载中国域名失败\n"));
        success = FALSE;
    }
    
    // 加载GFW列表
    CString gfwFile = rulesDir + _T("\\gfw.txt");
    if (!LoadGFWList(gfwFile)) {
     TRACE(_T("警告: 加载GFW列表失败\n"));
   success = FALSE;
    }
    
    // 加载污染IP
    CString poisonFile = rulesDir + _T("\\bogus-nxdomain.china.conf");
    if (!LoadPoisonIPs(poisonFile)) {
        TRACE(_T("警告: 加载污染IP失败\n"));
        success = FALSE;
    }
    
    return success;
}

// 判断是否是中国域名
BOOL CDomainRuleLoader::IsChineseDomain(const CString& domain) const
{
    // 标准化域名（转小写）
    CString normalizedDomain = domain;
    normalizedDomain.MakeLower();
    normalizedDomain.Trim();
    
    // 直接匹配
    if (m_chinaDomains.count(normalizedDomain) > 0) {
    return TRUE;
    }
    
    // 提取主域名匹配（www.baidu.com -> baidu.com）
    CString mainDomain = ExtractMainDomain(normalizedDomain);
    if (m_chinaDomains.count(mainDomain) > 0) {
        return TRUE;
    }
    
    // 通配符匹配（.cn 后缀）
    for (const auto& pattern : m_chinaDomains) {
      if (MatchDomain(normalizedDomain, pattern)) {
  return TRUE;
      }
    }
    
    return FALSE;
}

// 判断是否是GFW域名
BOOL CDomainRuleLoader::IsGFWDomain(const CString& domain) const
{
  CString normalizedDomain = domain;
    normalizedDomain.MakeLower();
    normalizedDomain.Trim();
    
    // 直接匹配
    if (m_gfwDomains.count(normalizedDomain) > 0) {
        return TRUE;
    }
    
    // 主域名匹配
    CString mainDomain = ExtractMainDomain(normalizedDomain);
    if (m_gfwDomains.count(mainDomain) > 0) {
        return TRUE;
    }
    
    return FALSE;
}

// 判断是否是污染IP
BOOL CDomainRuleLoader::IsPoisonIP(const CString& ip) const
{
    return m_poisonIPs.count(ip) > 0;
}
