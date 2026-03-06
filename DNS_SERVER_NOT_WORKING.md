# ?? DNS服务器不工作诊断指南

## ?? 症状
- DNS已设置为127.0.0.1（`ipconfig /all`确认）
- 启动DNS服务成功（按钮变为"停止服务"）
- **但是ping网站时没有日志记录**
- **网站ping不通**

---

## ?? 诊断步骤

### 步骤1：检查端口53是否被占用

**在命令提示符（管理员权限）中运行**：
```cmd
netstat -ano | findstr :53
```

**预期结果**：
```
UDP    0.0.0.0:53     *:*     进程ID
UDP    [::]:53   *:*    进程ID
```

**如果看到其他进程占用53端口**：
- 记下进程ID
- 打开任务管理器，查看是什么程序
- 常见占用53端口的程序：
  - `svchost.exe` (Windows DNS客户端服务)
  - 其他DNS代理软件

**解决方案**：
停止DNS客户端服务（临时）：
```cmd
net stop dnscache
```

---

### 步骤2：检查防火墙

**Windows防火墙可能阻止了UDP 53端口**

**添加防火墙规则**：
```cmd
netsh advfirewall firewall add rule name="StupidDNS-UDP53" dir=in action=allow protocol=UDP localport=53
```

---

### 步骤3：使用nslookup测试

**不要用ping，先用nslookup测试DNS服务器**：

```cmd
nslookup www.baidu.com 127.0.0.1
```

**预期结果**：
- 应该在程序日志中看到`www.baidu.com`的查询记录
- nslookup应该返回IP地址

**如果nslookup失败**：
- 检查程序是否真的绑定了53端口
- 查看Output窗口的TRACE日志

---

### 步骤4：检查程序日志（TRACE输出）

**在Visual Studio的"输出"窗口中查找**：

```
DNS服务器启动成功，监听端口 53
DNS服务器线程启动
```

**如果看到**：
```
绑定端口 53 失败，错误: 10048
```
说明**端口被占用**！

**如果看到**：
```
绑定端口 53 失败，错误: 10013
```
说明**没有管理员权限**！

---

### 步骤5：监听所有网络接口

**问题可能是只监听了特定接口**

当前代码：
```cpp
serverAddr.sin_addr.s_addr = INADDR_ANY;  // 0.0.0.0，应该监听所有接口
```

**验证**：使用Wireshark抓包
1. 下载Wireshark
2. 过滤器输入：`udp.port == 53`
3. ping一个网站
4. 查看是否有DNS查询包发送到127.0.0.1:53

---

### 步骤6：检查是否需要以管理员身份运行

**绑定53端口（特权端口）通常需要管理员权限**

1. 右键点击`StupidDNS.exe`
2. 选择"以管理员身份运行"
3. 再次测试

---

## ?? 最可能的原因

### 原因1：端口53被Windows DNS客户端服务占用 ?????

**症状**：
- 程序启动成功（但实际绑定失败被忽略了？）
- 没有错误提示

**解决方案**：
```cmd
net stop dnscache
```

然后重新启动你的程序。

---

### 原因2：防火墙阻止 ????

**解决方案**：
```cmd
netsh advfirewall firewall add rule name="StupidDNS-UDP53" dir=in action=allow protocol=UDP localport=53
```

---

### 原因3：没有管理员权限 ???

**解决方案**：
以管理员身份运行程序

---

## ?? 代码层面的可能问题

### 问题1：非阻塞模式导致recvfrom立即返回

当前代码设置了非阻塞模式：
```cpp
u_long mode = 1;
ioctlsocket(m_socket, FIONBIO, &mode);
```

然后在接收时：
```cpp
int recvLen = recvfrom(pServer->m_socket, buffer, sizeof(buffer), 0,
    (sockaddr*)&clientAddr, &clientAddrLen);

if (recvLen == SOCKET_ERROR) {
    int error = WSAGetLastError();
    if (error != WSAETIMEDOUT && error != WSAEWOULDBLOCK) {
    TRACE(_T("接收错误: %d\n"), error);
        break;  // ← 这里会退出线程！
    }
}
```

