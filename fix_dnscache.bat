@echo off
echo ========================================
echo 修复DNS Client服务，使其可以停止
echo ========================================
echo.

echo [1] 设置DNS Client服务为手动启动：
sc config dnscache start= demand
echo.

echo [2] 启动DNS Client服务：
net start dnscache
echo.

echo [3] 现在尝试停止服务：
net stop dnscache
echo.

echo [4] 检查服务状态：
sc query dnscache
echo.

if errorlevel 1 (
    echo.
    echo ? 服务可能被组策略保护！
    echo.
    echo 解决方案：
    echo 1. 运行 gpedit.msc
    echo 2. 计算机配置 - Windows设置 - 安全设置 - 系统服务
 echo 3. 找到 DNS Client
  echo 4. 设置为"未定义"或"已禁用"
    echo.
) else (
    echo.
    echo ? 成功！现在可以停止DNS Client服务了
    echo.
    echo 下一步：
    echo 1. 运行你的StupidDNS程序
 echo 2. 点击"启动服务"
    echo.
)

echo ========================================
pause
