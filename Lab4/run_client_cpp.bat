@echo off
title Client (C++)
echo Starting ClientApp (C++)...

if exist "build\Debug\ClientApp.exe" (
    "build\Debug\ClientApp.exe"
) else if exist "build\Release\ClientApp.exe" (
    "build\Release\ClientApp.exe"
) else if exist "build\ClientApp.exe" (
    "build\ClientApp.exe"
) else (
    echo ClientApp.exe not found!
)

pause