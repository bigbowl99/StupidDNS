# StupidDNS DNS服务器测速与选择功能需求

## 模块概述
实现DNS服务器的自动测速、评估和选择功能。软件启动时自动完成测速，选出最优的国内和海外DNS服务器。

---

## 1. 核心数据结构

### 1.1 DNS服务器信息结构
```cpp
// DNSServer.h
struct DNSServerInfo {
    CString name;           // 服务器名称，如 "阿里DNS"
    CString ip;             // IP地址，如 "223.5.5.5"
    CString location;       // 地理位置，如 "中国", "美国", "欧洲"
    CString provider;       // 提供商，如 "Alibaba", "Cloudflare"
    
    int latency;            // 延迟(毫秒)，-1表示未测试或失败
    double reliability;     // 可靠性(0-100%)
    BOOL isActive;          // 是否启用
    BOOL isOnline;          // 是否在线
    
    DWORD lastTestTime;     // 最后测试时间
    int failCount;          // 连续失败次数
    
    DNSServerInfo() {
        latency = -1;
        reliability = 0.0;
        isActive = FALSE;
        isOnline = FALSE;
        lastTestTime = 0;
        failCount = 0;
    }
};
```

### 1.2 测速结果结构
```cpp
struct SpeedTestResult {
    CString serverIP;
    int avgLatency;         // 平均延迟
    int minLatency;         // 最小延迟
    int maxLatency;         // 最大延迟
    int successCount;       // 成功次数
    int totalCount;         // 总测试次数
    double successRate;     // 成功率
    
    SpeedTestResult() {
        avgLatency = -1;
        minLatency = INT_MAX;
        maxLatency = 0;
        successCount = 0;
        totalCount = 0;
        successRate = 0.0;
    }
};
```

---

## 2. DNS服务器测速类

### 2.1 类定义
```cpp
// DNSSpeedTester.h
class CDNSSpeedTester {
public:
    CDNSSpeedTester();
    ~CDNSSpeedTester();
    
    // 测试单个DNS服务器
    SpeedTestResult TestSingleDNS(const CString& dnsIP, 
                                   int testCount = 3);
    
    // 测试多个DNS服务器（并发）
    std::vector<SpeedTestResult> TestMultipleDNS(
        const std::vector<CString>& dnsIPs, 
        int testCount = 3);
    
    // 测试DNS查询延迟
    int TestDNSQuery(const CString& dnsIP, 
                     const CString& testDomain = _T("www.google.com"));
    
    // ICMP Ping测试
    int TestICMPPing(const CString& ip);
    
    // 设置测试超时时间
    void SetTimeout(int timeoutMS) { m_timeout = timeoutMS; }
    
    // 获取测试进度回调
    void SetProgressCallback(
        std::function<void(int current, int total, CString status)> callback) {
        m_progressCallback = callback;
    }
    
private:
    int m_timeout;          // 超时时间(毫秒)，默认2000
    std::function<void(int, int, CString)> m_progressCallback;
    
    // 构造DNS查询包
    int BuildDNSQuery(const CString& domain, char* buffer, int bufferSize);
    
    // 解析DNS响应包
    BOOL ParseDNSResponse(const char* response, int length);
    
    // 执行单次DNS查询
    int DoSingleDNSQuery(SOCKET sock, sockaddr_in& dnsAddr, 
                         const char* query, int queryLen);
};
```

### 2.2 实现要点

#### TestDNSQuery 实现
```
功能：测试DNS查询延迟
流程：
1. 创建UDP socket
2. 设置超时 SO_RCVTIMEO = m_timeout
3. 构造DNS查询包（查询 www.google.com 的 A 记录）
4. 记录开始时间 startTime = GetTickCount()
5. 发送查询包到 dnsIP:53
6. 等待响应
7. 计算延迟 latency = GetTickCount() - startTime
8. 如果超时或失败，返回 -1
9. 关闭socket
10. 返回延迟值

DNS查询包格式（标准DNS协议）：
- Header: 12字节
  * Transaction ID: 2字节
  * Flags: 0x0100 (标准查询)
  * Questions: 1
  * Answer/Authority/Additional: 0
- Question:
  * QNAME: 域名编码（长度+标签格式）
  * QTYPE: 0x0001 (A记录)
  * QCLASS: 0x0001 (IN)
```

