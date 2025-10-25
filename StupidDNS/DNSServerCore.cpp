// DNSServerCore.cpp: DNS服务器核心实现
//

#include "pch.h"
#include "DNSServerCore.h"

// 构造函数
CDNSServerCore::CDNSServerCore()
    : m_nPort(53)
    , m_socket(INVALID_SOCKET)
    , m_bRunning(FALSE)
    , m_hThread(NULL)
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
    
  if (sscanf_s(ipA, "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) != 4) {
      return FALSE;
    }
    
    for (int i = 0; i < 4; i++) {
  if (parts[i] < 0 || parts[i] > 255) {
  return FALSE;
        }
        bytes[i] = (unsigned char)parts[i];
    }
    
    return TRUE;
}

// 从DNS查询包中提取域名
CString CDNSServerCore::ExtractDomainFromQuery(const char* queryPacket, int queryLen)
{
    if (queryLen < 12) {
return _T("");
    }
 
 // 跳过Header（12字节）
    int pos = 12;
    CString domain;
    
    while (pos < queryLen) {
        int labelLen = (unsigned char)queryPacket[pos];
        
    if (labelLen == 0) {
    // 域名结束
            break;
   }
        
        if (labelLen > 63 || pos + labelLen >= queryLen) {
            // 无效的标签长度
       return _T("");
     }
     
        pos++;
        
        // 提取标签
  for (int i = 0; i < labelLen; i++) {
   char ch = queryPacket[pos + i];
   domain += (TCHAR)ch;
        }
        
  pos += labelLen;
        
     // 添加点号（如果不是最后一个标签）
     if (pos < queryLen && queryPacket[pos] != 0) {
      domain += _T('.');
   }
    }
    
    return domain;
}

// 构造DNS响应包
int CDNSServerCore::BuildDNSResponse(const char* queryPacket, int queryLen,
        const CString& ip, char* responseBuffer, int bufferSize)
{
    if (queryLen < 12 || queryLen > bufferSize) {
  return -1;
    }
    
    // 复制查询包作为基础
  memcpy(responseBuffer, queryPacket, queryLen);
    
    // 修改Header标志位
    // QR=1 (响应), AA=1 (权威), RD=1 (递归期望), RA=1 (递归可用)
    responseBuffer[2] = 0x81;  // 10000001
    responseBuffer[3] = 0x80;  // 10000000
    
    // Answer count = 1
    responseBuffer[6] = 0x00;
  responseBuffer[7] = 0x01;
    
    int pos = queryLen;
    
    // Answer部分
    // NAME: 指针指向Question中的域名（0xC00C）
 responseBuffer[pos++] = 0xC0;
    responseBuffer[pos++] = 0x0C;
    
    // TYPE: A (0x0001)
    responseBuffer[pos++] = 0x00;
    responseBuffer[pos++] = 0x01;
    
    // CLASS: IN (0x0001)
    responseBuffer[pos++] = 0x00;
    responseBuffer[pos++] = 0x01;
    
    // TTL: 300秒
    responseBuffer[pos++] = 0x00;
 responseBuffer[pos++] = 0x00;
    responseBuffer[pos++] = 0x01;
    responseBuffer[pos++] = 0x2C;  // 300
    
    // RDLENGTH: 4
    responseBuffer[pos++] = 0x00;
    responseBuffer[pos++] = 0x04;
    
    // RDATA: IP地址（4字节）
    unsigned char ipBytes[4];
    if (!IPStringToBytes(ip, ipBytes)) {
        return -1;
    }
    
    for (int i = 0; i < 4; i++) {
   responseBuffer[pos++] = ipBytes[i];
    }
    
    return pos;
}

// 转发DNS查询到上游服务器
CString CDNSServerCore::ForwardQuery(const CString& dnsServer, const char* queryPacket,
int queryLen, int& outLatency)
{
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
   return _T("");
    }
    
 // 设置超时
    DWORD timeout = 2000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    // 设置DNS服务器地址
    sockaddr_in dnsAddr;
    memset(&dnsAddr, 0, sizeof(dnsAddr));
    dnsAddr.sin_family = AF_INET;
    dnsAddr.sin_port = htons(53);
    
    CStringA dnsServerA(dnsServer);
    dnsAddr.sin_addr.s_addr = inet_addr(dnsServerA);
    
    if (dnsAddr.sin_addr.s_addr == INADDR_NONE) {
  closesocket(sock);
        return _T("");
    }
    
    // 记录开始时间
    DWORD startTime = GetTickCount();
    
    // 发送查询
  int sendResult = sendto(sock, queryPacket, queryLen, 0,
          (sockaddr*)&dnsAddr, sizeof(dnsAddr));
    if (sendResult == SOCKET_ERROR) {
        closesocket(sock);
   return _T("");
    }
    
    // 接收响应
  char response[512];
    sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);
    
    int recvResult = recvfrom(sock, response, sizeof(response), 0,
   (sockaddr*)&fromAddr, &fromLen);
    
    if (recvResult <= 0) {
  closesocket(sock);
        return _T("");  // 超时或错误
    }
    
    // 计算延迟
    outLatency = GetTickCount() - startTime;
  
    // 从响应中提取IP
    CString ip = m_queryRouter.ExtractIPFromResponse(response, recvResult);
    
    closesocket(sock);
    return ip;
}

