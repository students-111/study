@echo off
rem Generate configuration with Keil-bound SysConfig 1.21.1 and MSPM0 SDK 2.02.

set "SYSCFG_PATH=D:\TI\TI_sys\sysconfig_cli.bat"
set "SDK_ROOT=D:\TI\M0_SDA\mspm0_sdk_2_02_00_05"
set "PROJECT_ROOT=%~dp0.."

if not exist "%SYSCFG_PATH%" (
    echo SysConfig not found: %SYSCFG_PATH%
    exit /b 1
)

if not exist "%SDK_ROOT%\.metadata\product.json" (
    echo MSPM0 SDK product file not found: %SDK_ROOT%\.metadata\product.json
    exit /b 1
)

call "%SYSCFG_PATH%" -o "%PROJECT_ROOT%" -s "%SDK_ROOT%\.metadata\product.json" --compiler keil "%PROJECT_ROOT%\empty.syscfg"
exit /b %ERRORLEVEL%