#### TestSingleDNS 实现
```
功能：测试单个DNS服务器，执行多次查询取平均值
参数：dnsIP - DNS服务器IP，testCount - 测试次数（默认3）
流程：
1. 初始化 SpeedTestResult 结果对象
2. 循环 testCount 次：
   a. 调用 TestDNSQuery(dnsIP)
   b. 如果成功（返回值 > 0）：
      - successCount++
      - 累加延迟
      - 更新 minLatency 和 maxLatency
   c. 如果失败：记录失败
   d. 间隔100ms避免过于频繁
3. 计算平均延迟 avgLatency = 总延迟 / successCount
4. 计算成功率 successRate = (successCount / testCount) * 100
5. 返回结果对象
```

#### TestMultipleDNS 实现
```
功能：并发测试多个DNS服务器
流程：
1. 创建线程池（最多10个线程）
2. 为每个DNS服务器启动一个测试任务
3. 使用 std::future 收集结果
4. 每完成一个，调用 progressCallback 更新进度
5. 等待所有任务完成
6. 返回所有结果的 vector
```

---

## 3. DNS服务器管理类

### 3.1 类定义
```cpp
// DNSServerManager.h
class CDNSServerManager {
public:
    CDNSServerManager();
    ~CDNSServerManager();
    
    // 初始化（软件启动时调用）
    BOOL Initialize();
    
    // 获取国内最快的DNS
    DNSServerInfo GetFastestDomesticDNS();
    
    // 获取海外最快的N个DNS
    std::vector<DNSServerInfo> GetFastestOverseasDNS(int count = 4);
    
    // 执行完整的测速流程
    BOOL PerformSpeedTest(
        std::function<void(CString)> progressCallback = nullptr);
    
    // 保存/加载配置
    BOOL SaveConfig();
    BOOL LoadConfig();
    
    // 获取所有DNS列表
    const std::vector<DNSServerInfo>& GetDomesticServers() const {
        return m_domesticServers;
    }
    const std::vector<DNSServerInfo>& GetOverseasServers() const {
        return m_overseasServers;
    }
    
private:
    std::vector<DNSServerInfo> m_domesticServers;   // 国内DNS列表
    std::vector<DNSServerInfo> m_overseasServers;   // 海外DNS列表
    
    DNSServerInfo m_activeDomesticDNS;              // 当前使用的国内DNS
    std::vector<DNSServerInfo> m_activeOverseasDNS; // 当前使用的海外DNS
    
    CDNSSpeedTester m_tester;
    
    // 初始化DNS服务器列表
    void InitDomesticDNSList();
    void InitOverseasDNSList();
    
    // 测速并选择最优服务器
    BOOL TestAndSelectDomesticDNS();
    BOOL TestAndSelectOverseasDNS();
    
    // 排序函数
    static BOOL CompareDNSByLatency(const DNSServerInfo& a, 
                                     const DNSServerInfo& b) {
        if (a.latency == -1) return false;
        if (b.latency == -1) return true;
        return a.latency < b.latency;
    }
};
```

### 3.2 实现要点

#### InitDomesticDNSList 实现
```
功能：初始化国内DNS服务器列表
硬编码3个国内DNS：
1. 阿里DNS: 223.5.5.5 (备用: 223.6.6.6)
2. 腾讯DNS: 119.29.29.29 (备用: 182.254.116.116)
3. 114DNS: 114.114.114.114 (备用: 114.114.115.115)

代码结构：
m_domesticServers.clear();

DNSServerInfo ali;
ali.name = _T("阿里DNS");
ali.ip = _T("223.5.5.5");
ali.location = _T("中国");
ali.provider = _T("Alibaba");
ali.reliability = 99.9;
m_domesticServers.push_back(ali);

// 依此类推添加其他国内DNS
```

#### InitOverseasDNSList 实现
```
功能：初始化海外DNS候选服务器列表
要求：预定义可靠性>80%的DNS服务器

美国DNS候选（至少8个，选最快4个）：
1. Cloudflare: 1.1.1.1, 1.0.0.1
2. Google: 8.8.8.8, 8.8.4.4
3. Quad9: 9.9.9.9, 149.112.112.112
4. OpenDNS: 208.67.222.222, 208.67.220.220
5. Level3: 4.2.2.1, 4.2.2.2
6. Verisign: 64.6.64.6, 64.6.65.6

欧洲DNS候选（至少4个，选最快2个）：
1. DNS.Watch (德国): 84.200.69.80, 84.200.70.40
2. FreeDNS (奥地利): 37.235.1.174, 37.235.1.177
3. OpenNIC (欧洲): 185.121.177.177, 169.239.202.202

亚洲DNS候选（不包括中国，至少4个，选最快2个）：
1. 日本 OpenDNS: 210.141.113.7
2. 韩国 KT: 168.126.63.1, 168.126.63.2
3. 新加坡 SingNet: 165.21.83.88, 165.21.100.88
4. 台湾 HiNet: 168.95.1.1, 168.95.192.1

代码结构：
DNSServerInfo cloudflare;
cloudflare.name = _T("Cloudflare");
cloudflare.ip = _T("1.1.1.1");
cloudflare.location = _T("美国");
cloudflare.provider = _T("Cloudflare");
cloudflare.reliability = 99.9;
m_overseasServers.push_back(cloudflare);

// 依此类推添加其他海外DNS
```

