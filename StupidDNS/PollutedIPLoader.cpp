// PollutedIPLoader.cpp - 污染IP加载器实现
//

#include "pch.h"
#include "PollutedIPLoader.h"
#include <fstream>

CPollutedIPLoader::CPollutedIPLoader()
{
}

CPollutedIPLoader::~CPollutedIPLoader()
{
}

// 加载污染IP列表
BOOL CPollutedIPLoader::LoadFromFile(const CString& filePath)
{
    Clear();

    CStdioFile file;
    if (!file.Open(filePath, CFile::modeRead | CFile::typeText)) {
        return FALSE;
    }
  
    CString line;
    int loadedCount = 0;
    
    while (file.ReadString(line)) {
   line.Trim();
        
        // 跳过空行和注释
        if (line.IsEmpty() || line[0] == _T('#')) {
   continue;
        }
        
  // 解析 bogus-nxdomain= 行
        CString ip;
      if (ParseBogusLine(line, ip)) {
     ip = NormalizeIP(ip);
            if (!ip.IsEmpty()) {
     m_pollutedIPs.insert(ip);
        loadedCount++;
            }
      }
    }
    
  file.Close();
    
    return (loadedCount > 0);
}

// 检查IP是否被污染
BOOL CPollutedIPLoader::IsPollutedIP(const CString& ip) const
{
    CString normalizedIP = NormalizeIP(ip);
    return (m_pollutedIPs.find(normalizedIP) != m_pollutedIPs.end());
}

// 获取污染IP数量
int CPollutedIPLoader::GetPollutedIPCount() const
{
    return (int)m_pollutedIPs.size();
}

// 清除污染IP列表
void CPollutedIPLoader::Clear()
{
    m_pollutedIPs.clear();
}

// 解析bogus-nxdomain行
BOOL CPollutedIPLoader::ParseBogusLine(const CString& line, CString& outIP)
{
    // 格式: bogus-nxdomain=123.125.81.12
    int pos = line.Find(_T("bogus-nxdomain="));
    if (pos == -1) {
        return FALSE;
    }
  
    // 提取IP部分
    int start = pos + 15;  // "bogus-nxdomain=" 的长度
    outIP = line.Mid(start);
    outIP.Trim();
    
    return !outIP.IsEmpty();
}

// 规范化IP地址
CString CPollutedIPLoader::NormalizeIP(const CString& ip) const
{
    CString normalized = ip;
    normalized.Trim();
    normalized.MakeLower();  // 转小写（虽然IP不区分大小写，但统一处理）
    return normalized;
}
