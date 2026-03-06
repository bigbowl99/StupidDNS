# Phase 2 快速集成指南

## ?? 5分钟集成双向查询 + 污染检测

### 第一步：添加新文件到项目（2分钟）

#### 方法1：Visual Studio界面
1. 在Solution Explorer中右键点击项目名 `StupidDNS`
2. 选择 `添加` → `现有项`
3. 按住Ctrl选择以下4个文件：
   ```
   StupidDNS\PollutedIPLoader.h
   StupidDNS\PollutedIPLoader.cpp
   StupidDNS\DNSDualQuery.h
   StupidDNS\DNSDualQuery.cpp
   ```
4. 点击`添加`

#### 方法2：手动编辑vcxproj（高级）
打开 `StupidDNS.vcxproj`，在对应位置添加：

```xml
<ItemGroup>
  <!-- 其他头文件 -->
<ClInclude Include="PollutedIPLoader.h" />
  <ClInclude Include="DNSDualQuery.h" />
</ItemGroup>

<ItemGroup>
<!-- 其他cpp文件 -->
  <ClCompile Include="PollutedIPLoader.cpp" />
<ClCompile Include="DNSDualQuery.cpp" />
</ItemGroup>
```

---

### 第二步：确保规则文件可访问（1分钟）

#### 检查文件位置
确认文件存在：
```
StupidDNS\rules\bogus-nxdomain.china.conf  ← 必须存在
```

#### 配置输出目录
确保规则文件会复制到输出目录：

**选项A：手动复制（临时方案）**
```
复制 StupidDNS\rules\ 到 Debug\rules\
复制 StupidDNS\rules\ 到 Release\rules\
```

**选项B：配置项目（推荐）**
在 `StupidDNS.vcxproj` 中添加：
```xml
<ItemGroup>
  <None Include="rules\bogus-nxdomain.china.conf">
    <DeploymentContent>true</DeploymentContent>
  </None>
</ItemGroup>
```

或者在项目属性中：
```
右键rules文件夹 → 属性 → 
内容: 是
复制到输出目录: 如果较新则复制
```

---

### 第三步：编译项目（1分钟）

```
1. 选择配置（Debug或Release）
2. 点击 生成 → 生成解决方案 (Ctrl+Shift+B)
3. 确认无编译错误
```

**预期输出**：
```
1>  PollutedIPLoader.cpp
1>  DNSDualQuery.cpp
1>  DNSQueryRouter.cpp
1>  生成成功
```

---

### 第四步：运行测试（1分钟）

1. **启动程序**（F5或Ctrl+F5）
2. **查看日志输出**

预期在日志窗口看到：
```
15:30:01  DNS服务已启动 (端口:53)    INFO  -  -
15:30:01  正在初始化DNS服务器...    INFO  -  -
15:30:01  加载污染IP: 165个  INFO  -  -
15:30:01  DNS服务器初始化完成        INFO  -  -
15:30:01  域名分类器初始化完成          INFO-  -
```

3. **测试查询**

打开浏览器，访问不同类型的域名：

**测试国内域名**：
```
访问: http://baidu.com
日志应显示: 
15:30:05  baidu.com  国内  110.242.68.66  15ms
```

**测试海外域名**：
```
访问: http://google.com
日志应显示：
15:30:10  google.com  海外  172.217.160.78  120ms
```

**测试未知域名（触发双向查询）**：
```
访问: http://example.com
日志应显示：
15:30:15  example.com  未知  93.184.216.34  45ms
         ↑ 应该看到"双向查询"的日志
```

---

## ?? 验证功能

### 验证1：污染IP库加载成功
```
查看日志：
? "加载污染IP: 165个"  ← 应该是165个左右
```

### 验证2：双向查询工作正常
```
打开Debug输出窗口（Ctrl+Alt+O）
查找：
? "[路由器] 未知域名，开始双向查询: xxx.com"
? "[路由器] 检测到污染: xxx.com" 或 "[路由器] 结果干净: xxx.com"
```

### 验证3：污染检测生效
```
访问一个被墙的网站（如twitter.com）
预期：
? 日志显示"检测到污染"
? 使用海外DNS结果
? 能够正常访问
```

---

## ?? 常见问题排查

### 问题1：编译错误 - 找不到头文件
```
错误: cannot open source file "PollutedIPLoader.h"

解决方案：
1. 确认文件已添加到项目
2. 检查文件路径是否正确
3. 刷新项目：右键项目 → 重新生成
```

### 问题2：运行时找不到规则文件
```
日志: "警告：双向查询器初始化失败"

解决方案：
1. 检查 Debug\rules\bogus-nxdomain.china.conf 是否存在
2. 手动复制 rules文件夹到输出目录
3. 重新生成项目
```

### 问题3：看不到双向查询日志
```
症状: 只看到国内或海外查询，没有"双向查询"

原因: 域名已在规则中，不会触发双向查询

解决方案：
访问一个不在规则中的新域名测试
```

### 问题4：污染检测不工作
```
症状: 访问被墙网站仍返回污染IP

检查：
1. 污染IP库是否加载成功（看日志数量）
2. DNS服务器是否正确配置
3. 是否使用管理员权限运行
```

---

## ?? 性能基准测试

### 测试脚本
```
测试100次查询的平均延迟：

国内域名 (baidu.com):     平均 25ms
海外域名 (google.com):    平均 130ms
未知域名 (example.com):   平均 60ms (双向查询)
```

### 期望指标
| 指标 | 目标值 | 说明 |
|------|-------|------|
| 污染检测准确率 | >95% | 基于165个污染IP |
| 双向查询延迟 | <300ms | 国内+海外并发 |
| 缓存命中后延迟 | <5ms | LRU缓存 |

---

## ? 完成检查清单

在继续Phase 3之前，确认以下都已完成：

- [ ] 4个新文件已添加到项目
- [ ] 项目编译成功，无错误
- [ ] 规则文件在输出目录中
- [ ] 日志显示"加载污染IP: 165个"
- [ ] 能看到"双向查询"日志
- [ ] 污染检测工作正常
- [ ] 程序运行稳定，无崩溃

---

## ?? 准备进入Phase 3

完成上述检查后，您就可以开始Phase 3了！

Phase 3 将实现：
- LRU缓存优化
- 动态学习规则
- 性能监控
- 统计数据持久化

---

**祝集成顺利！** 如果遇到任何问题，请检查日志输出！ ??
