@echo off

set BUILD_PATH=%~dp0build
if not exist %BUILD_PATH% mkdir %BUILD_PATH%

cd %BUILD_PATH%
cmake ../ -G "Visual Studio 14 Win64"

pause
