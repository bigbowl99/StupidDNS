// DNSDualQuery.cpp - DNS双向查询实现
//

#include "pch.h"
#include "DNSDualQuery.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

// 线程参数结构
struct QueryThreadParam {
    CString domain;
    std::vector<CString> dnsServers;
  DNSQueryResult* pResult;
    CDNSDualQuery* pQuery;
    int timeout;
};

CDNSDualQuery::CDNSDualQuery()
    : m_queryTimeout(2000)  // 默认2秒超时
{
}

CDNSDualQuery::~CDNSDualQuery()
{
}

// 初始化（加载污染IP库）
BOOL CDNSDualQuery::Initialize()
{
    // 获取程序目录
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    CString appPath = szPath;
    int pos = appPath.ReverseFind(_T('\\'));
    appPath = appPath.Left(pos);
  
    // 加载污染IP库
    CString pollutedIPFile = appPath + _T("\\rules\\bogus-nxdomain.china.conf");
    return m_pollutedIPLoader.LoadFromFile(pollutedIPFile);
}

// 执行双向查询（核心功能）
BOOL CDNSDualQuery::PerformDualQuery(
    const CString& domain,
    const std::vector<CString>& domesticDNS,
    const std::vector<CString>& overseasDNS,
  DNSQueryResult& outDomesticResult,
    DNSQueryResult& outOverseasResult
)
{
    // 初始化结果
    outDomesticResult.success = FALSE;
    outOverseasResult.success = FALSE;
    
    // 创建线程参数
    QueryThreadParam domesticParam;
    domesticParam.domain = domain;
    domesticParam.dnsServers = domesticDNS;
    domesticParam.pResult = &outDomesticResult;
    domesticParam.pQuery = this;
    domesticParam.timeout = m_queryTimeout;

    QueryThreadParam overseasParam;
    overseasParam.domain = domain;
    overseasParam.dnsServers = overseasDNS;
    overseasParam.pResult = &outOverseasResult;
    overseasParam.pQuery = this;
    overseasParam.timeout = m_queryTimeout;
    
    // 启动两个查询线程
    HANDLE hDomesticThread = (HANDLE)_beginthreadex(
        NULL, 0,
        [](void* param) -> unsigned {
            QueryThreadParam* p = (QueryThreadParam*)param;
          p->pQuery->QueryMultipleDNS(p->domain, p->dnsServers, *p->pResult);
          return 0;
  },
        &domesticParam, 0, NULL
    );
    
    HANDLE hOverseasThread = (HANDLE)_beginthreadex(
        NULL, 0,
  [](void* param) -> unsigned {
    QueryThreadParam* p = (QueryThreadParam*)param;
            p->pQuery->QueryMultipleDNS(p->domain, p->dnsServers, *p->pResult);
     return 0;
        },
    &overseasParam, 0, NULL
 );
    
    // 等待两个查询完成（最多等待5秒）
    HANDLE handles[] = { hDomesticThread, hOverseasThread };
    WaitForMultipleObjects(2, handles, TRUE, 5000);
    
    CloseHandle(hDomesticThread);
    CloseHandle(hOverseasThread);
    
    return (outDomesticResult.success || outOverseasResult.success);
}

// 污染检测
PollutionStatus CDNSDualQuery::DetectPollution(
    const DNSQueryResult& domesticResult,
    const DNSQueryResult& overseasResult
)
{
    // 1. 检查国内DNS是否返回污染IP
    if (domesticResult.success) {
        if (m_pollutedIPLoader.IsPollutedIP(domesticResult.ip)) {
       return POLLUTION_DETECTED;
      }
    }
  
    // 2. 比较国内外结果
    if (domesticResult.success && overseasResult.success) {
if (domesticResult.ip == overseasResult.ip) {
    // 结果一致，认为是干净的
        return POLLUTION_CLEAN;
} else {
  // 结果不一致，再次检查国内IP
            if (m_pollutedIPLoader.IsPollutedIP(domesticResult.ip)) {
       return POLLUTION_DETECTED;
       }
   // 结果不同但都不在污染库中，不确定
   return POLLUTION_UNCERTAIN;
        }
 }
    
    // 3. 只有一方成功
    if (domesticResult.success && !overseasResult.success) {
        // 只有国内成功，检查是否污染
        return m_pollutedIPLoader.IsPollutedIP(domesticResult.ip) 
            ? POLLUTION_DETECTED : POLLUTION_CLEAN;
    }
    
 if (!domesticResult.success && overseasResult.success) {
     // 只有海外成功，认为是干净的
        return POLLUTION_CLEAN;
    }
    
 return POLLUTION_UNCERTAIN;
}

