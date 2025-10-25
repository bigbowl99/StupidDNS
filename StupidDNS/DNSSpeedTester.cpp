// DNSSpeedTester.cpp: DNS测速类实现
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "pch.h"
#include "DNSSpeedTester.h"
#include <thread>
#include <future>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

// 构造函数
CDNSSpeedTester::CDNSSpeedTester()
    : m_timeout(2000)
    , m_wsaInitialized(FALSE)
{
    InitializeWinsock();
}

// 析构函数
CDNSSpeedTester::~CDNSSpeedTester()
{
    CleanupWinsock();
}

// 初始化Winsock
BOOL CDNSSpeedTester::InitializeWinsock()
{
    if (m_wsaInitialized)
        return TRUE;
    
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (result != 0) {
        return FALSE;
    }
    
  m_wsaInitialized = TRUE;
    return TRUE;
}

// 清理Winsock
void CDNSSpeedTester::CleanupWinsock()
{
    if (m_wsaInitialized) {
        WSACleanup();
        m_wsaInitialized = FALSE;
    }
}

// 域名编码（转换为DNS协议格式）
// 例如: www.google.com -> 3www6google3com0
int CDNSSpeedTester::EncodeDomainName(const CString& domain, char* buffer)
{
    CStringA domainA(domain);  // 转换为ANSI
    int pos = 0;
    int labelStart = 0;
    
    for (int i = 0; i <= domainA.GetLength(); i++) {
        if (i == domainA.GetLength() || domainA[i] == '.') {
        int labelLen = i - labelStart;
          if (labelLen > 0) {
                buffer[pos++] = (char)labelLen;
   memcpy(buffer + pos, (LPCSTR)domainA + labelStart, labelLen);
    pos += labelLen;
          }
     labelStart = i + 1;
        }
    }
    
    buffer[pos++] = 0;  // 结束标记
    return pos;
}

// 构造DNS查询包
int CDNSSpeedTester::BuildDNSQuery(const CString& domain, char* buffer, int bufferSize)
{
    memset(buffer, 0, bufferSize);
    
    // DNS Header (12 bytes)
    unsigned short transactionID = (unsigned short)GetTickCount();
    buffer[0] = (transactionID >> 8) & 0xFF;
    buffer[1] = transactionID & 0xFF;
    
    // Flags: 标准查询
    buffer[2] = 0x01;  // Recursion Desired
    buffer[3] = 0x00;
  
    // Questions = 1
buffer[4] = 0x00;
    buffer[5] = 0x01;
    
    // Answer RRs = 0
    buffer[6] = 0x00;
    buffer[7] = 0x00;

    // Authority RRs = 0
    buffer[8] = 0x00;
    buffer[9] = 0x00;
    
    // Additional RRs = 0
    buffer[10] = 0x00;
    buffer[11] = 0x00;
  
    // Question Section
    int pos = 12;
    
    // QNAME: 编码域名
    pos += EncodeDomainName(domain, buffer + pos);
    
  // QTYPE: A记录 (0x0001)
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;
    
    // QCLASS: IN (0x0001)
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;
    
    return pos;
}

// 解析DNS响应包
BOOL CDNSSpeedTester::ParseDNSResponse(const char* response, int length)
{
    // 简单验证：检查是否是响应包
    if (length < 12)
        return FALSE;
    
    // 检查Flags中的QR位（应该是1，表示响应）
    if (((unsigned char)response[2] & 0x80) == 0)
      return FALSE;
    
    // 检查RCODE（响应码，0表示无错误）
    int rcode = (unsigned char)response[3] & 0x0F;
  if (rcode != 0)
        return FALSE;
    
    // 检查是否有回答（避免有符号扩展）
    int answerCount = (((unsigned char)response[6]) << 8) | (unsigned char)response[7];
    if (answerCount == 0)
        return FALSE;
    
    return TRUE;
}

// 执行单次DNS查询
int CDNSSpeedTester::DoSingleDNSQuery(SOCKET sock, sockaddr_in& dnsAddr, 
     const char* query, int queryLen)
{
    // 记录开始时间
    DWORD startTime = GetTickCount();
    
    // 发送查询
    int sendResult = sendto(sock, query, queryLen, 0, 
           (sockaddr*)&dnsAddr, sizeof(dnsAddr));
    if (sendResult == SOCKET_ERROR) {
        return -1;
    }
    
    // 接收响应
    char response[512];
    sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);
    
    int recvResult = recvfrom(sock, response, sizeof(response), 0,
          (sockaddr*)&fromAddr, &fromLen);
    
    if (recvResult == SOCKET_ERROR) {
        return -1;  // 超时或错误
    }
    
    // 验证响应
    if (!ParseDNSResponse(response, recvResult)) {
        return -1;
    }
    
    // 计算延迟
    DWORD endTime = GetTickCount();
  int latency = endTime - startTime;
    
    return latency;
}

