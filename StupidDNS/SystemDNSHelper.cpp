// SystemDNSHelper.cpp: 系统DNS设置辅助类实现
//

#include "pch.h"
#include "SystemDNSHelper.h"
#include <iphlpapi.h>
#include <atlconv.h>

#pragma comment(lib, "iphlpapi.lib")

// 构造函数
CSystemDNSHelper::CSystemDNSHelper()
{
}

// 析构函数
CSystemDNSHelper::~CSystemDNSHelper()
{
}

// 获取所有网络适配器
std::vector<NetworkAdapter> CSystemDNSHelper::GetAllAdapters()
{
    std::vector<NetworkAdapter> adapters;
    
    // 获取适配器信息
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufferSize);
    
    if (pAddresses == NULL) {
        return adapters;
    }
    
    DWORD result = GetAdaptersAddresses(AF_INET, 
        GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS,
  NULL, pAddresses, &bufferSize);
    
    if (result == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufferSize);
        result = GetAdaptersAddresses(AF_INET, 
    GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS,
            NULL, pAddresses, &bufferSize);
    }
    
    if (result == NO_ERROR) {
     PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
      
      while (pCurrAddresses) {
   // 只添加以太网和WiFi适配器
 if (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD ||
   pCurrAddresses->IfType == IF_TYPE_IEEE80211) {
                
          NetworkAdapter adapter;
   adapter.name = CString(pCurrAddresses->FriendlyName);
      adapter.description = CString(pCurrAddresses->Description);
             
       // 转换GUID
        WCHAR guidStr[40];
       StringFromGUID2(pCurrAddresses->NetworkGuid, guidStr, 40);
     adapter.guid = CString(guidStr);
     
 adapter.isDHCPEnabled = (pCurrAddresses->Dhcpv4Enabled != 0);
     
     // TODO: 获取当前DNS
     adapter.currentDNS = _T("");
    
    adapters.push_back(adapter);
        }
      
 pCurrAddresses = pCurrAddresses->Next;
        }
    }
    
 free(pAddresses);
    return adapters;
}

// 执行netsh命令
BOOL CSystemDNSHelper::ExecuteNetshCommand(const CString& command)
{
    TRACE(_T("执行命令: %s\n"), command);
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // 隐藏窗口
    
    ZeroMemory(&pi, sizeof(pi));
    
    // 创建命令行
    CString cmdLine;
    cmdLine.Format(_T("netsh %s"), command);
    
    // 启动进程
    if (!CreateProcess(NULL, cmdLine.GetBuffer(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
  TRACE(_T("CreateProcess失败: %d\n"), GetLastError());
   return FALSE;
    }
    
    cmdLine.ReleaseBuffer();
    
    // 等待进程结束
    WaitForSingleObject(pi.hProcess, 5000);
    
 // 获取退出代码
    DWORD exitCode;
 GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return (exitCode == 0);
}

// 获取主要网络适配器
CString CSystemDNSHelper::GetPrimaryAdapter()
{
    auto adapters = GetAllAdapters();
    
    if (adapters.empty()) {
   return _T("");
    }
    
    // 返回第一个适配器的名称
    return adapters[0].name;
}

// 设置DNS为127.0.0.1
BOOL CSystemDNSHelper::SetDNSToLocalhost(const CString& adapterName)
{
    CString command;
    
    // 设置主DNS
    command.Format(_T("interface ip set dns name=\"%s\" static 127.0.0.1 primary"), 
        adapterName);
    
 if (!ExecuteNetshCommand(command)) {
    return FALSE;
    }
    
    TRACE(_T("已设置 %s 的DNS为 127.0.0.1\n"), adapterName);
    return TRUE;
}

// 设置DNS为自动获取
BOOL CSystemDNSHelper::SetDNSToAuto(const CString& adapterName)
{
    CString command;
    
    // 设置为DHCP
    command.Format(_T("interface ip set dns name=\"%s\" dhcp"), 
   adapterName);
    
    if (!ExecuteNetshCommand(command)) {
   return FALSE;
    }
    
    TRACE(_T("已设置 %s 的DNS为自动获取\n"), adapterName);
 return TRUE;
}

// 检查是否已设置为127.0.0.1
BOOL CSystemDNSHelper::IsSetToLocalhost(const CString& adapterName)
{
    // 通过注册表检查DNS设置
 // 简化实现：执行ipconfig /all并解析输出
    // 这里返回FALSE，实际使用时需要完整实现
    return FALSE;
}

// 设置所有活动适配器的DNS
BOOL CSystemDNSHelper::SetAllAdaptersDNS(BOOL setToLocalhost)
{
    auto adapters = GetAllAdapters();
    
    if (adapters.empty()) {
    TRACE(_T("未找到网络适配器\n"));
    return FALSE;
    }
    
    BOOL success = TRUE;
    int count = 0;
    
    for (const auto& adapter : adapters) {
   if (setToLocalhost) {
      if (SetDNSToLocalhost(adapter.name)) {
             count++;
     } else {
       success = FALSE;
      }
    } else {
   if (SetDNSToAuto(adapter.name)) {
 count++;
      } else {
     success = FALSE;
       }
   }
    }
    
    TRACE(_T("已设置 %d 个适配器的DNS\n"), count);
    return success && count > 0;
}
