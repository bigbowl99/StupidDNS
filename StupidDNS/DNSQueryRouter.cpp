// DNSQueryRouter.cpp: DNS查询路由器实现
//

#include "pch.h"
#include "DNSQueryRouter.h"
#include <future>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// 构造函数
CDNSQueryRouter::CDNSQueryRouter()
 : m_pServerManager(nullptr)
 , m_pClassifier(nullptr)
{
}

// 析构函数
CDNSQueryRouter::~CDNSQueryRouter()
{
}

// 初始化
BOOL CDNSQueryRouter::Initialize(CDNSServerManager* pServerManager,
 CDomainClassifier* pClassifier)
{
 m_pServerManager = pServerManager;
 m_pClassifier = pClassifier;
 
 if (!m_pServerManager || !m_pClassifier) {
 return FALSE;
 }
 
 return TRUE;
}

// 检查是否是内网IP
BOOL CDNSQueryRouter::IsPrivateIP(const CString& ip)
{
 //10.x.x.x
 if (ip.Left(3) == _T("10.")) {
 return TRUE;
 }
 
 //192.168.x.x
 if (ip.Left(8) == _T("192.168.")) {
 return TRUE;
 }
 
 //172.16.x.x -172.31.x.x
 if (ip.Left(4) == _T("172.")) {
 int secondOctet = _ttoi(ip.Mid(4));
 if (secondOctet >=16 && secondOctet <=31) {
 return TRUE;
 }
 }
 
 return FALSE;
}

// 验证DNS查询结果是否被污染
BOOL CDNSQueryRouter::ValidateResult(const CString& ip)
{
 //1. 检查是否在污染IP黑名单中
 if (m_pClassifier->IsPoisonedIP(ip)) {
 TRACE(_T("[污染检测] IP %s 在黑名单中\n"), ip);
 return FALSE;
 }
 
 //2. 检查是否是保留IP（通常是污染）
 if (IsPrivateIP(ip)) {
 TRACE(_T("[污染检测] IP %s 是内网地址\n"), ip);
 return FALSE;
 }
 
 //3. 检查是否是0.0.0.0或255.255.255.255
 if (ip == _T("0.0.0.0") || ip == _T("255.255.255.255")) {
 TRACE(_T("[污染检测] IP %s 是无效地址\n"), ip);
 return FALSE;
 }
 
 return TRUE;
}

// 从DNS响应包中提取IP地址
CString CDNSQueryRouter::ExtractIPFromResponse(const char* response, int length)
{
 // 简化版本：假设响应包格式正确
 // DNS响应包结构：
 // Header(12字节) + Question(可变) + Answer(可变)
 
 if (length <12) {
 return _T("");
 }
 
 // 跳过Header（12字节）
int pos =12;
 
 // 跳过Question部分
 // QNAME: 域名编码（以0结尾）
 while (pos < length && response[pos] !=0) {
 int labelLen = (unsigned char)response[pos];
 pos += labelLen +1;
 }
 pos++; // 跳过结尾的0
 
 // 跳过QTYPE(2字节) + QCLASS(2字节)
 pos +=4;
 
 if (pos >= length) {
 return _T("");
 }
 
 // Answer部分
 // 遍历所有Answer记录，找到第一个A记录
 // NAME:可能是指针（2字节，以0xC0开头）或完整域名
 //这里只处理最常见的指针形式
 while (pos +10 < length) {
 // NAME
 if ((unsigned char)response[pos] ==0xC0 && pos +1 < length) {
 pos +=2; // 指针
 } else {
 // 跳过完整域名（健壮性处理）
 while (pos < length && response[pos] !=0) {
 int labelLen = (unsigned char)response[pos];
 pos += labelLen +1;
 }
 pos++; // 跳过结尾0
 }

 if (pos +10 > length) return _T("");

 // TYPE(2) + CLASS(2) + TTL(4) + RDLENGTH(2)
 unsigned short type = ((unsigned char)response[pos] <<8) | (unsigned char)response[pos +1];
 pos +=2; // TYPE
 pos +=2; // CLASS
 pos +=4; // TTL
 unsigned short rdlength = ((unsigned char)response[pos] <<8) | (unsigned char)response[pos +1];
 pos +=2; // RDLENGTH

 if (pos + rdlength > length) return _T("");

 if (type ==0x0001 && rdlength ==4) { // A记录
 unsigned char ip1 = (unsigned char)response[pos];
 unsigned char ip2 = (unsigned char)response[pos +1];
 unsigned char ip3 = (unsigned char)response[pos +2];
 unsigned char ip4 = (unsigned char)response[pos +3];

 CString ip;
 ip.Format(_T("%d.%d.%d.%d"), ip1, ip2, ip3, ip4);
 return ip;
 }

 // 跳到下一个RR
 pos += rdlength;
 }
 
 return _T("");
}

