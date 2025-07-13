@echo off
setlocal enabledelayedexpansion

:: 获取脚本路径
set "sourcePath=%~f0"

:: 解析符号链接
:resolve_link
if exist "%sourcePath%" (
    for /f "tokens=*" %%A in ('dir /a:l /b "%sourcePath%" 2^>nul') do (
        set "linkTarget=%%~fA"
        if not "!linkTarget!"=="" (
            set "sourcePath=!linkTarget!"
            goto resolve_link
        )
    )
)

:: 获取基础路径
for %%A in ("%sourcePath%") do set "basePath=%%~dpA"
set "basePath=%basePath:~0,-1%"

echo NDK_HOME = %NDK_HOME%
echo APP_ROOT = %basePath%

cd /d "%basePath%"
call "%NDK_HOME%\ndk-build.cmd"
