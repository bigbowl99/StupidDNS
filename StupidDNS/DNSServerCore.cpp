// DNSServerCore.cpp: DNS服务器核心实现
//

#include "pch.h"
#include "DNSServerCore.h"

CDNSServerCore::CDNSServerCore()
    : m_nPort(53)
    , m_socket(INVALID_SOCKET)
    , m_tcpListenSocket(INVALID_SOCKET)
    , m_bRunning(FALSE)
    , m_hThread(NULL)
    , m_hTcpThread(NULL)
    , m_pServerManager(nullptr)
    , m_pClassifier(nullptr)
    , m_nTotalQueries(0)
    , m_nDomesticQueries(0)
    , m_nForeignQueries(0)
{
}

// 析构函数
CDNSServerCore::~CDNSServerCore()
{
    Stop();
}

// 初始化
BOOL CDNSServerCore::Initialize(int port, 
      CDNSServerManager* pServerManager,
              CDomainClassifier* pClassifier)
{
    m_nPort = port;
    m_pServerManager = pServerManager;
    m_pClassifier = pClassifier;
  
    // 初始化查询路由器
    if (!m_queryRouter.Initialize(pServerManager, pClassifier)) {
    TRACE(_T("DNS查询路由器初始化失败\n"));
   return FALSE;
    }
    
    return TRUE;
}

