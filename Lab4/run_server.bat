@echo off
title Server (C++)
echo Starting ServerApp...

if exist "build\Debug\ServerApp.exe" (
    "build\Debug\ServerApp.exe"
) else if exist "build\Release\ServerApp.exe" (
    "build\Release\ServerApp.exe"
) else if exist "build\ServerApp.exe" (
    "build\ServerApp.exe"
) else (
    echo ServerApp.exe not found!
)

pause