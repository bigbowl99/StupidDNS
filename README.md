StupidDNS - 智能DNS解析工具 v1.1.8 (IPv6 支持版)
================================
苦于SmartDNS、ChinaDNS‑NG这类产品都要求部署在openwrt上，或者Docer上，对普通用户不友好。各种C#版本体积又太大，拿出MFC做一个体积极小的Windows版本，供大家使用吧。
太久不做软件了，就不开源了，有问题我想办法改进。

【系统要求】
- Windows 7/8/10/11 (x64)
- 需要管理员权限（绑定53端口）
- IPv6支持（可选，如果系统支持IPv6则自动启用）

【快速开始】
1. 右键"以管理员身份运行" StupidDNS.exe
2. 点击"启动"按钮
3. 点击"设为系统DNS"按钮
4. 开始使用！

【新特性 - IPv6支持】
- 同时监听 IPv4 和 IPv6 端口（双栈支持）
- 支持 IPv6 DNS 服务器查询（如 2606:4700:4700::1111）
- 支持 IPv6 客户端查询（AAAA记录）
- 自动检测系统 IPv6 可用性
- 如果系统不支持IPv6，自动回退到IPv4模式

【功能说明】
- 自动区分国内/海外域名
- 智能DNS路由
- 防DNS污染
- 支持规则自定义
- IPv4/IPv6 双栈支持
- UDP 和 TCP 协议支持

【文件说明】
- StupidDNS.exe: 主程序
- rules/: 域名规则文件目录
  - accelerated-domains.china.conf: 中国域名列表
  - gfw.txt: GFW域名列表
  - bogus-nxdomain.china.conf: DNS污染IP列表
- dns_config.ini: DNS配置文件（自动生成）
- dns_cache.csv: DNS缓存文件（自动生成）

【注意事项】
- 停止服务前请先"恢复自动DNS"
- rules目录必须存在且包含规则文件
- IPv6功能需要系统网络支持IPv6

【常见问题】
Q: 无法启动服务
A: 请以管理员身份运行，并确保53端口未被占用

Q: DNS服务器测速失败
A: 点击"DNS服务器设置"手动测速

Q: IPv6不工作
A: 检查系统是否启用IPv6，运行 `ipconfig` 查看是否有IPv6地址

【技术说明】
- 使用 Winsock2 API 实现
- 支持 AF_INET 和 AF_INET6 地址族
- 使用 inet_pton/inet_ntop 进行IPv6地址转换
- 支持 AAAA (IPv6) 和 A (IPv4) 记录