#### TestAndSelectDomesticDNS 实现
```
功能：测试国内DNS并选择最快的一个
流程：
1. 如果已有缓存且未过期（<24小时），直接使用缓存
2. 提取所有国内DNS的IP列表
3. 调用 m_tester.TestMultipleDNS(ipList, 3)
4. 更新每个DNS的 latency 和 isOnline 状态
5. 按延迟排序（使用 CompareDNSByLatency）
6. 选择延迟最小的作为 m_activeDomesticDNS
7. 记录日志：选中了哪个DNS，延迟多少ms
8. 返回 TRUE

特殊处理：
- 如果所有DNS都失败（延迟 == -1），返回 FALSE
- 如果最快的DNS延迟 > 500ms，记录警告日志
```

#### TestAndSelectOverseasDNS 实现
```
功能：测试海外DNS并选择最快的4个
流程：
1. 如果已有缓存且未过期（<24小时），直接使用缓存
2. 提取所有海外DNS的IP列表
3. 调用 m_tester.TestMultipleDNS(ipList, 3)
4. 更新每个DNS的 latency 和 isOnline 状态
5. 过滤掉失败的DNS（latency == -1）
6. 按延迟排序
7. 选择前4个作为 m_activeOverseasDNS
8. 确保地理分布：
   - 至少包含1个美国DNS
   - 至少包含1个亚洲DNS（非中国）
   - 如果可能，包含1个欧洲DNS
9. 记录日志：列出选中的4个DNS及延迟
10. 返回 TRUE

地理分布逻辑：
- 先按延迟排序取前10名
- 从中选择：美国最快2个 + 亚洲最快1个 + 欧洲最快1个
- 如果某地区不足，用其他地区补充
```

#### PerformSpeedTest 实现
```
功能：执行完整的测速流程（启动时调用）
参数：progressCallback - 进度回调函数
流程：
1. 调用 InitDomesticDNSList()
2. 调用 InitOverseasDNSList()
3. 更新进度："正在测试国内DNS服务器..."
4. 调用 TestAndSelectDomesticDNS()
5. 更新进度："正在测试海外DNS服务器..."
6. 调用 TestAndSelectOverseasDNS()
7. 调用 SaveConfig() 保存结果
8. 更新进度："测速完成"
9. 返回 TRUE

进度回调格式：
progressCallback(_T("正在测试 Cloudflare (1/20)"));
progressCallback(_T("正在测试 Google (2/20)"));
...
```

---

## 4. 配置文件格式

### 4.1 配置文件路径
```
程序目录\dns_config.ini
```

### 4.2 INI文件格式
```ini
[DomesticDNS]
ActiveDNS=223.5.5.5
Name=阿里DNS
Latency=23
LastTest=1706083200

[OverseasDNS]
Count=4
DNS1=1.1.1.1
DNS1Name=Cloudflare
DNS1Latency=45
DNS2=8.8.8.8
DNS2Name=Google
DNS2Latency=52
DNS3=168.95.1.1
DNS3Name=台湾HiNet
DNS3Latency=38
DNS4=165.21.83.88
DNS4Name=新加坡SingNet
DNS4Latency=41

[Settings]
LastFullTest=1706083200
AutoTestInterval=86400
```

### 4.3 SaveConfig 实现
```
使用 CStdioFile 或 WritePrivateProfileString 写入INI文件
保存：
- 当前使用的国内DNS信息
- 当前使用的4个海外DNS信息
- 最后测速时间
```

### 4.4 LoadConfig 实现
```
使用 CStdioFile 或 GetPrivateProfileString 读取INI文件
加载：
- 恢复上次选择的DNS服务器
- 检查是否过期（>24小时）
- 如果过期，触发重新测速
```

