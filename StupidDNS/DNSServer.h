// DNSServer.h: DNS服务器数据结构定义
//

#pragma once

#include <afxwin.h>

// DNS服务器信息结构
struct DNSServerInfo {
    CString name;    // 服务器名称，如 "阿里DNS"
    CString ip;             // IP地址，如 "223.5.5.5"
    CString location;       // 地理位置，如 "中国", "美国", "欧洲"
    CString provider;       // 提供商，如 "Alibaba", "Cloudflare"
    
    int latency;  // 延迟(毫秒)，-1表示未测试或失败
    double reliability;     // 可靠性(0-100%)
    BOOL isActive;      // 是否启用
    BOOL isOnline;          // 是否在线
    
    DWORD lastTestTime;     // 最后测试时间
    int failCount;     // 连续失败次数
    
    DNSServerInfo() {
    latency = -1;
        reliability = 0.0;
      isActive = FALSE;
        isOnline = FALSE;
        lastTestTime = 0;
  failCount = 0;
    }
};

// 测速结果结构
struct SpeedTestResult {
    CString serverIP;
    int avgLatency;      // 平均延迟
    int minLatency;   // 最小延迟
    int maxLatency;      // 最大延迟
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
