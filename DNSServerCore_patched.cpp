// DNSServerCore.cpp: DNSʵ
//

#include "pch.h"
#include "DNSServerCore.h"

// === TCP helpers & globals (added) ===
static bool SendAll(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(s, buf + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}
static bool RecvAll(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len) {
        int n = recv(s, buf + got, len - got, 0);
        if (n <= 0) return false;
        got += n;
    }
    return true;
}
static bool SendTcpDnsMessage(SOCKET s, const char* payload, int payloadLen) {
    if (payloadLen < 0 || payloadLen > 65535) return false;
    uint16_t netLen = htons((uint16_t)payloadLen);
    return SendAll(s, (const char*)&netLen, 2) && SendAll(s, payload, payloadLen);
}
static int BuildServfailFromQuery(const char* q, int qlen, char* out, int outCap) {
    if (qlen < 12 || outCap < qlen) return -1;
    memcpy(out, q, qlen);
    unsigned char* f1 = (unsigned char*)(out + 2);
    unsigned char* f2 = (unsigned char*)(out + 3);
    unsigned char qr = 0x80; // QR=1
    unsigned char opcode = (*f1) & 0x78; // keep OPCODE
    unsigned char rd = (*f1) & 0x01; // keep RD
    *f1 = (unsigned char)(qr | opcode | rd);
    unsigned char ra = 0x80; // RA=1
    unsigned char rcode = 0x02; // SERVFAIL
    *f2 = (unsigned char)(ra | rcode);
    out[6]=out[7]=0; out[8]=out[9]=0; out[10]=out[11]=0;
    return qlen;
}

static SOCKET g_tcpListenSock = INVALID_SOCKET;
static HANDLE g_tcpThread = NULL;

static unsigned __stdcall TcpAcceptThread(void* ctx);


// 캯
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

// 
CDNSServerCore::~CDNSServerCore()
{
    Stop();
}

// ʼ
BOOL CDNSServerCore::Initialize(int port, 
      CDNSServerManager* pServerManager,
              CDomainClassifier* pClassifier)
{
    m_nPort = port;
    m_pServerManager = pServerManager;
    m_pClassifier = pClassifier;
  
    // ʼѯ·
    if (!m_queryRouter.Initialize(pServerManager, pClassifier)) {
    TRACE(_T("DNSѯ·ʼʧ\n"));
   return FALSE;
    }
    
    return TRUE;
}

// IPַתΪ4ֽ
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

// DNSѯȡ
CString CDNSServerCore::ExtractDomainFromQuery(const char* queryPacket, int queryLen)
{
    if (queryLen < 12) {
return _T("");
    }
 
 // Header12ֽڣ
    int pos = 12;
    CString domain;
    
    while (pos < queryLen) {
        int labelLen = (unsigned char)queryPacket[pos];
        
    if (labelLen == 0) {
    // 
            break;
   }
        
        if (labelLen > 63 || pos + labelLen >= queryLen) {
            // Чıǩ
       return _T("");
     }
     
        pos++;
        
        // ȡǩ
  for (int i = 0; i < labelLen; i++) {
   char ch = queryPacket[pos + i];
   domain += (TCHAR)ch;
        }
        
  pos += labelLen;
        
     // ӵţһǩ
     if (pos < queryLen && queryPacket[pos] != 0) {
      domain += _T('.');
   }
    }
    
    return domain;
}