// IP字符串转为4字节
BOOL CDNSServerCore::IPStringToBytes(const CString& ip, unsigned char* bytes)
{
    CStringA ipA(ip);
    int parts[4];
    
  if (sscanf_s(ipA, "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) !=4) {
      return FALSE;
    }
    
    for (int i =0; i <4; i++) {
  if (parts[i] <0 || parts[i] >255) {
  return FALSE;
        }
        bytes[i] = (unsigned char)parts[i];
    }
    
    return TRUE;
}

// 从DNS查询包中提取域名
CString CDNSServerCore::ExtractDomainFromQuery(const char* queryPacket, int queryLen)
{
    if (queryLen <12) {
return _T("");
 }
 
 // 跳过Header（12字节）
 int pos =12;
 CString domain;
 
 while (pos < queryLen) {
 if (pos >= queryLen) return _T("");
 unsigned char labelLen = (unsigned char)queryPacket[pos];
 
 if (labelLen ==0) {
 // 域名结束
 pos++;
 break;
 }
 
 // 确保后续读取 labelLen 个字节不会越界:需要 pos+1+labelLen <= queryLen
 if (labelLen >63 || pos +1 + (int)labelLen > queryLen) {
 // 无效的标签长度
 return _T("");
 }
 
 pos++;
 
 // 提取标签
 for (int i =0; i < labelLen; i++) {
 char ch = queryPacket[pos + i];
 domain += (TCHAR)ch;
 }
 
 pos += labelLen;
 
 // 添加点号（如果不是最后一个标签）
 if (pos < queryLen && (unsigned char)queryPacket[pos] !=0) {
 domain += _T('.');
 }
 }
 
 return domain;
}

// 构造DNS响应包（简单构造一条A记录的权威回答）
int CDNSServerCore::BuildDNSResponse(const char* queryPacket, int queryLen,
 const CString& ip, char* responseBuffer, int bufferSize)
{
 if (queryLen <12 || queryLen > bufferSize) {
 return -1;
 }
 
 //复制查询包作为基础
 memcpy(responseBuffer, queryPacket, queryLen);
 
 // 修改Header标志位
 // QR=1 (响应), AA=0 (非权威), RD=保持原值, RA=1
 responseBuffer[2] = (responseBuffer[2] |0x80); //置QR
 responseBuffer[3] =0x80; // RA=1, RCODE=0
 
 // QDCOUNT 保持原值，设置 Answer count =1, NS/AR =0
 if (queryLen <12) return -1;
 responseBuffer[6] =0x00; responseBuffer[7] =0x01; // ANCOUNT=1
 responseBuffer[8] =0x00; responseBuffer[9] =0x00; // NSCOUNT=0
 responseBuffer[10] =0x00; responseBuffer[11] =0x00; // ARCOUNT=0
 
 int pos = queryLen;
 
 //需要保证足够空间: NAME(2)+TYPE(2)+CLASS(2)+TTL(4)+RDLEN(2)+RDATA(4) =16
 if (pos +16 > bufferSize) return -1;
 
 // Answer部分
 // NAME: 指针指向Question中的域名（0xC00C）
 responseBuffer[pos++] =0xC0;
 responseBuffer[pos++] =0x0C;
 
 // TYPE: A (0x0001)
 responseBuffer[pos++] =0x00;
 responseBuffer[pos++] =0x01;
 
 // CLASS: IN (0x0001)
 responseBuffer[pos++] =0x00;
 responseBuffer[pos++] =0x01;
 
 // TTL:300秒
 responseBuffer[pos++] =0x00;
 responseBuffer[pos++] =0x00;
 responseBuffer[pos++] =0x01;
 responseBuffer[pos++] =0x2C; //300
 
 // RDLENGTH:4
 responseBuffer[pos++] =0x00;
 responseBuffer[pos++] =0x04;
 
 // RDATA: IP地址（4字节）
 unsigned char ipBytes[4];
 if (!IPStringToBytes(ip, ipBytes)) {
 return -1;
 }
 
 for (int i =0; i <4; i++) {
 responseBuffer[pos++] = ipBytes[i];
 }
 
 return pos;
}

// 构造最小 SERVFAIL 响应
int CDNSServerCore::BuildServFailResponse(const char* queryPacket, int queryLen,
 char* responseBuffer, int bufferSize)
{
 if (queryLen <12 || queryLen > bufferSize) return -1;
 memcpy(responseBuffer, queryPacket, queryLen);
 // 设置为响应 + RCODE=2(SERVFAIL) + RA=1
 responseBuffer[2] = (responseBuffer[2] |0x80); // QR=1
 responseBuffer[3] =0x82; //10000010 -> RA=1, RCODE=2
 // 保持 QDCOUNT，清空 AN/NS/AR
 responseBuffer[6] =0x00; responseBuffer[7] =0x00; // AN=0
 responseBuffer[8] =0x00; responseBuffer[9] =0x00; // NS=0
 responseBuffer[10] =0x00; responseBuffer[11] =0x00; // AR=0
 return queryLen; //只有头和原始 Question
}

// 转发DNS查询到上游服务器，并透传原始响应
BOOL CDNSServerCore::ForwardQuery(const CString& dnsServer, const char* queryPacket,
int queryLen, char* outResponse, int outResponseSize, int& outResponseLen,
 CString& outIP, int& outLatency)
{
 outLatency = -1;
 outResponseLen =0;
 outIP.Empty();
 
 SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 if (sock == INVALID_SOCKET) {
 return FALSE;
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
 return FALSE;
 }
 
 //记录开始时间
 DWORD startTime = GetTickCount();
 
 //发送查询
 int sendResult = sendto(sock, queryPacket, queryLen,0,
 (sockaddr*)&dnsAddr, sizeof(dnsAddr));
 if (sendResult == SOCKET_ERROR) {
 closesocket(sock);
 return FALSE;
 }
 
 // 接收响应
 sockaddr_in fromAddr;
 int fromLen = sizeof(fromAddr);
 int recvResult = recvfrom(sock, outResponse, outResponseSize,0,
 (sockaddr*)&fromAddr, &fromLen);
 
 if (recvResult <=0) {
 closesocket(sock);
 return FALSE; // 超时或错误
 }
 
 //计算延迟
 outLatency = GetTickCount() - startTime;
 outResponseLen = recvResult;
 
 // 尝试从响应中提取一个 A记录的 IP仅用于日志
 outIP = m_queryRouter.ExtractIPFromResponse(outResponse, recvResult);
 
 closesocket(sock);
 return TRUE;
}

// 转发DNS查询到上游服务器（TCP），并透传原始响应
BOOL CDNSServerCore::ForwardQueryTCP(const CString& dnsServer, const char* queryPacket,
 int queryLen, char* outResponse, int outResponseSize, int& outResponseLen,
 CString& outIP, int& outLatency)
{
 outLatency = -1;
 outResponseLen =0;
 outIP.Empty();
 
 SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 if (sock == INVALID_SOCKET) {
 return FALSE;
 }
 
 // 设置超时
 DWORD timeout =3000;
 setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
 setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
 
 // 设置DNS服务器地址
 sockaddr_in dnsAddr;
 memset(&dnsAddr,0, sizeof(dnsAddr));
 dnsAddr.sin_family = AF_INET;
 dnsAddr.sin_port = htons(53);
 
 CStringA dnsServerA(dnsServer);
 dnsAddr.sin_addr.s_addr = inet_addr(dnsServerA);
 
 if (dnsAddr.sin_addr.s_addr == INADDR_NONE) {
 closesocket(sock);
 return FALSE;
 }
 
 //记录开始时间
 DWORD startTime = GetTickCount();
 
 // 建立TCP连接
 if (connect(sock, (sockaddr*)&dnsAddr, sizeof(dnsAddr)) == SOCKET_ERROR) {
 closesocket(sock);
 return FALSE;
 }
 
 // TCP DNS 协议前面需要2字节长度
 unsigned short netLen = htons((unsigned short)queryLen);
 if (send(sock, (const char*)&netLen,2,0) !=2) {
 closesocket(sock);
 return FALSE;
 }
 if (send(sock, queryPacket, queryLen,0) != queryLen) {
 closesocket(sock);
 return FALSE;
 }
 
 //先收2字节长度
 unsigned short respLen =0;
 int r = recv(sock, (char*)&respLen,2, MSG_WAITALL);
 if (r !=2) {
 closesocket(sock);
 return FALSE;
 }
 respLen = ntohs(respLen);
 if (respLen > outResponseSize) {
 closesocket(sock);
 return FALSE;
 }
 int got =0;
 while (got < respLen) {
 int chunk = recv(sock, outResponse + got, respLen - got,0);
 if (chunk <=0) {
 closesocket(sock);
 return FALSE;
 }
 got += chunk;
 }
 outLatency = GetTickCount() - startTime;
 outResponseLen = respLen;
 
 // 尝试从响应中提取一个 A记录的 IP仅用于日志
 outIP = m_queryRouter.ExtractIPFromResponse(outResponse, outResponseLen);
 
 closesocket(sock);
 return TRUE;
}

// 处理DNS查询（UDP）
void CDNSServerCore::HandleDNSQuery(const char* queryPacket, int queryLen,
 const sockaddr_in& clientAddr)
{
 // 提取域名
 CString domain = ExtractDomainFromQuery(queryPacket, queryLen);
 
 if (domain.IsEmpty()) {
 TRACE(_T("无效的DNS查询包\n"));
 return;
 }
 
 TRACE(_T("[DNS请求] %s\n"), domain);
 
 // 路由查询
 int latency =0;
 DomainType type = m_pClassifier->ClassifyDomain(domain);
 
 CString resultIP;
 CString typeStr;
 
 // 用于透传上游完整响应
 char upstreamResp[65536];
 int upstreamLen =0;
 BOOL gotUpstream = FALSE;
 
 auto tryForward = [&](const CString& dns){
 if (dns.IsEmpty()) return FALSE;
 BOOL ok = ForwardQuery(dns, queryPacket, queryLen,
 upstreamResp, sizeof(upstreamResp), upstreamLen,
 resultIP, latency);
 if (!ok) TRACE(_T("[Forward-UDP] %s失败\n"), dns);
 return ok;
 };

 switch (type) {
 case DOMESTIC:
 typeStr = _T("国内");
 m_nDomesticQueries++;
 {
 DNSServerInfo domesticDNS = m_pServerManager->GetFastestDomesticDNS();
 if (!domesticDNS.ip.IsEmpty()) {
 gotUpstream = tryForward(domesticDNS.ip);
 }
 // 本地兜底
 if (!gotUpstream) {
 gotUpstream = tryForward(_T("114.114.114.114"));
 if (!gotUpstream) gotUpstream = tryForward(_T("223.5.5.5"));
 }
 }
 break;
 
 case FOREIGN:
 case ASSOCIATED:
 typeStr = _T("国外");
 m_nForeignQueries++;
 {
 auto overseasDNS = m_pServerManager->GetFastestOverseasDNS(1);
 if (!overseasDNS.empty() && !overseasDNS[0].ip.IsEmpty()) {
 gotUpstream = tryForward(overseasDNS[0].ip);
 }
 // 本地兜底
 if (!gotUpstream) {
 gotUpstream = tryForward(_T("1.1.1.1"));
 if (!gotUpstream) gotUpstream = tryForward(_T("8.8.8.8"));
 }
 }
 break;
 
 case UNKNOWN:
 typeStr = _T("未知");
 //先国内再海外
 {
 DNSServerInfo domesticDNS = m_pServerManager->GetFastestDomesticDNS();
 if (!domesticDNS.ip.IsEmpty()) {
 gotUpstream = tryForward(domesticDNS.ip);
 }
 if (gotUpstream && !resultIP.IsEmpty() && !m_pClassifier->IsPoisonedIP(resultIP)) {
 m_nDomesticQueries++;
 } else {
 // 国内失败或污染 -> 海外
 auto overseasDNS = m_pServerManager->GetFastestOverseasDNS(1);
 if (!overseasDNS.empty()) {
 gotUpstream = tryForward(overseasDNS[0].ip);
 }
 //兜底：公共DNS
 if (!gotUpstream) {
 gotUpstream = tryForward(_T("1.1.1.1"));
 if (!gotUpstream) gotUpstream = tryForward(_T("8.8.8.8"));
 }
 if (gotUpstream) m_nForeignQueries++;
 }
 }
 break;
 }
 
 m_nTotalQueries++;
 
 //记录日志
 LogQuery(domain, typeStr, gotUpstream ? _T("成功") : _T("失败"), 
 resultIP, latency);
 
 //发送响应
 if (gotUpstream) {
 sendto(m_socket, upstreamResp, upstreamLen,0,
 (sockaddr*)&clientAddr, sizeof(clientAddr));
 } else {
 char servfail[512];
 int len = BuildServFailResponse(queryPacket, queryLen, servfail, sizeof(servfail));
 if (len >0) {
 sendto(m_socket, servfail, len,0, (sockaddr*)&clientAddr, sizeof(clientAddr));
 }
 }
}

// 处理DNS查询（TCP）
void CDNSServerCore::HandleDNSQueryTCP(const char* queryPacket, int queryLen, SOCKET clientSock)
{
 // 提取域名
 CString domain = ExtractDomainFromQuery(queryPacket, queryLen);
 
 if (domain.IsEmpty()) {
 TRACE(_T("无效的DNS查询包\n"));
 return;
 }
 
 TRACE(_T("[DNS请求-TCP] %s\n"), domain);
 
 // 路由查询
 int latency =0;
 DomainType type = m_pClassifier->ClassifyDomain(domain);
 
 CString resultIP;
 CString typeStr;
 
 // 用于透传上游完整响应
 char upstreamResp[65536];
 int upstreamLen =0;
 BOOL gotUpstream = FALSE;
 
 auto tryForwardTcp = [&](const CString& dns){
 if (dns.IsEmpty()) return FALSE;
 BOOL ok = ForwardQueryTCP(dns, queryPacket, queryLen,
 upstreamResp, sizeof(upstreamResp), upstreamLen,
 resultIP, latency);
 if (!ok) TRACE(_T("[Forward-TCP] %s失败\n"), dns);
 return ok;
 };

 switch (type) {
 case DOMESTIC:
 typeStr = _T("国内");
 m_nDomesticQueries++;
 {
 DNSServerInfo domesticDNS = m_pServerManager->GetFastestDomesticDNS();
 if (!domesticDNS.ip.IsEmpty()) {
 gotUpstream = tryForwardTcp(domesticDNS.ip);
 }
 // 本地兜底
 if (!gotUpstream) {
 gotUpstream = tryForwardTcp(_T("114.114.114.114"));
 if (!gotUpstream) gotUpstream = tryForwardTcp(_T("223.5.5.5"));
 }
 }
 break;
 
 case FOREIGN:
 case ASSOCIATED:
 typeStr = _T("国外");
 m_nForeignQueries++;
 {
 auto overseasDNS = m_pServerManager->GetFastestOverseasDNS(1);
 if (!overseasDNS.empty() && !overseasDNS[0].ip.IsEmpty()) {
 gotUpstream = tryForwardTcp(overseasDNS[0].ip);
 }
 // 本地兜底
 if (!gotUpstream) {
 gotUpstream = tryForwardTcp(_T("1.1.1.1"));
 if (!gotUpstream) gotUpstream = tryForwardTcp(_T("8.8.8.8"));
 }
 }
 break;
 
 case UNKNOWN:
 typeStr = _T("未知");
 {
 DNSServerInfo domesticDNS = m_pServerManager->GetFastestDomesticDNS();
 if (!domesticDNS.ip.IsEmpty()) {
 gotUpstream = tryForwardTcp(domesticDNS.ip);
 }
 if (gotUpstream && !resultIP.IsEmpty() && !m_pClassifier->IsPoisonedIP(resultIP)) {
 m_nDomesticQueries++;
 } else {
 auto overseasDNS = m_pServerManager->GetFastestOverseasDNS(1);
 if (!overseasDNS.empty()) {
 gotUpstream = tryForwardTcp(overseasDNS[0].ip);
 }
 //兜底：公共DNS
 if (!gotUpstream) {
 gotUpstream = tryForwardTcp(_T("1.1.1.1"));
 if (!gotUpstream) gotUpstream = tryForwardTcp(_T("8.8.8.8"));
 }
 if (gotUpstream) m_nForeignQueries++;
 }
 }
 break;
 }
 
 m_nTotalQueries++;
 
 //记录日志
 LogQuery(domain, typeStr, gotUpstream ? _T("成功") : _T("失败"), 
 resultIP, latency);
 
 //发送响应
 if (gotUpstream) {
 // 写2字节长度 + 报文
 unsigned short netLen = htons((unsigned short)upstreamLen);
 send(clientSock, (const char*)&netLen,2,0);
 send(clientSock, upstreamResp, upstreamLen,0);
 } else {
 // 返回SERVFAIL
 char servfail[512];
 int len = BuildServFailResponse(queryPacket, queryLen, servfail, sizeof(servfail));
 if (len >0) {
 unsigned short netLen = htons((unsigned short)len);
 send(clientSock, (const char*)&netLen,2,0);
 send(clientSock, servfail, len,0);
 }
 }
}

// 日志记录
void CDNSServerCore::LogQuery(const CString& domain, const CString& type,
const CString& result, const CString& ip, int latency)
{
 if (m_logCallback) {
 m_logCallback(domain, type, result, ip, latency);
 }
}

// UDP服务器线程
DWORD WINAPI CDNSServerCore::ServerThread(LPVOID lpParam)
{
 CDNSServerCore* pServer = (CDNSServerCore*)lpParam;
 
 TRACE(_T("[ServerThread] DNS服务器线程启动\n"));
 TRACE(_T("[ServerThread] Socket: %d, Port: %d\n"), pServer->m_socket, pServer->m_nPort);
 
 char buffer[512];
 sockaddr_in clientAddr;
 int clientAddrLen = sizeof(clientAddr);
 
 int loopCount =0;
 int errorCount =0;

 while (pServer->m_bRunning) {
 loopCount++;
 if (loopCount %1000 ==0) {
 TRACE(_T("[ServerThread] 等待DNS查询... (已循环 %d 次, 错误 %d 次)\n"), 
 loopCount, errorCount);
 }
 
 // 重置客户端地址长度
 clientAddrLen = sizeof(clientAddr);
 memset(&clientAddr,0, sizeof(clientAddr));
 
 // 接收DNS查询
 int recvLen = recvfrom(pServer->m_socket, buffer, sizeof(buffer),0,
 (sockaddr*)&clientAddr, &clientAddrLen);
 
 if (recvLen >0) {
 char clientIP[INET_ADDRSTRLEN];
 inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
 
 TRACE(_T("[ServerThread] ★★★ 收到DNS查询！★★★\n"));
 TRACE(_T("[ServerThread] 长度: %d 字节\n"), recvLen);
 TRACE(_T("[ServerThread] 来自: %S:%d\n"), clientIP, ntohs(clientAddr.sin_port));
 
 // 打印前16字节（十六进制）
 TRACE(_T("[ServerThread] 数据: "));
 for (int i =0; i < min(16, recvLen); i++) {
 TRACE(_T("%02X "), (unsigned char)buffer[i]);
 }
 TRACE(_T("\n"));
 
 //处理查询
 pServer->HandleDNSQuery(buffer, recvLen, clientAddr);
 }
 else if (recvLen == SOCKET_ERROR) {
 int error = WSAGetLastError();
 if (error == WSAEWOULDBLOCK) {
 // 非阻塞模式下没有数据，休眠一下避免CPU100%
 Sleep(10);
 }
 else if (error != WSAETIMEDOUT) {
 errorCount++;
 TRACE(_T("[ServerThread] ? 接收错误: %d (总错误: %d)\n"), error, errorCount);
 if (errorCount >10) {
 TRACE(_T("[ServerThread] 错误次数过多，线程退出\n"));
 break;
 }
 }
 }
 else if (recvLen ==0) {
 TRACE(_T("[ServerThread] recvfrom返回0（连接关闭？）\n"));
 }
 }
 
 TRACE(_T("[ServerThread] DNS服务器线程退出\n"));
 return 0;
}

// TCP服务器线程
DWORD WINAPI CDNSServerCore::ServerTcpThread(LPVOID lpParam)
{
 CDNSServerCore* pServer = (CDNSServerCore*)lpParam;
 
 TRACE(_T("[ServerTcpThread] DNS服务器TCP线程启动\n"));
 TRACE(_T("[ServerTcpThread] Socket: %d, Port: %d\n"), pServer->m_tcpListenSocket, pServer->m_nPort);
 
 while (pServer->m_bRunning) {
 sockaddr_in caddr;
 int clen = sizeof(caddr);
 
 SOCKET cs = accept(pServer->m_tcpListenSocket, (sockaddr*)&caddr, &clen);
 if (cs == INVALID_SOCKET) {
 int err = WSAGetLastError();
 if (err == WSAEWOULDBLOCK) {
 Sleep(5);
 continue;
 }
 else {
 break;
 }
 }
 
 TRACE(_T("[ServerTcpThread] 新连接: %d\n"), cs);
 
 // 每个连接处理一次或多次请求（简单处理：一次收一条）
 for (;;) {
 unsigned short qlen =0;
 int r = recv(cs, (char*)&qlen,2, MSG_WAITALL);
 if (r !=2) break;
 qlen = ntohs(qlen);
 if (qlen ==0 || qlen >65535) break;
 std::vector<char> qbuf(qlen);
 int got =0;
 while (got < qlen) {
 int chunk = recv(cs, qbuf.data() + got, qlen - got,0);
 if (chunk <=0) { got = -1; break; }
 got += chunk;
 }
 if (got != (int)qlen) break;
 pServer->HandleDNSQueryTCP(qbuf.data(), qlen, cs);
 }
 
 closesocket(cs);
 TRACE(_T("[ServerTcpThread] 关闭连接: %d\n"), cs);
 }
 
 TRACE(_T("[ServerTcpThread] DNS服务器TCP线程退出\n"));
 return 0;
}

// 启动服务器
BOOL CDNSServerCore::Start()
{
 if (m_bRunning) {
 TRACE(_T("[Start] DNS服务器已在运行\n"));
 return FALSE; // 已经在运行
 }
 
 TRACE(_T("[Start] 开始启动DNS服务器...\n"));
 
 // 初始化Winsock
 WSADATA wsaData;
 if (WSAStartup(MAKEWORD(2,2), &wsaData) !=0) {
 TRACE(_T("[Start] ? WSAStartup失败\n"));
 return FALSE;
 }
 
 TRACE(_T("[Start] ? Winsock初始化成功\n"));
 
 // 创建UDP Socket
 m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 if (m_socket == INVALID_SOCKET) {
 int error = WSAGetLastError();
 TRACE(_T("[Start] ? 创建Socket失败，错误: %d\n"), error);
 WSACleanup();
 return FALSE;
 }
 
 TRACE(_T("[Start] ? Socket创建成功，句柄: %d\n"), m_socket);
 
 //允许地址重用（重要！）
 int reuse =1;
 if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) <0) {
 TRACE(_T("[Start] 警告: 设置SO_REUSEADDR失败\n"));
 }
 
 // 设置为非阻塞模式
 u_long mode =1;
 if (ioctlsocket(m_socket, FIONBIO, &mode) !=0) {
 TRACE(_T("[Start] ? 设置非阻塞模式失败\n"));
 }
 TRACE(_T("[Start] ? Socket设置为非阻塞模式\n"));
 
 //绑定端口
 sockaddr_in serverAddr;
 memset(&serverAddr,0, sizeof(serverAddr));
 serverAddr.sin_family = AF_INET;
 serverAddr.sin_addr.s_addr = INADDR_ANY; //监听所有接口（包括127.0.0.1和真实IP）
 serverAddr.sin_port = htons(m_nPort);
 
 TRACE(_T("[Start] 尝试绑定端口 %d (0.0.0.0:%d)...\n"), m_nPort, m_nPort);
 
 if (bind(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
 int error = WSAGetLastError();
 TRACE(_T("[Start] ?绑定端口 %d失败，错误码: %d\n"), m_nPort, error);
 
 CString errorMsg;
 errorMsg.Format(_T("无法绑定端口 %d！\n\n错误码: %d\n\n"), m_nPort, error);
 
 if (error == WSAEACCES || error ==10013) {
 errorMsg += _T("原因：需要管理员权限\n\n");
 errorMsg += _T("解决方案：以管理员身份运行程序。\n");
 } else if (error == WSAEADDRINUSE || error ==10048) {
 errorMsg += _T("原因：端口已被占用（其他程序正在监听53端口）\n\n");
 errorMsg += _T("解决方案：关闭占用端口的程序或更换端口。\n");
 }
 
 AfxMessageBox(errorMsg, MB_OK | MB_ICONERROR);
 
 closesocket(m_socket);
 m_socket = INVALID_SOCKET;
 WSACleanup();
 return FALSE;
 }
 
 TRACE(_T("[Start] ? 成功绑定端口 %d\n"), m_nPort);

 // 创建TCP监听Socket
 m_tcpListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 if (m_tcpListenSocket == INVALID_SOCKET) {
 TRACE(_T("[Start] ? 创建TCP监听Socket失败，错误: %d\n"), WSAGetLastError());
 closesocket(m_socket);
 m_socket = INVALID_SOCKET;
 WSACleanup();
 return FALSE;
 }
 
 if (setsockopt(m_tcpListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) <0) {
 TRACE(_T("[Start] 警告: 设置TCP SO_REUSEADDR失败\n"));
 }
 
 // 设置为非阻塞模式
 if (ioctlsocket(m_tcpListenSocket, FIONBIO, &mode) !=0) {
 TRACE(_T("[Start] ? 设置TCP非阻塞模式失败\n"));
 }
 
 //绑定TCP端口
 sockaddr_in tcpAddr;
 memset(&tcpAddr,0, sizeof(tcpAddr));
 tcpAddr.sin_family = AF_INET;
 tcpAddr.sin_addr.s_addr = INADDR_ANY; //监听所有接口
 tcpAddr.sin_port = htons(m_nPort);
 
 TRACE(_T("[Start] 尝试绑定TCP端口 %d (0.0.0.0:%d)...\n"), m_nPort, m_nPort);
 
 if (bind(m_tcpListenSocket, (sockaddr*)&tcpAddr, sizeof(tcpAddr)) == SOCKET_ERROR) {
 int error = WSAGetLastError();
 TRACE(_T("[Start] ?绑定TCP端口 %d失败，错误码: %d\n"), m_nPort, error);
 closesocket(m_tcpListenSocket);
 closesocket(m_socket);
 m_tcpListenSocket = INVALID_SOCKET;
 m_socket = INVALID_SOCKET;
 WSACleanup();
 return FALSE;
 }
 
 // 开始监听
 if (listen(m_tcpListenSocket, SOMAXCONN) == SOCKET_ERROR) {
 TRACE(_T("[Start] ? TCP监听失败，错误码: %d\n"), WSAGetLastError());
 closesocket(m_tcpListenSocket);
 closesocket(m_socket);
 m_tcpListenSocket = INVALID_SOCKET;
 m_socket = INVALID_SOCKET;
 WSACleanup();
 return FALSE;
 }
 
 // 启动工作线程
 m_bRunning = TRUE;
 TRACE(_T("[Start] 创建工作线程...\n"));
 m_hThread = CreateThread(NULL,0, ServerThread, this,0, NULL);
 m_hTcpThread = CreateThread(NULL,0, ServerTcpThread, this,0, NULL);
 
 if (!m_hThread || !m_hTcpThread) {
 TRACE(_T("[Start] ? 创建线程失败\n"));
 m_bRunning = FALSE;
 closesocket(m_socket);
 closesocket(m_tcpListenSocket);
 m_socket = INVALID_SOCKET;
 m_tcpListenSocket = INVALID_SOCKET;
 WSACleanup();
 return FALSE;
 }
 
 TRACE(_T("[Start] ? 工作线程创建成功\n"));
 TRACE(_T("[Start] ========================================\n"));
 TRACE(_T("[Start] DNS服务器启动成功！\n"));
 TRACE(_T("[Start]监听地址:0.0.0.0:%d (所有网络接口)\n"), m_nPort);
 TRACE(_T("[Start] ========================================\n"));
 
 return TRUE;
}

// 停止服务器
void CDNSServerCore::Stop()
{
 if (!m_bRunning) {
 return;
 }
 
 TRACE(_T("正在停止DNS服务器...\n"));
 
 m_bRunning = FALSE;
 
 // 等待线程退出
 if (m_hThread != NULL) {
 WaitForSingleObject(m_hThread,5000);
 CloseHandle(m_hThread);
 m_hThread = NULL;
 }
 
 if (m_hTcpThread != NULL) {
 WaitForSingleObject(m_hTcpThread,5000);
 CloseHandle(m_hTcpThread);
 m_hTcpThread = NULL;
 }
 
 //关闭Socket
 if (m_socket != INVALID_SOCKET) {
 closesocket(m_socket);
 m_socket = INVALID_SOCKET;
 }
 
 if (m_tcpListenSocket != INVALID_SOCKET) {
 closesocket(m_tcpListenSocket);
 m_tcpListenSocket = INVALID_SOCKET;
 }
 
 WSACleanup();
 
 TRACE(_T("DNS服务器已停止\n"));
}
