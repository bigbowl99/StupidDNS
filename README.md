# StupidDNS

> 运行在 Windows 上的智能 DNS 代理工具，自动将国内域名路由到国内 DNS、将被污染 / GFW 域名路由到海外 DNS，彻底解决 DNS 污染问题。

---

## ✨ 功能特性

- **智能分流**：根据域名规则自动判断国内 / 国外，分别使用对应的上游 DNS 服务器
- **DNS 污染检测**：内置污染 IP 规则库，自动识别并丢弃被污染的响应
- **GFW 域名识别**：维护 GFW 域名列表，强制使用海外 DNS 解析
- **响应缓存**：内置 DNS 响应缓存，加快重复查询速度，可配置缓存大小与 TTL
- **自动测速**：启动时在后台对所有上游 DNS 测速，自动选择延迟最低的服务器
- **双栈支持**：同时监听 IPv4 / IPv6，支持 UDP 和 TCP 协议
- **系统 DNS 接管**：一键将所有网卡的 DNS 设置为 `127.0.0.1`，退出时自动恢复
- **GitHub Hosts 智能更新**：通过小众海外 DNS + TCP 连通测试 + `curl --resolve` 直连，绕过 DNS 污染拉取最新 GitHub Hosts 并写入系统
- **系统托盘**：关闭窗口后最小化到托盘，支持开机自启动
- **实时日志**：彩色日志列表，按类型（国内 / 国外 / 缓存 / 错误）着色显示

---

## 🖥️ 系统要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10 / 11（x64） |
| 权限 | **管理员权限**（绑定 53 端口、修改系统 DNS 需要） |
| 其他依赖 | 无（`curl.exe` 为 Windows 10 系统自带） |

---

## 🚀 快速开始

1. 从 [Releases](../../releases) 下载 `StupidDNS.zip` 并解压
2. 右键 `StupidDNS.exe` → **以管理员身份运行**
3. 点击 **▶ 启动**，默认监听 `53` 端口
4. 点击 **设为系统 DNS**，将所有网卡 DNS 指向 `127.0.0.1`

> 如果 53 端口被占用，可在端口框修改为其他端口（如 `5353`），但「设为系统 DNS」功能将不可用。

---

## 📁 规则文件

解压后 `rules/` 目录中包含以下规则文件，程序启动时自动加载：

| 文件 | 说明 |
|------|------|
| `rules/accelerated-domains.china.conf` | 国内域名列表（走国内 DNS） |
| `rules/bogus-nxdomain.china.conf` | 污染 IP 列表（丢弃含这些 IP 的响应） |
| `rules/gfw.txt` | GFW 域名列表（强制走海外 DNS） |

规则来源：[dnsmasq-china-list](https://github.com/felixonmars/dnsmasq-china-list)

---

## 🌐 上游 DNS

点击 **🌐 上游DNS** 查看所有预置服务器的延迟，或手动触发测速。测速结果缓存 24 小时。

| 类型 | 地址 | 服务商 |
|------|------|--------|
| 国内 | `223.5.5.5` | 阿里 DNS |
| 国内 | `119.29.29.29` | 腾讯 DNS |
| 国内 | `114.114.114.114` | 114 DNS |
| 海外 | `185.222.222.222` | DNS.SB（香港节点） |
| 海外 | `94.140.14.14` | AdGuard DNS |
| 海外 | `77.88.8.8` | Yandex DNS |

> 刻意不使用 Cloudflare（`1.1.1.1`）和 Google（`8.8.8.8`），这两个在中国大陆 UDP 53 端口被污染或阻断。

---

## 🔧 GitHub Hosts 智能更新

针对 GitHub 在中国大陆图片加载失败、访问缓慢的问题，点击 **更新 GitHub Hosts** 自动执行：

1. 用小众海外 DNS 解析 `raw.githubusercontent.com`，获取未被污染的真实 IP
2. 对所有 IP 进行 TCP 443 连通性测试，选出第一个可达的 IP
3. 调用系统自带 `curl.exe --resolve` 将域名绑定到可达 IP，正常发起 HTTPS 请求拉取最新 hosts 文件
4. 过滤掉系统 hosts 中所有旧的 GitHub 条目，写入新内容，刷新 DNS 缓存

> hosts 数据来源：[ittuann/GitHub-IP-hosts](https://github.com/ittuann/GitHub-IP-hosts)

---

## ⚠️ 注意事项

- 程序需要**管理员权限**才能绑定 53 端口和修改系统 DNS
- 退出程序时会**自动恢复**系统 DNS 为自动获取，防止断网
- GitHub Hosts 更新功能需要网络能访问海外 DNS 服务器（UDP 53 端口）

---

## 📄 License

本软件免费使用，禁止二次分发或修改。

---

## 🙏 致谢

- [dnsmasq-china-list](https://github.com/felixonmars/dnsmasq-china-list) — 国内域名 & 污染 IP 规则
- [ittuann/GitHub-IP-hosts](https://github.com/ittuann/GitHub-IP-hosts) — GitHub Hosts 数据
- [DNS.SB](https://dns.sb) — 香港节点海外 DNS

