@echo off
set TARGET_NAME=.dll
set PLATFORM=windows
set MAKE_TOOL=make

if "%1"=="debug" (
    set DEBUG=debug=1
) else (
    set DEBUG=
)

set current_path=%~dp0
cd /d "%current_path%/.."

%MAKE_TOOL% .a platform=%PLATFORM% %DEBUG%

cd ./build

%MAKE_TOOL% %TARGET_NAME% platform=%PLATFORM% target=widgets all=1 %DEBUG%