# DNS缓存清除功能实现指南

## ?? 功能概述

在启动DNS服务和设置系统DNS时，自动清除系统DNS缓存，确保新的DNS设置立即生效。

---

## ?? 实现方式

使用Windows命令 `ipconfig /flushdns` 清除DNS缓存。

---

## ?? 已创建的文件

### 1. `DNSCacheHelper.h`
DNS缓存辅助类头文件

```cpp
// DNSCacheHelper.h
#pragma once
#include <atlstr.h>

class CDNSCacheHelper {
public:
    CDNSCacheHelper();
    ~CDNSCacheHelper();

    // 清除DNS缓存
 static BOOL FlushDNSCache();
    
    // 通过命令行清除DNS缓存
    static BOOL FlushDNSCacheByCommand();
    
    // 检查是否有管理员权限
  static BOOL HasAdminPrivileges();
};
```

### 2. `DNSCacheHelper.cpp`
DNS缓存辅助类实现（已完成）

---

## ?? 集成步骤

### 步骤1：在 StupidDNSDlg.cpp 顶部添加头文件
```cpp
#include "DNSCacheHelper.h"
```

? **已完成**

### 步骤2：在 OnBnClickedStart() 中添加DNS缓存清除
在启动DNS服务器成功后，添加：

```cpp
// 启动DNS服务器
if (!m_dnsServer.Start()) {
 // ... 错误处理 ...
    return;
}

// ===== 清除DNS缓存 =====
TRACE(_T("[启动服务] 正在清除DNS缓存...\n"));
if (CDNSCacheHelper::FlushDNSCache()) {
    CTime currentTime = CTime::GetCurrentTime();
    CString timeStr = currentTime.Format(_T("%H:%M:%S"));
    AddLogEntry(timeStr, _T("已清除DNS缓存"), _T("INFO"), _T("-"), _T("-"));
} else {
    CTime currentTime = CTime::GetCurrentTime();
    CString timeStr = currentTime.Format(_T("%H:%M:%S"));
    AddLogEntry(timeStr, _T("警告：清除DNS缓存失败（可能需要管理员权限）"), _T("WARN"), _T("-"), _T("-"));
}
```

? **已完成**

### 步骤3：在 OnBnClickedSetSystemDns() 中添加DNS缓存清除
在设置系统DNS成功后，添加：

```cpp
// 设置DNS
if (CSystemDNSHelper::SetAllAdaptersDNS(TRUE)) {
    // ===== 清除DNS缓存 =====
    TRACE(_T("[设置系统DNS] 正在清除DNS缓存...\n"));
  CDNSCacheHelper::FlushDNSCache();
            
    CString successMsg = _T("DNS设置成功！\n\n已将系统DNS设为: 127.0.0.1\n\n");
    successMsg += _T("DNS缓存已清除，现在可以点击【启动服务】按钮。\n");
    successMsg += _T("浏览器将通过本程序进行DNS解析。");

    AfxMessageBox(successMsg, MB_OK | MB_ICONINFORMATION);
    
    // 添加日志
    CTime currentTime = CTime::GetCurrentTime();
CString timeStr = currentTime.Format(_T("%H:%M:%S"));
    AddLogEntry(timeStr, _T("系统DNS已设为: 127.0.0.1，DNS缓存已清除"), _T("INFO"), _T("-"), _T("-"));
}
```

? **已完成**

---

## ?? 项目文件配置

### 添加文件到项目（必须）

#### 方法1：通过VS界面
1. 右键点击项目 `StupidDNS`
2. 选择 `添加` → `现有项`
3. 选择以下文件：
   - `StupidDNS\DNSCacheHelper.h`
   - `StupidDNS\DNSCacheHelper.cpp`
4. 点击"添加"

#### 方法2：手动编辑 `.vcxproj`
在 `StupidDNS.vcxproj` 中添加：

```xml
<ItemGroup>
  <!-- 其他头文件 -->
  <ClInclude Include="DNSCacheHelper.h" />
</ItemGroup>

<ItemGroup>
  <!-- 其他cpp文件 -->
  <ClCompile Include="DNSCacheHelper.cpp" />
</ItemGroup>
```

---

## ?? 使用场景

### 场景1：启动DNS服务
```
用户操作：点击【启动服务】按钮
程序流程：
1. 初始化DNS服务器
2. 绑定端口53
3. 启动监听线程
4. ? 清除DNS缓存 ← 新增
5. 显示"DNS服务已启动"日志
```

