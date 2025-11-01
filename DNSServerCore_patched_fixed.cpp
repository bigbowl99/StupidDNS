// DNSServerCore_patched_fixed.cpp
// Patch: rename helper funcs to avoid symbol/macro conflicts; fix TCP thread; avoid aliasing on ntohs; ensure 3-arg calls.
// This is a drop-in replacement for DNSServerCore_patched.cpp in your project.

#include "pch.h"
#include "DNSServerCore.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <vector>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

// === TCP helpers & globals (renamed to avoid conflicts) ===
static bool DNS_SendAll(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(s, buf + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}
static bool DNS_RecvAll(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len) {
        int n = recv(s, buf + got, len - got, 0);
        if (n <= 0) return false;
        got += n;
    }
    return true;
}
static bool DNS_SendTcpMessage(SOCKET s, const char* payload, int payloadLen) {
    if (payloadLen < 0 || payloadLen > 65535) return false;
    uint16_t be = htons((uint16_t)payloadLen);
    return DNS_SendAll(s, (const char*)&be, 2) && DNS_SendAll(s, payload, payloadLen);
}
static int DNS_BuildServfailFromQuery(const char* q, int qlen, char* out, int outCap) {
    if (qlen < 12 || outCap < qlen) return -1;
    memcpy(out, q, qlen);
    unsigned char* f1 = (unsigned char*)(out + 2);
    unsigned char* f2 = (unsigned char*)(out + 3);
    unsigned char qr = 0x80;                    // QR = 1
    unsigned char opcode = (*f1) & 0x78;        // keep OPCODE
    unsigned char rd = (*f1) & 0x01;            // keep RD
    *f1 = (unsigned char)(qr | opcode | rd);
    unsigned char ra = 0x80;                    // RA = 1
    unsigned char rcode = 0x02;                 // SERVFAIL
    *f2 = (unsigned char)(ra | rcode);
    out[6]=out[7]=0; out[8]=out[9]=0; out[10]=out[11]=0;
    return qlen;
}

static SOCKET g_tcpListenSock = INVALID_SOCKET;
static HANDLE g_tcpThread = NULL;
static unsigned __stdcall TcpAcceptThread(void* ctx);

// === original ctor/dtor & existing methods are kept as-is in your project ===
// We only inject Start/Stop additions and the TcpAcceptThread impl here.

// ---- Start() addition: ensure TCP listener is created and thread launched ----
BOOL CDNSServerCore::Start()
{
    if (m_bRunning) {
        TRACE(_T("[Start] DNS服务器已在运行\n"));
        return FALSE;
    }
    TRACE(_T("[Start] 开始启动DNS服务器...\n"));

    // --- original UDP startup path (unchanged) ---
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        TRACE(_T("[Start] WSAStartup 失败\n"));
        return FALSE;
    }
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        TRACE(_T("[Start] UDP socket 创建失败: %d\n"), WSAGetLastError());
        WSACleanup();
        return FALSE;
    }
    u_long nb = 1;
    ioctlsocket(m_socket, FIONBIO, &nb);
    sockaddr_in saddr{}; saddr.sin_family = AF_INET; saddr.sin_addr.s_addr = INADDR_ANY; saddr.sin_port = htons(m_nPort);
    if (bind(m_socket, (sockaddr*)&saddr, sizeof(saddr)) == SOCKET_ERROR) {
        TRACE(_T("[Start] UDP 绑定失败: %d\n"), WSAGetLastError());
        closesocket(m_socket); m_socket = INVALID_SOCKET; WSACleanup();
        return FALSE;
    }
    m_bRunning = TRUE;
    m_hThread = CreateThread(NULL, 0, ServerThread, this, 0, NULL);
    if (!m_hThread) {
        TRACE(_T("[Start] UDP线程创建失败\n"));
        m_bRunning = FALSE; closesocket(m_socket); m_socket = INVALID_SOCKET; WSACleanup();
        return FALSE;
    }
    TRACE(_T("[Start] UDP 监听 0.0.0.0:%d 已启动\n"), m_nPort);

    // --- NEW: TCP 53 listener ---
    if (g_tcpListenSock == INVALID_SOCKET) {
        g_tcpListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (g_tcpListenSock == INVALID_SOCKET) {
            TRACE(_T("[Start][TCP] socket 失败: %d\n"), WSAGetLastError());
        } else {
            u_long nb2 = 1; ioctlsocket(g_tcpListenSock, FIONBIO, &nb2);
            sockaddr_in taddr{}; taddr.sin_family = AF_INET; taddr.sin_addr.s_addr = htonl(INADDR_ANY); taddr.sin_port = htons((u_short)m_nPort);
            if (bind(g_tcpListenSock, (sockaddr*)&taddr, sizeof(taddr)) == SOCKET_ERROR ||
                listen(g_tcpListenSock, SOMAXCONN) == SOCKET_ERROR) {
                TRACE(_T("[Start][TCP] bind/listen 失败: %d\n"), WSAGetLastError());
                closesocket(g_tcpListenSock); g_tcpListenSock = INVALID_SOCKET;
            } else {
                TRACE(_T("[Start][TCP] 监听 0.0.0.0:%d (TCP)\n"), m_nPort);
                if (!g_tcpThread) {
                    unsigned tid = 0;
                    g_tcpThread = (HANDLE)_beginthreadex(nullptr, 0, TcpAcceptThread, this, 0, &tid);
                    TRACE(_T("[Start][TCP] accept 线程启动, tid=%u\n"), tid);
                }
            }
        }
    }
    return TRUE;
}

