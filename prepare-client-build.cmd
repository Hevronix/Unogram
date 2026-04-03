@echo off
setlocal EnableDelayedExpansion

set "VS_DEV_CMD=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
set "NINJA_DIR=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "PYTHON_DIR=C:\Users\maksw\AppData\Local\Programs\Python\Python312"
set "GIT_BIN_DIR=C:\Program Files\Git\cmd"

if not exist "%VS_DEV_CMD%" (
	echo Missing VsDevCmd.bat at "%VS_DEV_CMD%".
	exit /b 1
)

if not exist "%PYTHON_DIR%\python.exe" (
	echo Missing python.exe at "%PYTHON_DIR%\python.exe".
	exit /b 1
)

call "%VS_DEV_CMD%" -arch=x64 -host_arch=x64 -vcvars_ver=14.44
if errorlevel 1 exit /b %errorlevel%

set "Platform=x64"
set "PATH=%NINJA_DIR%;%PYTHON_DIR%;%GIT_BIN_DIR%;!PATH!"
set "UNOGRAM_SKIP_BREAKPAD=1"
set "GIT_CONFIG_COUNT=2"
set "GIT_CONFIG_KEY_0=http.sslBackend"
set "GIT_CONFIG_VALUE_0=openssl"
set "GIT_CONFIG_KEY_1=http.version"
set "GIT_CONFIG_VALUE_1=HTTP/1.1"

for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"
if errorlevel 1 exit /b %errorlevel%

"%PYTHON_DIR%\python.exe" client\Telegram\build\prepare\prepare.py silent
exit /b %errorlevel%
