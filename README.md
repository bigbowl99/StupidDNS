# StupidDNS - 智能DNS解析工具

## 项目简介

StupidDNS 是一个简洁、智能的 DNS 解析工具，旨在对抗 DNS 污染，提供更快速、更可靠的域名解析服务。

## 主要特性

### ?? 简洁易用
- **零配置启动**：一键启动 DNS 服务，无需复杂设置
- **自动测速**：自动测试并选择最快的 DNS 服务器
- **统计信息**：实时显示查询统计、缓存命中率、运行时长

### ?? 智能分流
- **国内/海外分流**：自动识别域名类型，选择最优 DNS
- **DNS 污染检测**：智能过滤被污染的 DNS 响应
- **多服务器并发**：同时查询多个 DNS 服务器，取最快响应

### ?? 高级功能
- **DNS 缓存**：减少重复查询，提升响应速度
- **系统 DNS 集成**：一键设置为系统 DNS
- **日志管理**：实时查看查询日志，支持导出

## 系统要求

- **操作系统**：Windows 7 及以上版本
- **权限**：部分功能需要管理员权限（修改系统 DNS）
- **.NET Framework**：无需安装，原生 MFC 应用

## 使用说明

### 快速开始

1. **启动程序**（建议以管理员身份运行）
2. **设置系统 DNS**
   - 点击"设为系统DNS"按钮
   - 选择"是"将 DNS 设为 127.0.0.1
3. **启动服务**
   - 点击"启动服务"按钮
   - 等待 DNS 服务器初始化完成
4. **开始使用**
   - 正常浏览网页，程序会自动处理 DNS 查询

### DNS 服务器管理

1. 点击"DNS服务器"按钮
2. 点击"全部测速"测试所有预置 DNS
3. 点击"应用"自动选择最快的 5 个 DNS

预置 DNS 服务器包括：
- 香港：HKNet, PCCW
- 台湾：HiNet, SeedNet
- 日本：OpenDNS, OCN
- 新加坡：SingNet, StarHub
- 韩国：KT, SK
- 国际：Google DNS, Cloudflare, OpenDNS

### 高级选项

- **DNS 缓存**：设置缓存大小和有效期
- **查询超时**：调整 DNS 查询超时时间
- **并发查询**：设置同时查询的 DNS 数量
- **日志设置**：配置文件日志和日志级别
- **系统设置**：开机自启、最小化到托盘

## 技术架构

### 核心模块
- **DNSServerCore**：DNS 服务器核心引擎
- **DNSServerManager**：DNS 服务器管理（测速、选择）
- **DomainClassifier**：域名分类器（国内/海外/GFW）
- **DNSQueryRouter**：查询路由器（智能分流）
- **SystemDNSHelper**：系统 DNS 设置辅助

### 技术特点
- 使用 **MFC** 框架开发，原生 Windows 应用
- **ICMP Ping** 测速，精确测量 DNS 延迟
- **异步查询**，多线程并发处理
- **内存缓存**，LRU 算法管理缓存条目

## 编译说明

### 开发环境
- **Visual Studio 2019/2022**
- **Windows SDK 10**
- **C++14 标准**

### 编译步骤
```bash
# 1. 克隆仓库
git clone https://github.com/yourname/StupidDNS.git
cd StupidDNS

# 2. 打开解决方案
# 使用 Visual Studio 打开 StupidDNS.sln

# 3. 编译
# 选择 Release 配置，生成解决方案
```

### 依赖库
- **iphlpapi.lib**：网络适配器管理
- **ws2_32.lib**：Winsock 网络编程

## 项目结构

```
StupidDNS/
├── StupidDNS/
│   ├── images/   # 图标资源
│   │   └── logo.ico
│   ├── res/       # 其他资源
│   ├── *.h / *.cpp   # 源代码文件
│   ├── StupidDNS.rc         # 资源文件
│   ├── Resource.h  # 资源头文件
│ └── StupidDNS.vcxproj    # 项目文件
├── .gitignore
├── README.md
└── LICENSE
```

## 版本历史

### v1.0.0 (2025-01-24)
- ? 首个正式版本发布
- ? 核心 DNS 解析功能
- ? 自动测速和 DNS 选择
- ? 系统 DNS 集成
- ? 实时日志和统计
- ? 简洁的用户界面

## 常见问题

### Q: 为什么需要管理员权限？
A: 修改系统 DNS 设置、绑定 53 端口需要管理员权限。

### Q: 如何恢复默认 DNS？
A: 点击"设为系统DNS"，选择"否"即可恢复为自动获取。

### Q: 程序占用多少资源？
A: 内存占用约 10-20MB，CPU 占用极低（< 1%）。

### Q: 支持 IPv6 吗？
A: 当前版本仅支持 IPv4，IPv6 支持计划在未来版本中加入。

## 贡献指南

欢迎提交 Issue 和 Pull Request！

### 提交 Issue
- 清晰描述问题或建议
- 附上系统信息和日志（如有）
- 提供复现步骤

### Pull Request
- Fork 本仓库
- 创建特性分支 (`git checkout -b feature/AmazingFeature`)
- 提交更改 (`git commit -m 'Add some AmazingFeature'`)
- 推送到分支 (`git push origin feature/AmazingFeature`)
- 创建 Pull Request

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 致谢

- 感谢所有 DNS 服务提供商
- 感谢开源社区的支持

## 联系方式

- **GitHub**: https://github.com/yourname/stupiddns
- **Email**: your.email@example.com

---

**?? 免责声明**

本工具仅供学习和研究使用，使用者需自行承担使用本工具的风险。
开发者不对因使用本工具而导致的任何直接或间接损失负责。

---

Made with ?? by StupidDNS Team
