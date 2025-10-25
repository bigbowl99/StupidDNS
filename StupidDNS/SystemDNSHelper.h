// SystemDNSHelper.h: 系统DNS设置辅助类
//

#pragma once

#include <vector>

// 网络适配器信息
struct NetworkAdapter {
    CString name;         // 适配器名称
    CString guid;// GUID
    CString description;    // 描述
    BOOL isDHCPEnabled;     // 是否启用DHCP
    CString currentDNS;     // 当前DNS
};

// 系统DNS设置辅助类
class CSystemDNSHelper {
public:
    CSystemDNSHelper();
    ~CSystemDNSHelper();
    
    // 获取所有网络适配器
    static std::vector<NetworkAdapter> GetAllAdapters();
    
    // 设置DNS为127.0.0.1
    static BOOL SetDNSToLocalhost(const CString& adapterName);
    
    // 设置DNS为自动获取（DHCP）
    static BOOL SetDNSToAuto(const CString& adapterName);
    
    // 检查是否已设置为127.0.0.1
    static BOOL IsSetToLocalhost(const CString& adapterName);
 
    // 设置所有活动适配器的DNS
    static BOOL SetAllAdaptersDNS(BOOL setToLocalhost);
    
    // 执行netsh命令
    static BOOL ExecuteNetshCommand(const CString& command);
    
    // 获取主要网络适配器（正在使用的）
    static CString GetPrimaryAdapter();
};