// 选择最佳结果
DNSQueryResult CDNSDualQuery::SelectBestResult(
  const DNSQueryResult& domesticResult,
  const DNSQueryResult& overseasResult,
    PollutionStatus pollution
)
{
    // 如果检测到污染，直接使用海外结果
  if (pollution == POLLUTION_DETECTED) {
        return overseasResult;
    }
    
    // 如果干净，比较延迟，选择更快的
    if (pollution == POLLUTION_CLEAN) {
        if (domesticResult.success && overseasResult.success) {
     return (domesticResult.latency < overseasResult.latency) 
    ? domesticResult : overseasResult;
    }
        if (domesticResult.success) return domesticResult;
        if (overseasResult.success) return overseasResult;
    }
    
    // 不确定的情况，优先使用海外结果（更保险）
    if (overseasResult.success) return overseasResult;
    if (domesticResult.success) return domesticResult;
    
    // 都失败了，返回空结果
    DNSQueryResult emptyResult;
    return emptyResult;
}

// 并发查询多个DNS（选择最快的）
BOOL CDNSDualQuery::QueryMultipleDNS(
    const CString& domain,
    const std::vector<CString>& dnsServers,
    DNSQueryResult& result
)
{
    if (dnsServers.empty()) {
    return FALSE;
    }
    
    // 简化实现：顺序查询第一个可用的DNS
    // TODO: 后续可优化为真正的并发查询
    for (const auto& dnsServer : dnsServers) {
    if (QueryDNSServer(domain, dnsServer, result)) {
            return TRUE;
        }
  }
    
    return FALSE;
}

// 查询单个DNS服务器
BOOL CDNSDualQuery::QueryDNSServer(
  const CString& domain,
    const CString& dnsServer,
    DNSQueryResult& result
)
{
    // 初始化Winsock
WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return FALSE;
    }
    
    // 创建UDP socket
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
    WSACleanup();
        return FALSE;
    }
    
    // 设置超时
    int timeout = m_queryTimeout;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    // 设置DNS服务器地址
    sockaddr_in dnsAddr;
    dnsAddr.sin_family = AF_INET;
    dnsAddr.sin_port = htons(53);
    dnsAddr.sin_addr.s_addr = inet_addr(CT2A(dnsServer));
    
    // 构建DNS查询包
    char queryBuffer[512];
    int queryLen = BuildDNSQuery(domain, queryBuffer, sizeof(queryBuffer));
    if (queryLen <= 0) {
        closesocket(sock);
        WSACleanup();
  return FALSE;
    }
    
    // 发送查询
    DWORD startTime = GetTickCount();
    int sent = sendto(sock, queryBuffer, queryLen, 0, 
      (sockaddr*)&dnsAddr, sizeof(dnsAddr));
    if (sent == SOCKET_ERROR) {
     closesocket(sock);
        WSACleanup();
      return FALSE;
    }
    
    // 接收响应
    char responseBuffer[512];
    int recvLen = recvfrom(sock, responseBuffer, sizeof(responseBuffer), 0, NULL, NULL);
    DWORD endTime = GetTickCount();
    
    closesocket(sock);
    WSACleanup();
    
 if (recvLen <= 0) {
        return FALSE;
    }
    
    // 解析响应
    CString ip = ParseDNSResponse(responseBuffer, recvLen);
    if (ip.IsEmpty()) {
 return FALSE;
    }
    
    // 填充结果
    result.ip = ip;
    result.latency = (int)(endTime - startTime);
    result.success = TRUE;
    result.dnsServer = dnsServer;
    result.timestamp = time(NULL);
    
    return TRUE;
}

