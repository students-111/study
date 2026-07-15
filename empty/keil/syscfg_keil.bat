@echo off
rem Generate configuration with Keil-bound SysConfig and MSPM0 SDK.

set "SYSCFG_PATH=C:\ti\ccs2100\ccs\utils\sysconfig_1.28.0\sysconfig_cli.bat"
set "SDK_ROOT=C:\TI\mspm0_sdk_2_11_00_07"
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