// DNSӦ
int CDNSServerCore::BuildDNSResponse(const char* queryPacket, int queryLen,
        const CString& ip, char* responseBuffer, int bufferSize)
{
    if (queryLen < 12 || queryLen > bufferSize) {
  return -1;
    }
    
    // ƲѯΪ
  memcpy(responseBuffer, queryPacket, queryLen);
    
    // ޸Header־λ
    // QR=1 (Ӧ), AA=1 (Ȩ), RD=1 (ݹ), RA=1 (ݹ)
    responseBuffer[2] = 0x81;  // 10000001
    responseBuffer[3] = 0x80;  // 10000000
    
    // Answer count = 1
    responseBuffer[6] = 0x00;
  responseBuffer[7] = 0x01;
    
    int pos = queryLen;
    
    // Answer
    // NAME: ָָQuestionе0xC00C
 responseBuffer[pos++] = 0xC0;
    responseBuffer[pos++] = 0x0C;
    
    // TYPE: A (0x0001)
    responseBuffer[pos++] = 0x00;
    responseBuffer[pos++] = 0x01;
    
    // CLASS: IN (0x0001)
    responseBuffer[pos++] = 0x00;
    responseBuffer[pos++] = 0x01;
    
    // TTL: 300
    responseBuffer[pos++] = 0x00;
 responseBuffer[pos++] = 0x00;
    responseBuffer[pos++] = 0x01;
    responseBuffer[pos++] = 0x2C;  // 300
    
    // RDLENGTH: 4
    responseBuffer[pos++] = 0x00;
    responseBuffer[pos++] = 0x04;
    
    // RDATA: IPַ4ֽڣ
    unsigned char ipBytes[4];
    if (!IPStringToBytes(ip, ipBytes)) {
        return -1;
    }
    
    for (int i = 0; i < 4; i++) {
   responseBuffer[pos++] = ipBytes[i];
    }
    
    return pos;
}

// תDNSѯη
CString CDNSServerCore::ForwardQuery(const CString& dnsServer, const char* queryPacket,
int queryLen, int& outLatency)
{
    // ʼӳΪ0
    outLatency = 0;
    
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
   return _T("");
    }
    
 // óʱ
    DWORD timeout = 2000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    // DNSַ
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
    
    // ¼ʼʱ
    DWORD startTime = GetTickCount();
    
    // Ͳѯ
  int sendResult = sendto(sock, queryPacket, queryLen, 0,
          (sockaddr*)&dnsAddr, sizeof(dnsAddr));
    if (sendResult == SOCKET_ERROR) {
        closesocket(sock);
   return _T("");
    }
    
    // Ӧ
  char response[512];
    sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);
    
    int recvResult = recvfrom(sock, response, sizeof(response), 0,
   (sockaddr*)&fromAddr, &fromLen);
    
    if (recvResult <= 0) {
  closesocket(sock);
        return _T("");  // ʱ
    }
    
    // ӳ
    outLatency = GetTickCount() - startTime;
  
    // ӦȡIP
    CString ip = m_queryRouter.ExtractIPFromResponse(response, recvResult);
    
    closesocket(sock);
    return ip;
}

