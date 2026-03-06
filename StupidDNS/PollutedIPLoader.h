// PollutedIPLoader.h - 污染IP加载器
//

#pragma once

#include <vector>
#include <set>
#include <atlstr.h>

// 污染IP加载器
class CPollutedIPLoader {
public:
    CPollutedIPLoader();
    ~CPollutedIPLoader();

    // 加载污染IP列表
    BOOL LoadFromFile(const CString& filePath);
    
    // 检查IP是否被污染
    BOOL IsPollutedIP(const CString& ip) const;
    
    // 获取污染IP数量
    int GetPollutedIPCount() const;
    
    // 清除污染IP列表
    void Clear();
    
private:
    std::set<CString> m_pollutedIPs;  // 使用set快速查找
    
    // 解析bogus-nxdomain行
    BOOL ParseBogusLine(const CString& line, CString& outIP);
    
    // 规范化IP地址（去除空格等）
    CString NormalizeIP(const CString& ip) const;
};
