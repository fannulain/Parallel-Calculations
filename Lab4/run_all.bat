@echo off
echo Starting all applications...

start "C++ Server" cmd /c run_server.bat

timeout /t 2 /nobreak > nul

start "C++ Client" cmd /c run_client_cpp.bat
start "Python Client" cmd /c run_client_py.bat

echo All started!