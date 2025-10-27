// DNSServerCore.h: DNS服务器核心类
//

#pragma once

#include "DNSQueryRouter.h"
#include "DomainClassifier.h"
#include "DNSServerManager.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// DNS服务器核心类
class CDNSServerCore {
public:
    CDNSServerCore();
    ~CDNSServerCore();
    
    // 初始化
    BOOL Initialize(int port, 
   CDNSServerManager* pServerManager,
        CDomainClassifier* pClassifier);
  
    // 启动服务器
    BOOL Start();
  
    // 停止服务器
  void Stop();
    
    // 检查是否正在运行
    BOOL IsRunning() const { return m_bRunning; }
    
  // 设置日志回调
    void SetLogCallback(std::function<void(CString, CString, CString, CString, int)> callback) {
        m_logCallback = callback;
    }
    
    // 获取统计信息
    int GetTotalQueries() const { return m_nTotalQueries; }
    int GetDomesticQueries() const { return m_nDomesticQueries; }
    int GetForeignQueries() const { return m_nForeignQueries; }
    
private:
    int m_nPort;             // 监听端口
    SOCKET m_socket;            // UDP Socket
    SOCKET m_tcpListenSocket; // TCP监听 Socket
    BOOL m_bRunning;         // 运行状态
    HANDLE m_hThread;   // 工作线程句柄
    HANDLE m_hTcpThread; // TCP线程句柄
 
    CDNSQueryRouter m_queryRouter;  // DNS查询路由器
    CDNSServerManager* m_pServerManager;
    CDomainClassifier* m_pClassifier;
    
    // 统计信息
    int m_nTotalQueries;
    int m_nDomesticQueries;
    int m_nForeignQueries;
    
    // 日志回调
    std::function<void(CString domain, CString type, CString result, CString ip, int latency)> m_logCallback;
    
    // 工作线程
    static DWORD WINAPI ServerThread(LPVOID lpParam);
    static DWORD WINAPI ServerTcpThread(LPVOID lpParam);
    
    // 处理DNS查询
    void HandleDNSQuery(const char* queryPacket, int queryLen,
         const sockaddr_in& clientAddr);
    void HandleDNSQueryTCP(const char* queryPacket, int queryLen, SOCKET clientSock);
  
    // 从DNS查询包中提取域名
    CString ExtractDomainFromQuery(const char* queryPacket, int queryLen);
 
    // 构造DNS响应包
    int BuildDNSResponse(const char* queryPacket, int queryLen,
   const CString& ip, char* responseBuffer, int bufferSize);

    // 构造SERVFAIL响应
    int BuildServFailResponse(const char* queryPacket, int queryLen,
 char* responseBuffer, int bufferSize);
 
    // 转发DNS查询到上游服务器
    BOOL ForwardQuery(const CString& dnsServer, const char* queryPacket, 
   int queryLen, char* outResponse, int outResponseSize, int& outResponseLen,
 CString& outIP, int& outLatency);

    // 转发DNS查询并返回原始响应（TCP 上游）
    BOOL ForwardQueryTCP(const CString& dnsServer, const char* queryPacket,
 int queryLen, char* outResponse, int outResponseSize, int& outResponseLen,
 CString& outIP, int& outLatency);
    
    // IP字符串转为4字节
    BOOL IPStringToBytes(const CString& ip, unsigned char* bytes);
    
    // 记录日志
    void LogQuery(const CString& domain, const CString& type,
     const CString& result, const CString& ip, int latency);
};