**如果非阻塞socket没有数据，会返回`WSAEWOULDBLOCK`**，但代码会继续循环（CPU100%）。

**更严重的是**：如果其他错误，线程会退出！

---

## ? 立即测试方案

### 方案1：检查端口占用（最快）

```cmd
netstat -ano | findstr :53
```

如果被占用，停止DNS缓存服务：
```cmd
net stop dnscache
```

### 方案2：使用nslookup直接测试（最准确）

```cmd
nslookup www.baidu.com 127.0.0.1
```

查看程序日志是否有记录。

### 方案3：检查TRACE日志（最详细）

在Visual Studio的"输出"窗口查找：
- `DNS服务器启动成功`
- `DNS服务器线程启动`
- `绑定端口 53 失败`

---

## ?? 代码修复建议

### 修复1：添加更多TRACE日志

在`ServerThread`中：
```cpp
DWORD WINAPI CDNSServerCore::ServerThread(LPVOID lpParam)
{
    CDNSServerCore* pServer = (CDNSServerCore*)lpParam;
    
    TRACE(_T("[ServerThread] DNS服务器线程启动\n"));
    TRACE(_T("[ServerThread] Socket: %d, Port: %d\n"), pServer->m_socket, pServer->m_nPort);
    
    char buffer[512];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    
    int loopCount = 0;

    while (pServer->m_bRunning) {
loopCount++;
if (loopCount % 100 == 0) {
     TRACE(_T("[ServerThread] 等待DNS查询... (循环 %d 次)\n"), loopCount);
 }
        
        // 接收DNS查询
        int recvLen = recvfrom(pServer->m_socket, buffer, sizeof(buffer), 0,
            (sockaddr*)&clientAddr, &clientAddrLen);
    
        if (recvLen > 0) {
  TRACE(_T("[ServerThread] 收到DNS查询，长度: %d 字节\n"), recvLen);
     // 处理查询
    pServer->HandleDNSQuery(buffer, recvLen, clientAddr);
        }
     else if (recvLen == SOCKET_ERROR) {
   int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
     // 非阻塞模式下没有数据，休眠一下
      Sleep(10);
    }
          else if (error != WSAETIMEDOUT) {
     TRACE(_T("[ServerThread] 接收错误: %d\n"), error);
     break;
      }
        }
    }
    
    TRACE(_T("[ServerThread] DNS服务器线程退出\n"));
    return 0;
}
```

### 修复2：检查绑定是否真的成功

在`Start()`中：
```cpp
if (bind(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
    int error = WSAGetLastError();
    TRACE(_T("[Start] 绑定端口 %d 失败，错误码: %d\n"), m_nPort, error);
    
    CString errorMsg;
    errorMsg.Format(_T("无法绑定端口 %d！错误码: %d\n\n"), m_nPort, error);
    
    if (error == WSAEACCES || error == 10013) {
 errorMsg += _T("原因：需要管理员权限\n");
    } else if (error == WSAEADDRINUSE || error == 10048) {
        errorMsg += _T("原因：端口已被占用\n");
    errorMsg += _T("请运行: net stop dnscache\n");
    }
    
    AfxMessageBox(errorMsg, MB_OK | MB_ICONERROR);
    
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
    WSACleanup();
    return FALSE;
}
```

---

## ?? 请按以下顺序操作

1. **立即检查端口占用**：
   ```cmd
   netstat -ano | findstr :53
   ```

2. **如果被占用，停止DNS缓存**：
   ```cmd
   net stop dnscache
   ```

3. **重新启动程序**

4. **使用nslookup测试**：
   ```cmd
   nslookup www.baidu.com 127.0.0.1
   ```

5. **查看程序日志**，告诉我是否有记录

---

**请先执行步骤1-5，然后告诉我结果！** ??
