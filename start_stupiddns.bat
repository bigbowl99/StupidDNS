@echo off
echo ========================================
echo StupidDNS 一键启动脚本
echo ========================================
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ? 需要管理员权限！
    echo.
    echo 请右键此文件，选择"以管理员身份运行"
    pause
    exit /b 1
)

echo [1] 停止DNS Client服务...
net stop dnscache 2>nul
if errorlevel 1 (
    echo    ? 无法停止DNS Client服务
    echo.
    echo    尝试强制停止...
    sc stop dnscache
    timeout /t 2 /nobreak >nul
)

echo [2] 检查端口53占用...
netstat -ano -p UDP | findstr ":53 " >nul
if not errorlevel 1 (
    echo    ? 端口53仍被占用！
    echo.
    netstat -ano -p UDP | findstr ":53 "
    echo.
    echo请手动结束占用53端口的进程
    pause
    exit /b 1
)

echo [3] 启动StupidDNS...
echo.
start "" "%~dp0x64\Debug\StupidDNS.exe"

echo ========================================
echo ? StupidDNS已启动！
echo.
echo 关闭此窗口后，DNS Client服务将自动恢复
echo ========================================
echo.
pause

echo [4] 恢复DNS Client服务...
net start dnscache
echo.
echo ? DNS Client服务已恢复
echo.
pause
