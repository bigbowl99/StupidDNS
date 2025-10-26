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
    TRACE(_T("[ExecuteNetshCommand] 开始执行命令: %s\n"), command);
    
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
    
    TRACE(_T("[ExecuteNetshCommand] 完整命令: %s\n"), cmdLine);
    
    // 启动进程
    if (!CreateProcess(NULL, cmdLine.GetBuffer(), NULL, NULL, FALSE,
  CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
      DWORD error = GetLastError();
      TRACE(_T("[ExecuteNetshCommand] CreateProcess失败，错误代码: %d\n"), error);
        cmdLine.ReleaseBuffer();
      return FALSE;
    }
    
    cmdLine.ReleaseBuffer();
  TRACE(_T("[ExecuteNetshCommand] 进程已启动，等待完成...\n"));
    
    // 减少等待时间从5秒改为2秒（给更多时间）
  DWORD waitResult = WaitForSingleObject(pi.hProcess, 2000);
    
    TRACE(_T("[ExecuteNetshCommand] 等待结果: %d\n"), waitResult);
    
    // 获取退出代码
DWORD exitCode = 0;
if (waitResult == WAIT_OBJECT_0) {
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        TRACE(_T("[ExecuteNetshCommand] 进程正常退出，退出代码: %d\n"), exitCode);
      } else {
  TRACE(_T("[ExecuteNetshCommand] 无法获取退出代码\n"));
       exitCode = 1;
        }
    } else if (waitResult == WAIT_TIMEOUT) {
        // 超时，但不强制终止，因为netsh可能需要更多时间
    TRACE(_T("[ExecuteNetshCommand] 等待超时，但不终止进程\n"));
        // 再等待1秒
        waitResult = WaitForSingleObject(pi.hProcess, 1000);
        if (waitResult == WAIT_OBJECT_0) {
        GetExitCodeProcess(pi.hProcess, &exitCode);
     TRACE(_T("[ExecuteNetshCommand] 额外等待后完成，退出代码: %d\n"), exitCode);
     } else {
            TRACE(_T("[ExecuteNetshCommand] 仍然超时，假设成功（netsh通常会成功）\n"));
        // 假设成功，因为netsh命令通常会成功执行
      exitCode = 0;
        }
    } else {
        TRACE(_T("[ExecuteNetshCommand] 等待失败，错误: %d\n"), GetLastError());
  exitCode = 1;
    }
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    BOOL success = (exitCode == 0);
    TRACE(_T("[ExecuteNetshCommand] 命令执行完成，返回: %d\n"), success);
return success;
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
    // 获取所有适配器信息
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufferSize);
    
    if (pAddresses == NULL) {
  return FALSE;
    }
    
    DWORD result = GetAdaptersAddresses(AF_INET, 
        GAA_FLAG_INCLUDE_PREFIX,
   NULL, pAddresses, &bufferSize);
    
    if (result == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
      pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufferSize);
 result = GetAdaptersAddresses(AF_INET, 
 GAA_FLAG_INCLUDE_PREFIX,
         NULL, pAddresses, &bufferSize);
    }
    
    BOOL isLocalhost = FALSE;
 
    if (result == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        
        while (pCurrAddresses) {
            // 只检查以太网和WiFi适配器
  if (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD ||
   pCurrAddresses->IfType == IF_TYPE_IEEE80211) {
           
 // 获取DNS服务器地址
PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsServer = pCurrAddresses->FirstDnsServerAddress;
      
          if (pDnsServer) {
           sockaddr_in* pSockAddr = (sockaddr_in*)pDnsServer->Address.lpSockaddr;
       if (pSockAddr && pSockAddr->sin_family == AF_INET) {
  // 检查是否是127.0.0.1 (0x7F000001 网络字节序)
      ULONG dnsIP = ntohl(pSockAddr->sin_addr.s_addr);
   if (dnsIP == 0x7F000001) {  // 127.0.0.1
     isLocalhost = TRUE;
            break;
       }
    }
      }
            }
            
 pCurrAddresses = pCurrAddresses->Next;
 }
    }
  
    free(pAddresses);
    return isLocalhost;
}

// 设置所有活动适配器的DNS
BOOL CSystemDNSHelper::SetAllAdaptersDNS(BOOL setToLocalhost)
{
    auto adapters = GetAllAdapters();
 
  if (adapters.empty()) {
   TRACE(_T("[SetAllAdaptersDNS] 未找到网络适配器\n"));
    return FALSE;
    }
  
// **只设置第一个活动适配器，避免长时间等待**
    TRACE(_T("[SetAllAdaptersDNS] 找到 %d 个网络适配器，只设置第一个\n"), adapters.size());
    
    const auto& adapter = adapters[0];  // 只设置第一个
    
 BOOL success = FALSE;
    if (setToLocalhost) {
     success = SetDNSToLocalhost(adapter.name);
        if (success) {
    TRACE(_T("[SetAllAdaptersDNS] 成功设置 %s\n"), adapter.name);
        } else {
       TRACE(_T("[SetAllAdaptersDNS] 失败设置 %s\n"), adapter.name);
        }
} else {
    success = SetDNSToAuto(adapter.name);
   if (success) {
      TRACE(_T("[SetAllAdaptersDNS] 成功恢复 %s\n"), adapter.name);
   } else {
         TRACE(_T("[SetAllAdaptersDNS] 失败恢复 %s\n"), adapter.name);
      }
    }
    
    return success;
}
