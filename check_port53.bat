@echo off
echo ========================================
echo 检查端口53占用情况
echo ========================================
echo.

echo [1] 检查TCP 53端口：
netstat -ano | findstr ":53 "
echo.

echo [2] 检查UDP 53端口：
netstat -ano -p UDP | findstr ":53 "
echo.

echo [3] 检查DNS Client服务状态：
sc query dnscache
echo.

echo [4] 尝试停止DNS Client服务：
net stop dnscache
echo.

echo [5] 再次检查端口占用：
netstat -ano -p UDP | findstr ":53 "
echo.

echo ========================================
echo 完成！
echo ========================================
pause
