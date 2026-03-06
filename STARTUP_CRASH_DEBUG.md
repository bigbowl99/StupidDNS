# ?? 程序无法启动诊断指南

## ?? 症状
- 双击程序后无反应
- 任务管理器中找不到进程
- 没有窗口显示

---

## ?? 诊断步骤

### 步骤1：检查是否生成了exe文件
**位置**：`D:\projects\StupidDNS\x64\Debug\StupidDNS.exe`

```cmd
dir D:\projects\StupidDNS\x64\Debug\StupidDNS.exe
```

如果文件不存在，说明编译未成功。

---

### 步骤2：检查事件查看器
1. 按 `Win+R`，输入 `eventvwr.msc`
2. 展开 `Windows 日志` → `应用程序`
3. 查找最近的错误事件
4. 查找与`StupidDNS.exe`相关的错误

**可能的错误**：
- 缺少DLL文件（如`mfc140ud.dll`、`vcruntime140d.dll`）
- 访问冲突（Access Violation）
- 堆栈溢出（Stack Overflow）

---

### 步骤3：使用命令行启动（查看错误信息）
```cmd
cd D:\projects\StupidDNS\x64\Debug
StupidDNS.exe
```

**如果看到错误消息**，记录下来并继续诊断。

---

### 步骤4：检查依赖项
使用 Dependency Walker 检查缺失的DLL：

1. 下载 [Dependency Walker](http://www.dependencywalker.com/)
2. 打开 `StupidDNS.exe`
3. 查看红色标记的缺失DLL

**常见缺失的DLL**：
- `mfc140ud.dll` (MFC Debug DLL)
- `msvcp140d.dll` (C++ Runtime Debug)
- `vcruntime140d.dll` (Visual C++ Runtime Debug)

**解决方法**：
- Debug版本需要安装 Visual Studio
- Release版本需要安装 Visual C++ Redistributable

---

### 步骤5：检查调试输出
**在Visual Studio中运行**：
1. 打开 Visual Studio
2. 按 `F5` 启动调试
3. 查看"输出"窗口的TRACE信息

**预期输出**：
```
[OnInitDialog] 开始初始化DNS模块...
[初始化] 使用默认DNS配置（跳过测速）
[初始化] DNS服务器初始化完成（使用默认配置）
[初始化] 初始化完成
```

**如果卡住不动**：
- 可能在某个函数中阻塞
- 检查是否有网络请求（测速、规则下载）
- 检查是否有死锁

---

### 步骤6：最小化测试
**创建最简单的版本**，注释掉可能有问题的代码：

在`OnInitDialog()`中：
```cpp
BOOL CStupidDNSDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    
    // 只保留基本初始化
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    
    // 注释掉所有DNS初始化
    // InitializeDNSServers();
    // InitializeDomainClassifier();
    
    return TRUE;
}
```

**如果这样能启动**：
- 说明问题在DNS初始化中
- 逐步取消注释，找到问题代码

---

## ?? 常见问题和解决方案

### 问题1：缺少MFC DLL
**症状**：程序无法启动，提示缺少`mfc140ud.dll`

**解决**：
- Debug版本：需要安装Visual Studio
- Release版本：编译Release配置，使用静态链接MFC

**修改项目设置**：
1. 右键项目 → 属性
2. `配置属性` → `常规` → `MFC的使用`
3. 改为：`在静态库中使用MFC`
4. 重新编译

---

### 问题2：访问冲突（Crash on Startup）
**症状**：程序启动立即崩溃

**可能原因**：
- 未初始化的成员变量
- 空指针访问
- 数组越界

**检查代码**：
```cpp
// 在构造函数中确保所有成员都初始化
CStupidDNSDlg::CStupidDNSDlg(CWnd* pParent)
    : CDialogEx(IDD_STUPIDDNS_DIALOG, pParent)
    , m_bServiceRunning(FALSE)  // ← 确保初始化
    , m_nPort(53)
    // ... 所有成员都要初始化
{
}
```

---

### 问题3：初始化阻塞
**症状**：程序启动后一直卡住，无响应

**可能原因**：
- `InitializeDNSServers()` 中的测速操作阻塞UI线程
- 网络请求超时
- 死锁

**解决**：
已在最新代码中**移除了阻塞的测速操作**：
```cpp
// 跳过测速，使用默认配置
TRACE(_T("[初始化] 使用默认DNS配置（跳过测速）\n"));
```

---

### 问题4：规则文件加载失败
**症状**：初始化时抛出异常

**检查文件**：
- `china_domain_list.txt`
- `gfw_domain_list.txt`
- `poisoned_ip_list.txt`

**解决**：
代码已添加异常捕获：
```cpp
try {
    InitializeDomainClassifier();
} catch (...) {
    // 失败后使用默认规则
}
```

---

## ? 推荐调试步骤（从简单到复杂）

### 1. 在Visual Studio中按F5启动
查看是否有错误消息或异常。

### 2. 查看输出窗口
查找TRACE输出，找到卡住的位置。

### 3. 添加断点
在`OnInitDialog()`开头添加断点：
```cpp
BOOL CStupidDNSDlg::OnInitDialog()
{
    __debugbreak();  // ← 添加这行，程序会在这里暂停
    CDialogEx::OnInitDialog();
  // ...
}
```

### 4. 逐步执行
- 按 `F10` 逐行执行
- 观察哪一行导致程序卡住或崩溃

### 5. 查看调用堆栈
如果程序崩溃，查看 `调试` → `窗口` → `调用堆栈`

---

## ?? 收集信息

如果问题仍未解决，请提供以下信息：

1. **事件查看器中的错误**（如果有）
2. **Visual Studio输出窗口的TRACE信息**
3. **程序崩溃时的调用堆栈**
4. **是否缺少DLL文件**
5. **是否以管理员身份运行**

---

## ?? 快速测试版本

如果需要，我可以提供一个**最小化版本**，移除所有复杂初始化，只显示窗口。

这样可以确定问题是在：
- ? MFC框架本身
- ? 对话框资源
- ? DNS初始化代码

---

## ?? 临时解决方案

**修改`OnInitDialog()`，注释掉所有DNS初始化**：

```cpp
BOOL CStupidDNSDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    
    // 暂时禁用DNS初始化
    /*
    InitializeDNSServers();
    InitializeDomainClassifier();
    */
    
    // 显示简单消息
    SetDlgItemText(IDC_STATIC_STATUS, _T("● 已就绪"));
    
    return TRUE;
}
```

如果这样能启动，说明问题确实在DNS初始化中。