// ---- Stop(): add TCP shutdown ----
void CDNSServerCore::Stop()
{
    if (!m_bRunning) {
        // also close TCP resources if any remain
        if (g_tcpListenSock != INVALID_SOCKET) { closesocket(g_tcpListenSock); g_tcpListenSock = INVALID_SOCKET; }
        if (g_tcpThread) { WaitForSingleObject(g_tcpThread, 2000); CloseHandle(g_tcpThread); g_tcpThread = NULL; }
        return;
    }
    m_bRunning = FALSE;

    if (m_hThread) { WaitForSingleObject(m_hThread, 5000); CloseHandle(m_hThread); m_hThread = NULL; }
    if (m_socket != INVALID_SOCKET) { closesocket(m_socket); m_socket = INVALID_SOCKET; }

    if (g_tcpListenSock != INVALID_SOCKET) { closesocket(g_tcpListenSock); g_tcpListenSock = INVALID_SOCKET; }
    if (g_tcpThread) { WaitForSingleObject(g_tcpThread, 2000); CloseHandle(g_tcpThread); g_tcpThread = NULL; }

    WSACleanup();
    TRACE(_T("DNS服务器已停止\n"));
}

// ---- TcpAcceptThread: read framed query, forward via TCP upstream, reply or SERVFAIL ----
static unsigned __stdcall TcpAcceptThread(void* ctx)
{
    CDNSServerCore* self = (CDNSServerCore*)ctx;
    const char* upstream = "8.8.8.8"; // TODO: hook your strategy here

    while (self->m_bRunning) {
        sockaddr_in caddr{}; int alen = sizeof(caddr);
        SOCKET cs = accept(g_tcpListenSock, (sockaddr*)&caddr, &alen);
        if (cs == INVALID_SOCKET) { Sleep(10); continue; }

        // one-query-per-connection (simple and robust)
        char lenBuf[2];
        int n = recv(cs, lenBuf, 2, MSG_WAITALL);
        if (n != 2) { closesocket(cs); continue; }
        uint16_t mlen_be; memcpy(&mlen_be, lenBuf, 2);
        uint16_t mlen = ntohs(mlen_be);
        if (mlen == 0 || mlen > 4096) { closesocket(cs); continue; }

        std::vector<char> q(mlen);
        if (!DNS_RecvAll(cs, q.data(), (int)q.size())) { closesocket(cs); continue; }

        // forward to upstream over TCP
        bool ok = false;
        SOCKET us = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (us != INVALID_SOCKET) {
            sockaddr_in da{}; da.sin_family = AF_INET; da.sin_port = htons(53);
            InetPtonA(AF_INET, upstream, &da.sin_addr);
            if (connect(us, (sockaddr*)&da, sizeof(da)) == 0) {
                uint16_t qlen_be = htons((uint16_t)q.size());
                if (DNS_SendAll(us, (const char*)&qlen_be, 2) &&
                    DNS_SendAll(us, q.data(), (int)q.size())) {
                    char rlenBuf[2];
                    int r = recv(us, rlenBuf, 2, MSG_WAITALL);
                    if (r == 2) {
                        uint16_t rbe; memcpy(&rbe, rlenBuf, 2);
                        uint16_t rlen = ntohs(rbe);
                        if (rlen > 0 && rlen <= 8192) {
                            std::vector<char> resp(rlen);
                            if (DNS_RecvAll(us, resp.data(), (int)resp.size())) {
                                (void)DNS_SendTcpMessage(cs, resp.data(), (int)resp.size());
                                ok = true;
                            }
                        }
                    }
                }
            }
            closesocket(us);
        }
        if (!ok) {
            char fail[2048];
            int fl = DNS_BuildServfailFromQuery(q.data(), (int)q.size(), fail, sizeof(fail));
            if (fl > 0) (void)DNS_SendTcpMessage(cs, fail, fl);
        }
        closesocket(cs);
    }
    return 0u;
}