// 实际执行DNS查询
CString CDNSQueryRouter::DoQuery(const CString& dnsServer, const CString& domain, int& latency)
{
 latency = -1;

 // 创建UDP socket
 SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 if (sock == INVALID_SOCKET) {
 return _T("");
 }

 // 设置超时
 DWORD timeout =2000;
 setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

 // 设置DNS服务器地址
 sockaddr_in dnsAddr;
 memset(&dnsAddr,0, sizeof(dnsAddr));
 dnsAddr.sin_family = AF_INET;
 dnsAddr.sin_port = htons(53);

 CStringA dnsServerA(dnsServer);
 dnsAddr.sin_addr.s_addr = inet_addr(dnsServerA);

 if (dnsAddr.sin_addr.s_addr == INADDR_NONE) {
 closesocket(sock);
 return _T("");
 }

 // 构造DNS查询包（与 CDNSSpeedTester 保持一致的最简实现）
 char queryBuffer[512];
 memset(queryBuffer,0, sizeof(queryBuffer));

 // Header
 unsigned short transactionID = (unsigned short)GetTickCount();
 queryBuffer[0] = (transactionID >>8) &0xFF;
 queryBuffer[1] = transactionID &0xFF;
 queryBuffer[2] =0x01; // RD
 queryBuffer[3] =0x00;
 queryBuffer[4] =0x00; queryBuffer[5] =0x01; // QDCOUNT =1
 // ANCOUNT/NS/AR 默认为0

 // QNAME 编码
 auto encodeDomain = [](const CString& dom, char* buf) -> int {
 CStringA d(dom);
 int pos =0;
 int labelStart =0;
 for (int i =0; i <= d.GetLength(); i++) {
 if (i == d.GetLength() || d[i] == '.') {
 int labelLen = i - labelStart;
 if (labelLen >0) {
 buf[pos++] = (char)labelLen;
 memcpy(buf + pos, (LPCSTR)d + labelStart, labelLen);
 pos += labelLen;
 }
 labelStart = i +1;
 }
 }
 buf[pos++] =0; //结束
 return pos;
 };

 int pos =12;
 pos += encodeDomain(domain, queryBuffer + pos);
 // QTYPE = A
 queryBuffer[pos++] =0x00; queryBuffer[pos++] =0x01;
 // QCLASS = IN
 queryBuffer[pos++] =0x00; queryBuffer[pos++] =0x01;

 int queryLen = pos;

 //记录开始时间
 DWORD startTime = GetTickCount();

 //发送查询
 int sendResult = sendto(sock, queryBuffer, queryLen,0, (sockaddr*)&dnsAddr, sizeof(dnsAddr));
 if (sendResult == SOCKET_ERROR) {
 closesocket(sock);
 return _T("");
 }

 // 接收响应
 char response[512];
 sockaddr_in fromAddr;
 int fromLen = sizeof(fromAddr);

 int recvResult = recvfrom(sock, response, sizeof(response),0, (sockaddr*)&fromAddr, &fromLen);
 if (recvResult <=0) {
 closesocket(sock);
 return _T(""); // 超时或错误
 }

 //计算延迟
 latency = GetTickCount() - startTime;

 // 提取IP
 CString ip = ExtractIPFromResponse(response, recvResult);

 closesocket(sock);
 return ip;
}