// DNSѯ
void CDNSServerCore::HandleDNSQuery(const char* queryPacket, int queryLen,
          const sockaddr_in& clientAddr)
{
 // ȡ
    CString domain = ExtractDomainFromQuery(queryPacket, queryLen);
    
    if (domain.IsEmpty()) {
        TRACE(_T("ЧDNSѯ\n"));
        return;
    }
    
    TRACE(_T("[DNS] %s\n"), domain);
    
    // ·ɲѯ
    int latency = 0;
    DomainType type = m_pClassifier->ClassifyDomain(domain);
    
    CString resultIP;
    CString typeStr;
    
    switch (type) {
    case DOMESTIC:
        typeStr = _T("");
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
        typeStr = _T("");
 m_nForeignQueries++;
        {
            auto overseasDNS = m_pServerManager->GetFastestOverseasDNS(1);
   if (!overseasDNS.empty() && !overseasDNS[0].ip.IsEmpty()) {
       resultIP = ForwardQuery(overseasDNS[0].ip, queryPacket, queryLen, latency);
  }
        }
  break;
        
 case UNKNOWN:
    typeStr = _T("δ֪");
 // ԹDNS
 {
         DNSServerInfo domesticDNS = m_pServerManager->GetFastestDomesticDNS();
 if (!domesticDNS.ip.IsEmpty()) {
       resultIP = ForwardQuery(domesticDNS.ip, queryPacket, queryLen, latency);
        
       // ֤ǷȾ
   if (!resultIP.IsEmpty() && !m_pClassifier->IsPoisonedIP(resultIP)) {
     m_nDomesticQueries++;
 } else {
         // ȾԺDNS
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
    
    // ¼־
    LogQuery(domain, typeStr, resultIP.IsEmpty() ? _T("ʧ") : _T("ɹ"), 
    resultIP, latency);
    
    // Ӧ
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

// ¼־
void CDNSServerCore::LogQuery(const CString& domain, const CString& type,
const CString& result, const CString& ip, int latency)
{
    if (m_logCallback) {
  m_logCallback(domain, type, result, ip, latency);
    }
}

// ߳
DWORD WINAPI CDNSServerCore::ServerThread(LPVOID lpParam)
{
    CDNSServerCore* pServer = (CDNSServerCore*)lpParam;
    
    TRACE(_T("[ServerThread] DNS߳\n"));
    TRACE(_T("[ServerThread] Socket: %d, Port: %d\n"), pServer->m_socket, pServer->m_nPort);
    
    char buffer[512];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    
    int loopCount = 0;
    int errorCount = 0;

    while (pServer->m_bRunning) {
        loopCount++;
      if (loopCount % 1000 == 0) {
   TRACE(_T("[ServerThread] ȴDNSѯ... (ѭ %d ,  %d )\n"), 
          loopCount, errorCount);
    }
  
        // ÿͻ˵ַ
  clientAddrLen = sizeof(clientAddr);
  memset(&clientAddr, 0, sizeof(clientAddr));
        
        // DNSѯ
  int recvLen = recvfrom(pServer->m_socket, buffer, sizeof(buffer), 0,
       (sockaddr*)&clientAddr, &clientAddrLen);
  
 if (recvLen > 0) {
      char clientIP[INET_ADDRSTRLEN];
       inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
   
 TRACE(_T("[ServerThread]  յDNSѯ\n"));
      TRACE(_T("[ServerThread] : %d ֽ\n"), recvLen);
      TRACE(_T("[ServerThread] : %S:%d\n"), clientIP, ntohs(clientAddr.sin_port));
       
   // ӡǰ16ֽڣʮƣ
            TRACE(_T("[ServerThread] : "));
        for (int i = 0; i < min(16, recvLen); i++) {
       TRACE(_T("%02X "), (unsigned char)buffer[i]);
    }
 TRACE(_T("\n"));
            
          // ѯ
   pServer->HandleDNSQuery(buffer, recvLen, clientAddr);
        }
 else if (recvLen == SOCKET_ERROR) {
       int error = WSAGetLastError();
          if (error == WSAEWOULDBLOCK) {
 // ģʽûݣһ±CPU 100%
     Sleep(10);
  }
      else if (error != WSAETIMEDOUT) {
       errorCount++;
       TRACE(_T("[ServerThread] ? մ: %d (ܴ: %d)\n"), error, errorCount);
           if (errorCount > 10) {
    TRACE(_T("[ServerThread] ࣬߳˳\n"));
      break;
      }
          }
    }
        else if (recvLen == 0) {
     TRACE(_T("[ServerThread] recvfrom0ӹرգ\n"));
  }
    }
 
    TRACE(_T("[ServerThread] DNS߳˳\n"));
    return 0;
}

// 
BOOL CDNSServerCore::Start()
{
  if (m_bRunning) {
     TRACE(_T("[Start] DNS\n"));
        return FALSE;  // Ѿ
    }
    
    TRACE(_T("[Start] ʼDNS...\n"));
    
    // ʼWinsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
  TRACE(_T("[Start] ? WSAStartupʧ\n"));
        return FALSE;
  }
    
    TRACE(_T("[Start] ? Winsockʼɹ\n"));
    
    // UDP Socket
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
     int error = WSAGetLastError();
   TRACE(_T("[Start] ? Socketʧܣ: %d\n"), error);
        WSACleanup();
        return FALSE;
    }
    
 TRACE(_T("[Start] ? Socketɹ: %d\n"), m_socket);
    
    // ַãҪ
    int reuse = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
    TRACE(_T("[Start] : SO_REUSEADDRʧ\n"));
    }
    
    // Ϊģʽ
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
  TRACE(_T("[Start] ? ÷ģʽʧ\n"));
    }
  TRACE(_T("[Start] ? SocketΪģʽ\n"));
    
    // 󶨶˿
    sockaddr_in serverAddr;
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // нӿڣ127.0.0.1ʵIP
    serverAddr.sin_port = htons(m_nPort);
    
    TRACE(_T("[Start] ԰󶨶˿ %d (0.0.0.0:%d)...\n"), m_nPort, m_nPort);
 
    if (bind(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
   TRACE(_T("[Start] ? 󶨶˿ %d ʧܣ: %d\n"), m_nPort, error);
        
     CString errorMsg;
   errorMsg.Format(_T("޷󶨶˿ %d\n\n: %d\n\n"), m_nPort, error);
        
     if (error == WSAEACCES || error == 10013) {
          errorMsg += _T("ԭҪԱȨ\n\n");
         errorMsg += _T("\n");
      errorMsg += _T("1. Ҽѡ\"ԹԱ\"\n");
        } else if (error == WSAEADDRINUSE || error == 10048) {
    errorMsg += _T("ԭ򣺶˿ѱռãWindows DNSͻ˷\n\n");
   errorMsg += _T("\n");
   errorMsg += _T("1. ԹԱCMD\n");
   errorMsg += _T("2. ִ: net stop dnscache\n");
 errorMsg += _T("3. \n");
        }
     
  AfxMessageBox(errorMsg, MB_OK | MB_ICONERROR);
        
   closesocket(m_socket);
   m_socket = INVALID_SOCKET;
      WSACleanup();
 return FALSE;
    }
    
    TRACE(_T("[Start] ? ɹ󶨶˿ %d\n"), m_nPort);

    // ߳
    m_bRunning = TRUE;
    TRACE(_T("[Start] ߳...\n"));
  m_hThread = CreateThread(NULL, 0, ServerThread, this, 0, NULL);
    
    if (m_hThread == NULL) {
   TRACE(_T("[Start] ? ߳ʧ\n"));
        m_bRunning = FALSE;
closesocket(m_socket);
 m_socket = INVALID_SOCKET;
  WSACleanup();
   return FALSE;
    }
    
    TRACE(_T("[Start] ? ̴߳ɹ\n"));
    TRACE(_T("[Start] ========================================\n"));
    TRACE(_T("[Start] DNSɹ\n"));
    TRACE(_T("[Start] ַ: 0.0.0.0:%d (ӿ)\n"), m_nPort);
    TRACE(_T("[Start] ========================================\n"));
    TRACE(_T("[Start] Ҫʾ\n"));
    TRACE(_T("[Start] 1. ֹͣWindows DNSͻ˷net stop dnscache\n"));
    TRACE(_T("[Start] 2. Ȼʹ nslookup www.baidu.com 127.0.0.1 \n"));
    TRACE(_T("[Start] ========================================\n"));
    
    
    // === TCP 53 listener (added) ===
    if (g_tcpListenSock == INVALID_SOCKET) {
        g_tcpListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (g_tcpListenSock != INVALID_SOCKET) {
            u_long nb = 1; ioctlsocket(g_tcpListenSock, FIONBIO, &nb);
            sockaddr_in taddr; memset(&taddr,0,sizeof(taddr));
            taddr.sin_family = AF_INET; taddr.sin_port = htons((u_short)m_nPort); taddr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(g_tcpListenSock, (sockaddr*)&taddr, sizeof(taddr)) == SOCKET_ERROR ||
                listen(g_tcpListenSock, SOMAXCONN) == SOCKET_ERROR) {
                closesocket(g_tcpListenSock); g_tcpListenSock = INVALID_SOCKET;
            } else {
                if (!g_tcpThread) {
                    g_tcpThread = (HANDLE)_beginthreadex(nullptr, 0, &TcpAcceptThread, this, 0, nullptr);
                }
            }
        }
    }
return TRUE;
}

// ֹͣ
void CDNSServerCore::Stop()
{
    if (!m_bRunning) {
        
    // === TCP 53 shutdown (added) ===
    if (g_tcpListenSock != INVALID_SOCKET) { closesocket(g_tcpListenSock); g_tcpListenSock = INVALID_SOCKET; }
    if (g_tcpThread) { WaitForSingleObject(g_tcpThread, 2000); CloseHandle(g_tcpThread); g_tcpThread = NULL; }
return;
    }
    
    TRACE(_T("ֹͣDNS...\n"));
    
    m_bRunning = FALSE;
    
    // ȴ߳˳
    if (m_hThread != NULL) {
        WaitForSingleObject(m_hThread, 5000);
        CloseHandle(m_hThread);
   m_hThread = NULL;
    }
    
    // رSocket
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
 m_socket = INVALID_SOCKET;
    }
    
    WSACleanup();
    
    TRACE(_T("DNSֹͣ\n"));
}
