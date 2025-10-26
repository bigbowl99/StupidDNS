// DNSConfigCache.cpp - DNS配置缓存管理实现（简化版）
//

#include "pch.h"
#include "DNSConfigCache.h"
#include <shlobj.h>
#include <algorithm>
#include <sstream>

#pragma comment(lib, "shell32.lib")

CDNSConfigCache::CDNSConfigCache()
    : m_lastUpdateTime(0)
    , m_nExpireDays(7)
{
    m_strConfigFile = GetConfigFilePath();
}

CDNSConfigCache::~CDNSConfigCache()
{
}

// 获取配置文件路径
CString CDNSConfigCache::GetConfigFilePath()
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath)))
    {
   CString appDataPath = szPath;
 appDataPath += _T("\\StupidDNS");
  
        // 创建目录（如果不存在）
        CreateDirectory(appDataPath, NULL);
   
return appDataPath + _T("\\dns_config.json");
    }
    
    // 回退：使用程序目录
    return _T(".\\dns_config.json");
}

// 加载配置（简化版）
BOOL CDNSConfigCache::LoadConfig()
{
    // 暂时返回FALSE，让程序使用默认配置
// TODO: 实现完整的JSON解析
    return FALSE;
}

// 保存配置（简化版）
BOOL CDNSConfigCache::SaveConfig()
{
  // 暂时返回TRUE，不报错
    // TODO: 实现完整的JSON生成
 return TRUE;
}

// 检查配置是否过期
BOOL CDNSConfigCache::IsConfigExpired()
{
    if (m_lastUpdateTime == 0) {
   return TRUE;  // 未加载配置
    }
    
    time_t now = time(NULL);
    time_t expireTime = m_lastUpdateTime + (m_nExpireDays * 24 * 3600);
    
return (now > expireTime);
}

// 获取国内DNS列表
std::vector<DNSServerInfo> CDNSConfigCache::GetDomesticDNS()
{
 return m_domesticDNS;
}

// 获取海外DNS列表
std::vector<DNSServerInfo> CDNSConfigCache::GetOverseasDNS()
{
    return m_overseasDNS;
}

// 更新DNS测速结果（简化版）
void CDNSConfigCache::UpdateDNSResult(const CString& ip, int latency, bool success)
{
    // 简化实现：仅记录IP和延迟
    // TODO: 完整实现需要匹配DNS服务器并更新
}

// 清除配置缓存
void CDNSConfigCache::ClearConfig()
{
    m_domesticDNS.clear();
m_overseasDNS.clear();
    m_lastUpdateTime = 0;
    
    // 删除配置文件
    DeleteFile(m_strConfigFile);
}

// 简化版：暂不实现复杂的JSON解析
BOOL CDNSConfigCache::ParseJSON(const CString& json)
{
return FALSE;
}

// 简化版：暂不实现JSON生成
CString CDNSConfigCache::GenerateJSON()
{
    return _T("");
}

// 读取文件内容
CString CDNSConfigCache::ReadFile(const CString& filePath)
{
    CStdioFile file;
  if (!file.Open(filePath, CFile::modeRead | CFile::typeText)) {
 return _T("");
}
    
    CString content, line;
while (file.ReadString(line)) {
        content += line + _T("\n");
  }
    
    file.Close();
    return content;
}

// 写入文件内容
BOOL CDNSConfigCache::WriteFile(const CString& filePath, const CString& content)
{
 CStdioFile file;
    if (!file.Open(filePath, CFile::modeCreate | CFile::modeWrite | CFile::typeText)) {
        return FALSE;
    }
    
    file.WriteString(content);
file.Close();
    
    return TRUE;
}