// 查询国内DNS
CString CDNSQueryRouter::QueryDomesticDNS(const CString& domain, int& latency)
{
 DNSServerInfo domesticDNS = m_pServerManager->GetFastestDomesticDNS();
 
 if (domesticDNS.ip.IsEmpty()) {
 TRACE(_T("[路由] 没有可用的国内DNS\n"));
 return _T("");
 }
 
 TRACE(_T("[路由] 查询国内DNS: %s (%s)\n"), domesticDNS.name, domesticDNS.ip);
 
 CString result = DoQuery(domesticDNS.ip, domain, latency);
 
 if (!result.IsEmpty()) {
 TRACE(_T("[路由] %s -> 国内DNS结果: %s (%dms)\n"), 
 domain, result, latency);
 }
 
 return result;
}

// 查询海外DNS
CString CDNSQueryRouter::QueryOverseasDNS(const CString& domain, int& latency)
{
 auto overseasDNS = m_pServerManager->GetFastestOverseasDNS(1);
 
 if (overseasDNS.empty() || overseasDNS[0].ip.IsEmpty()) {
 TRACE(_T("[路由] 没有可用的海外DNS\n"));
 return _T("");
 }
 
 TRACE(_T("[路由] 查询海外DNS: %s (%s)\n"), 
 overseasDNS[0].name, overseasDNS[0].ip);
 
 CString result = DoQuery(overseasDNS[0].ip, domain, latency);
 
 if (!result.IsEmpty()) {
 TRACE(_T("[路由] %s -> 海外DNS结果: %s (%dms)\n"), 
 domain, result, latency);
 }
 
 return result;
}

// 双向查询并验证
CString CDNSQueryRouter::QueryBothAndValidate(const CString& domain, int& latency)
{
 TRACE(_T("[路由] 未知域名，开始双向查询: %s\n"), domain);
 
 // 同时向国内DNS和海外DNS查询（并发）
 int domesticLatency =0;
 int overseasLatency =0;
 
 auto domesticFuture = std::async(std::launch::async, [&]() {
 return QueryDomesticDNS(domain, domesticLatency);
 });
 
 auto overseasFuture = std::async(std::launch::async, [&]() {
 return QueryOverseasDNS(domain, overseasLatency);
 });
 
 // 等待结果
 CString domesticIP = domesticFuture.get();
 CString overseasIP = overseasFuture.get();
 
 // 优先使用国内结果（速度快）
 if (!domesticIP.IsEmpty()) {
 // 验证是否污染
 if (ValidateResult(domesticIP)) {
// 未污染，使用国内结果
 latency = domesticLatency;
 TRACE(_T("[路由] 使用国内DNS结果: %s (%dms)\n"), 
 domesticIP, latency);
 return domesticIP;
 } else {
 TRACE(_T("[路由] 国内DNS结果被污染: %s\n"), domesticIP);
 }
 }
 
 // 国内结果污染或失败，使用海外结果
 if (!overseasIP.IsEmpty()) {
 latency = overseasLatency;
 TRACE(_T("[路由] 使用海外DNS结果: %s (%dms)\n"), 
 overseasIP, latency);
 
 // 学习：标记该域名为海外域名
 // 下次直接走海外DNS
 // TODO: 实现学习机制
 
 return overseasIP;
 }
 
 // 都失败
 TRACE(_T("[路由] 双向查询都失败: %s\n"), domain);
 latency = -1;
 return _T("");
}

// 路由查询（核心函数）
CString CDNSQueryRouter::RouteQuery(const CString& domain, int& outLatency)
{
 //1. 分类域名
 DomainType type = m_pClassifier->ClassifyDomain(domain);
 
 //2. 根据分类选择查询方式
 CString result;
 
 switch (type) {
 case DOMESTIC:
 // 国内域名 -> 国内DNS
 result = QueryDomesticDNS(domain, outLatency);
 break;
 
 case FOREIGN:
 case ASSOCIATED:
 // 国外域名或关联域名 -> 海外DNS
 result = QueryOverseasDNS(domain, outLatency);
 break;
 
 case UNKNOWN:
 // 未知域名 -> 双向查询+验证
 result = QueryBothAndValidate(domain, outLatency);
 break;
 }
 
 return result;
}
