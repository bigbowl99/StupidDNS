// DNSCacheHelper.h: DNS缓存清除辅助类
//

#pragma once

#include <atlstr.h>

// DNS缓存辅助类
class CDNSCacheHelper {
public:
    CDNSCacheHelper();
    ~CDNSCacheHelper();

    // 清除DNS缓存
    static BOOL FlushDNSCache();
    
    // 通过命令行清除DNS缓存（备用方法）
    static BOOL FlushDNSCacheByCommand();
    
    // 检查是否有管理员权限
    static BOOL HasAdminPrivileges();
};