// 测试DNS查询延迟
int CDNSSpeedTester::TestDNSQuery(const CString& dnsIP, const CString& testDomain)
{
    // 创建UDP socket
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 if (sock == INVALID_SOCKET) {
        return -1;
    }
    
    // 设置接收超时
    DWORD timeout = m_timeout;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    // 设置DNS服务器地址
    sockaddr_in dnsAddr;
    memset(&dnsAddr, 0, sizeof(dnsAddr));
    dnsAddr.sin_family = AF_INET;
    dnsAddr.sin_port = htons(53);
    
    CStringA dnsIPA(dnsIP);
    dnsAddr.sin_addr.s_addr = inet_addr(dnsIPA);
    
    if (dnsAddr.sin_addr.s_addr == INADDR_NONE) {
        closesocket(sock);
     return -1;
    }
    
    // 构造DNS查询包
    char queryBuffer[512];
 int queryLen = BuildDNSQuery(testDomain, queryBuffer, sizeof(queryBuffer));
    
    // 执行查询
    int latency = DoSingleDNSQuery(sock, dnsAddr, queryBuffer, queryLen);
    
    // 关闭socket
  closesocket(sock);
  
    return latency;
}

// 测试单个DNS服务器
SpeedTestResult CDNSSpeedTester::TestSingleDNS(const CString& dnsIP, int testCount)
{
    SpeedTestResult result;
    result.serverIP = dnsIP;
    result.totalCount = testCount;
    
    int totalLatency = 0;
    
  for (int i = 0; i < testCount; i++) {
     int latency = TestDNSQuery(dnsIP);
   
        if (latency > 0) {
            // 成功
    result.successCount++;
            totalLatency += latency;
            
        if (latency < result.minLatency)
       result.minLatency = latency;
            if (latency > result.maxLatency)
             result.maxLatency = latency;
        }
     
        // 间隔100ms避免过于频繁
 if (i < testCount - 1) {
    Sleep(100);
        }
    }
    
    // 计算平均延迟
    if (result.successCount > 0) {
        result.avgLatency = totalLatency / result.successCount;
        result.successRate = (double)result.successCount / testCount * 100.0;
    } else {
        result.avgLatency = -1;
        result.minLatency = -1;
  result.successRate = 0.0;
    }
    
    return result;
}

// 测试多个DNS服务器（并发）
std::vector<SpeedTestResult> CDNSSpeedTester::TestMultipleDNS(
    const std::vector<CString>& dnsIPs, int testCount)
{
    std::vector<SpeedTestResult> results;
    std::vector<std::future<SpeedTestResult>> futures;
    
    int total = dnsIPs.size();
    int current = 0;
    
    // 使用线程池测试（最多10个并发）
    const int maxThreads = 10;
    int activeThreads = 0;
    
    for (size_t i = 0; i < dnsIPs.size(); i++) {
        // 等待直到有空闲线程
 while (activeThreads >= maxThreads) {
            for (auto it = futures.begin(); it != futures.end(); ) {
     if (it->wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) {
          results.push_back(it->get());
         it = futures.erase(it);
    activeThreads--;
          current++;
        
            // 调用进度回调
 if (m_progressCallback) {
             CString status;
              status.Format(_T("正在测试 DNS %d/%d"), current, total);
                m_progressCallback(current, total, status);
         }
      } else {
     ++it;
    }
          }
        }
    
        // 启动新的测试任务
        CString dnsIP = dnsIPs[i];
   futures.push_back(std::async(std::launch::async, [this, dnsIP, testCount]() {
     return TestSingleDNS(dnsIP, testCount);
        }));
        activeThreads++;
    }
 
    // 等待所有任务完成
    for (auto& future : futures) {
        results.push_back(future.get());
     current++;
      
        if (m_progressCallback) {
       CString status;
            status.Format(_T("正在测试 DNS %d/%d"), current, total);
            m_progressCallback(current, total, status);
     }
    }
    
    return results;
}

// ICMP Ping测试（暂不实现，返回-1）
int CDNSSpeedTester::TestICMPPing(const CString& ip)
{
// TODO: 实现ICMP Ping
    // 需要 iphlpapi.lib 和 icmpapi.h
    return -1;
}