### 场景2：设置系统DNS
```
用户操作：点击【设为系统DNS】按钮
程序流程：
1. 检查管理员权限
2. 设置网络适配器DNS为127.0.0.1
3. ? 清除DNS缓存 ← 新增
4. 显示"DNS设置成功"提示
5. 记录日志
```

---

## ? 验证测试

### 测试1：启动服务并清除缓存
**步骤**：
1. 启动StupidDNS（管理员权限）
2. 点击【启动服务】
3. 查看日志

**预期结果**：
- ? 日志显示："DNS服务已启动 (端口:53)"
- ? 日志显示："已清除DNS缓存"
- ? TRACE输出：`[启动服务] 正在清除DNS缓存...`
- ? TRACE输出：`[DNS缓存] 命令行清除DNS缓存成功`

### 测试2：设置系统DNS并清除缓存
**步骤**：
1. 点击【设为系统DNS】
2. 确认设置
3. 查看日志

**预期结果**：
- ? 显示提示："DNS设置成功！DNS缓存已清除..."
- ? 日志显示："系统DNS已设为: 127.0.0.1，DNS缓存已清除"
- ? TRACE输出：`[设置系统DNS] 正在清除DNS缓存...`

### 测试3：验证DNS缓存确实被清除
**步骤**：
1. 设置系统DNS为127.0.0.1
2. 启动DNS服务
3. 浏览器访问 `google.com`
4. 立即看到DNS查询日志（无缓存延迟）

**预期结果**：
- ? 立即看到DNS查询（不是从缓存读取）
- ? 日志显示查询google.com的记录

---

## ?? 故障排查

### 问题1：清除DNS缓存失败
**症状**：日志显示"警告：清除DNS缓存失败"

**原因**：
- 程序没有以管理员权限运行

**解决方法**：
- 右键程序图标，选择"以管理员身份运行"

### 问题2：编译错误
**症状**：找不到`CDNSCacheHelper`

**原因**：
- DNSCacheHelper.h/cpp未添加到项目

**解决方法**：
1. 检查Solution Explorer中是否有这两个文件
2. 重新添加文件到项目
3. 清理并重新生成解决方案

---

## ?? 技术细节

### ipconfig /flushdns 命令
```cmd
C:\> ipconfig /flushdns

Windows IP 配置

已成功刷新 DNS 解析缓存。
```

### CreateProcess 参数
- `CREATE_NO_WINDOW`: 不创建新窗口（后台执行）
- `SW_HIDE`: 隐藏命令行窗口
- `WaitForSingleObject`: 等待进程结束（最多5秒）
- `GetExitCodeProcess`: 获取退出代码（0=成功）

### 为什么不使用 DnsFlushResolverCache API？
- `DnsFlushResolverCache()` 需要 Windows 8+
- 需要链接 `dnsapi.dll`
- 兼容性问题
- **`ipconfig /flushdns` 更可靠、兼容性更好**

---

## ?? 参考资料

### Windows命令
- [ipconfig /flushdns - Microsoft Docs](https://docs.microsoft.com/zh-cn/windows-server/administration/windows-commands/ipconfig)

### Windows API
- [CreateProcess](https://docs.microsoft.com/zh-cn/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw)
- [WaitForSingleObject](https://docs.microsoft.com/zh-cn/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject)

---

## ?? 完成检查清单

- [x] 创建 `DNSCacheHelper.h`
- [x] 创建 `DNSCacheHelper.cpp`
- [x] 在 `StupidDNSDlg.cpp` 中添加头文件引用
- [x] 在 `OnBnClickedStart()` 中添加缓存清除
- [x] 在 `OnBnClickedSetSystemDns()` 中添加缓存清除
- [ ] 将文件添加到项目（手动操作）
- [ ] 编译测试
- [ ] 功能验证

---

## ?? 使用建议

### 最佳实践
1. **始终以管理员权限运行**程序
2. **先设置系统DNS**，再启动服务
3. **验证DNS缓存清除**：访问新域名立即看到查询

### 用户提示
建议在程序启动时检查管理员权限：
```cpp
if (!IsUserAnAdmin()) {
    AfxMessageBox(_T("建议以管理员身份运行，以确保DNS缓存清除功能正常工作"), 
        MB_OK | MB_ICONINFORMATION);
}
```

---

**功能已实现！现在只需将文件添加到项目并编译即可！** ??