---

## 5. 主对话框集成

### 5.1 在 CStupidDNSDlg 中添加
```cpp
// StupidDNSDlg.h
class CStupidDNSDlg : public CDialogEx {
private:
    CDNSServerManager m_dnsManager;
    
    // 启动时初始化
    void InitializeDNSServers();
    
    // 显示测速进度
    void ShowSpeedTestProgress(CString status);
};
```

### 5.2 OnInitDialog 中调用
```cpp
BOOL CStupidDNSDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    
    // ... 其他初始化代码 ...
    
    // 初始化DNS服务器（可能需要几秒钟）
    InitializeDNSServers();
    
    return TRUE;
}

void CStupidDNSDlg::InitializeDNSServers() {
    // 加载配置
    if (!m_dnsManager.LoadConfig()) {
        // 首次运行或配置过期，执行测速
        AfxMessageBox(_T("首次运行，正在测试DNS服务器速度，请稍候..."));
        
        // 执行测速（带进度回调）
        m_dnsManager.PerformSpeedTest([this](CString status) {
            // 更新状态栏或进度条
            ShowSpeedTestProgress(status);
        });
    }
    
    // 在日志中显示当前使用的DNS
    DNSServerInfo domesticDNS = m_dnsManager.GetFastestDomesticDNS();
    CString msg;
    msg.Format(_T("国内DNS: %s (%s) - %dms"), 
        domesticDNS.name, domesticDNS.ip, domesticDNS.latency);
    AddLogEntry(_T("系统"), msg, _T("INFO"), _T("-"), _T("-"));
    
    auto overseasDNS = m_dnsManager.GetFastestOverseasDNS();
    for (auto& dns : overseasDNS) {
        msg.Format(_T("海外DNS: %s (%s) - %dms"), 
            dns.name, dns.ip, dns.latency);
        AddLogEntry(_T("系统"), msg, _T("INFO"), _T("-"), _T("-"));
    }
}
```

---

## 6. 测试用例

### 6.1 单元测试
```cpp
void TestDNSSpeedTester() {
    CDNSSpeedTester tester;
    
    // 测试单个DNS
    int latency = tester.TestDNSQuery(_T("8.8.8.8"));
    ASSERT(latency > 0 && latency < 5000);
    
    // 测试多个DNS
    std::vector<CString> dnsIPs = {
        _T("223.5.5.5"),
        _T("8.8.8.8"),
        _T("1.1.1.1")
    };
    auto results = tester.TestMultipleDNS(dnsIPs);
    ASSERT(results.size() == 3);
}
```

### 6.2 集成测试
```
启动软件 → 检查是否自动完成DNS测速 → 
查看日志显示选中的DNS → 验证配置文件生成
```

---

## 7. 性能要求

- 单个DNS测试时间：< 2秒（包括超时）
- 国内DNS测速总时间：< 10秒
- 海外DNS测速总时间：< 30秒
- 并发测试：最多10个线程
- 内存占用：测速模块 < 5MB

---

## 8. 错误处理

- 所有网络操作必须有超时保护
- DNS测试失败不应导致程序崩溃
- 如果所有DNS都不可用，显示错误提示
- 测速过程可被用户中断

---

## 9. 文件清单

需要创建/修改的文件：
1. DNSServer.h - 数据结构定义
2. DNSSpeedTester.h / DNSSpeedTester.cpp - 测速类
3. DNSServerManager.h / DNSServerManager.cpp - 管理类
4. 修改 StupidDNSDlg.h / StupidDNSDlg.cpp - 集成到主界面

---

## 10. 代码规范

- 使用 Unicode 字符集
- 所有字符串使用 _T() 宏
- 添加详细的中文注释
- 网络操作使用 Winsock2
- 线程安全：使用 CRITICAL_SECTION 保护共享数据
- 错误处理：使用 try-catch 或返回值检查
- 日志记录：关键操作记录到日志

---

## 11. 依赖库
```cpp
// 需要包含的头文件
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <future>
#include <functional>

// 需要链接的库
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
```

---

请根据此需求文档生成完整的代码实现。
```

---

把这个文档发给 Copilot：
```
@workspace 请根据 DNS_SERVER_TESTING_REQUIREMENTS.md 生成完整的DNS服务器测速模块代码，包括：
1. 所有头文件和实现文件
2. 完整的类实现
3. 在主对话框中的集成代码
4. 必要的工具函数

要求代码可以直接编译运行。