// 处理DNS查询
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
    int latency = 0;
    DomainType type = m_pClassifier->ClassifyDomain(domain);
    
    CString resultIP;
    CString typeStr;
    
    switch (type) {
    case DOMESTIC:
        typeStr = _T("国内");
        m_nDomesticQueries++;
    {
     DNSServerInfo domesticDNS = m_pServerManager->GetFastestDomesticDNS();
            if (!domesticDNS.ip.IsEmpty()) {
    resultIP = ForwardQuery(domesticDNS.ip, queryPacket, queryLen, latency);
 }
        }
        break;
      
    case FOREIGN:
    case ASSOCIATED:
        typeStr = _T("海外");
 m_nForeignQueries++;
        {
            auto overseasDNS = m_pServerManager->GetFastestOverseasDNS(1);
   if (!overseasDNS.empty() && !overseasDNS[0].ip.IsEmpty()) {
       resultIP = ForwardQuery(overseasDNS[0].ip, queryPacket, queryLen, latency);
  }
        }
  break;
        
 case UNKNOWN:
    typeStr = _T("未知");
 // 尝试国内DNS
 {
         DNSServerInfo domesticDNS = m_pServerManager->GetFastestDomesticDNS();
 if (!domesticDNS.ip.IsEmpty()) {
       resultIP = ForwardQuery(domesticDNS.ip, queryPacket, queryLen, latency);
        
       // 验证是否污染
   if (!resultIP.IsEmpty() && !m_pClassifier->IsPoisonedIP(resultIP)) {
     m_nDomesticQueries++;
 } else {
         // 污染，尝试海外DNS
           auto overseasDNS = m_pServerManager->GetFastestOverseasDNS(1);
              if (!overseasDNS.empty()) {
resultIP = ForwardQuery(overseasDNS[0].ip, queryPacket, queryLen, latency);
            m_nForeignQueries++;
       }
       }
    }
        }
        break;
  }
  
    m_nTotalQueries++;
    
    // 记录日志
    LogQuery(domain, typeStr, resultIP.IsEmpty() ? _T("失败") : _T("成功"), 
    resultIP, latency);
    
    // 构造响应包并发送
    if (!resultIP.IsEmpty()) {
   char responseBuffer[512];
        int responseLen = BuildDNSResponse(queryPacket, queryLen, resultIP,
        responseBuffer, sizeof(responseBuffer));
        
        if (responseLen > 0) {
            sendto(m_socket, responseBuffer, responseLen, 0,
    (sockaddr*)&clientAddr, sizeof(clientAddr));
  }
    }
}

// 记录日志
void CDNSServerCore::LogQuery(const CString& domain, const CString& type,
const CString& result, const CString& ip, int latency)
{
    if (m_logCallback) {
  m_logCallback(domain, type, result, ip, latency);
    }
}

// 服务器线程
DWORD WINAPI CDNSServerCore::ServerThread(LPVOID lpParam)
{
    CDNSServerCore* pServer = (CDNSServerCore*)lpParam;
    
    TRACE(_T("DNS服务器线程启动\n"));
    
    char buffer[512];
 sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    while (pServer->m_bRunning) {
   // 接收DNS查询
     int recvLen = recvfrom(pServer->m_socket, buffer, sizeof(buffer), 0,
        (sockaddr*)&clientAddr, &clientAddrLen);
  
 if (recvLen > 0) {
   // 处理查询
  pServer->HandleDNSQuery(buffer, recvLen, clientAddr);
        }
  else if (recvLen == SOCKET_ERROR) {
   int error = WSAGetLastError();
       if (error != WSAETIMEDOUT && error != WSAEWOULDBLOCK) {
   TRACE(_T("接收错误: %d\n"), error);
       break;
   }
        }
    }
    
 TRACE(_T("DNS服务器线程退出\n"));
    return 0;
}

// 启动服务器
BOOL CDNSServerCore::Start()
{
    if (m_bRunning) {
        return FALSE;  // 已经在运行
    }
    
    // 初始化Winsock
    WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
   TRACE(_T("WSAStartup失败\n"));
        return FALSE;
    }
    
    // 创建UDP Socket
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
   TRACE(_T("创建Socket失败\n"));
  WSACleanup();
   return FALSE;
    }
    
 // 设置为非阻塞模式
 u_long mode = 1;
    ioctlsocket(m_socket, FIONBIO, &mode);
    
    // 绑定端口
 sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_nPort);
    
    if (bind(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        TRACE(_T("绑定端口 %d 失败，错误: %d\n"), m_nPort, WSAGetLastError());
   closesocket(m_socket);
        m_socket = INVALID_SOCKET;
   WSACleanup();
  return FALSE;
    }
    
    // 启动工作线程
    m_bRunning = TRUE;
    m_hThread = CreateThread(NULL, 0, ServerThread, this, 0, NULL);
    
    if (m_hThread == NULL) {
        TRACE(_T("创建线程失败\n"));
        m_bRunning = FALSE;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        WSACleanup();
   return FALSE;
    }
    
    TRACE(_T("DNS服务器启动成功，监听端口 %d\n"), m_nPort);
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
        WaitForSingleObject(m_hThread, 5000);
        CloseHandle(m_hThread);
   m_hThread = NULL;
    }
    
    // 关闭Socket
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
 m_socket = INVALID_SOCKET;
    }
    
    WSACleanup();
    
    TRACE(_T("DNS服务器已停止\n"));
}