// 构建DNS查询包
int CDNSDualQuery::BuildDNSQuery(const CString& domain, char* buffer, int bufferSize)
{
    if (bufferSize < 512) {
        return -1;
    }
    
    memset(buffer, 0, bufferSize);
    
    // DNS Header
    buffer[0] = 0x12;  // Transaction ID (high byte)
    buffer[1] = 0x34;  // Transaction ID (low byte)
  buffer[2] = 0x01;  // Flags (standard query)
    buffer[3] = 0x00;  // Flags
    buffer[4] = 0x00;  // Questions (high byte)
    buffer[5] = 0x01;  // Questions (low byte) - 1 question
    buffer[6] = 0x00;  // Answer RRs
    buffer[7] = 0x00;
  buffer[8] = 0x00;  // Authority RRs
    buffer[9] = 0x00;
    buffer[10] = 0x00; // Additional RRs
buffer[11] = 0x00;
    
    // 编码域名
    int pos = 12;
    int encodedLen = EncodeDomainName(domain, buffer + pos);
    if (encodedLen <= 0) {
     return -1;
    }
pos += encodedLen;
    
    // Query Type (A record)
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;
    
    // Query Class (IN)
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;
    
    return pos;
}

// 编码域名（转换为DNS协议格式）
int CDNSDualQuery::EncodeDomainName(const CString& domain, char* buffer)
{
    CString domainCopy = domain;
    int pos = 0;
    
    int start = 0;
    int dotPos = 0;
    
    while ((dotPos = domainCopy.Find(_T('.'), start)) != -1) {
        int len = dotPos - start;
   if (len > 63 || len <= 0) {
            return -1;
        }
        
 buffer[pos++] = (char)len;
     for (int i = 0; i < len; i++) {
   buffer[pos++] = (char)domainCopy[start + i];
        }
        
        start = dotPos + 1;
    }
    
  // 最后一段
    int len = domainCopy.GetLength() - start;
    if (len > 0 && len <= 63) {
     buffer[pos++] = (char)len;
        for (int i = 0; i < len; i++) {
      buffer[pos++] = (char)domainCopy[start + i];
     }
    }
 
    buffer[pos++] = 0x00;  // 结束标记
  
    return pos;
}

// 解析DNS响应包
CString CDNSDualQuery::ParseDNSResponse(const char* response, int length)
{
    if (length < 12) {
        return _T("");
    }
    
    // 跳过Header（12字节）
    int pos = 12;
  
    // 跳过Question部分
    while (pos < length && response[pos] != 0) {
        int len = (unsigned char)response[pos];
   pos += len + 1;
    }
    pos++; // 跳过结束的0
    pos += 4; // 跳过Type和Class
    
    // 解析Answer部分
    if (pos + 12 > length) {
      return _T("");
    }
    
    // 跳过Name（压缩指针）
    if ((unsigned char)response[pos] == 0xC0) {
        pos += 2;
    }
  
  // 检查Type（应该是A record = 0x0001）
    if (response[pos] != 0x00 || response[pos + 1] != 0x01) {
        return _T("");
    }
pos += 2;
    
    // 跳过Class
    pos += 2;
    
    // 跳过TTL
    pos += 4;
    
    // 读取Data Length
    int dataLen = ((unsigned char)response[pos] << 8) | (unsigned char)response[pos + 1];
    pos += 2;

    // 读取IP地址（4字节）
    if (dataLen == 4 && pos + 4 <= length) {
        CString ip;
      ip.Format(_T("%d.%d.%d.%d"),
            (unsigned char)response[pos],
            (unsigned char)response[pos + 1],
  (unsigned char)response[pos + 2],
            (unsigned char)response[pos + 3]);
        return ip;
    }
    
    return _T("");
}
