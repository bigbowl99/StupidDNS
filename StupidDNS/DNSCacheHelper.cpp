// DNSCacheHelper.cpp: DNS缓存清除辅助类实现
//

#include "pch.h"
#include "DNSCacheHelper.h"
#include <windns.h>

#pragma comment(lib, "dnsapi.lib")

// 构造函数
CDNSCacheHelper::CDNSCacheHelper()
{
}

// 析构函数
CDNSCacheHelper::~CDNSCacheHelper()
{
}

// 清除DNS缓存（使用Windows API）
BOOL CDNSCacheHelper::FlushDNSCache()
{
    // 直接使用命令行方法，更可靠
  return FlushDNSCacheByCommand();
}

// 通过命令行清除DNS缓存
BOOL CDNSCacheHelper::FlushDNSCacheByCommand()
{
    TRACE(_T("[DNS缓存] 正在清除DNS缓存...\n"));
    
  STARTUPINFO si;
PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // 隐藏窗口
    
    ZeroMemory(&pi, sizeof(pi));
    
    // 执行命令：ipconfig /flushdns
    CString cmdLine = _T("ipconfig /flushdns");
    
 // 创建进程
    if (!CreateProcess(NULL, cmdLine.GetBuffer(), NULL, NULL, FALSE,
   CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
      TRACE(_T("[DNS缓存] 创建进程失败: %d\n"), GetLastError());
      cmdLine.ReleaseBuffer();
    return FALSE;
    }
  
    cmdLine.ReleaseBuffer();
    
    // 等待进程结束（最多等待5秒）
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 5000);
    
    // 获取退出代码
    DWORD exitCode = 0;
  GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
 CloseHandle(pi.hThread);
    
    if (waitResult == WAIT_OBJECT_0 && exitCode == 0) {
   TRACE(_T("[DNS缓存] 命令行清除DNS缓存成功\n"));
        return TRUE;
    } else {
        TRACE(_T("[DNS缓存] 命令行清除DNS缓存失败，退出代码: %d\n"), exitCode);
        return FALSE;
    }
}

// 检查是否有管理员权限
BOOL CDNSCacheHelper::HasAdminPrivileges()
{
    return IsUserAnAdmin();
}
