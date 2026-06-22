@echo off
chcp 65001 >nul
setlocal

set "MINGW_ROOT=D:\QT\Tools\mingw1310_64"
set "SQLITE_ROOT=D:\QT\Tools\mingw1310_64\opt"
set "SQLITE_DLL_DIR=%SQLITE_ROOT%\lib\sqlite3.34.0"
pushd "%~dp0"

if not exist "%MINGW_ROOT%\bin\g++.exe" (
    echo MinGW g++ not found: %MINGW_ROOT%\bin\g++.exe
    exit /b 1
)

if not exist "%SQLITE_ROOT%\include\sqlite3.h" (
    echo sqlite3 header not found: %SQLITE_ROOT%\include\sqlite3.h
    exit /b 1
)

echo Building 学习助手控制台版.exe...
"%MINGW_ROOT%\bin\g++.exe" -std=c++17 -Wall -Wextra -I"学习助手" -I"%SQLITE_ROOT%\include" ^
 "学习助手\演示主程序.cpp" ^
 "学习助手\学习服务.cpp" ^
 -L"%SQLITE_ROOT%\lib" -lsqlite3 ^
 -o "学习助手\学习助手控制台版.exe" || goto :fail

copy /Y "%SQLITE_DLL_DIR%\sqlite3340.dll" "学习助手\" >nul || goto :fail

echo Build finished: 学习助手\学习助手控制台版.exe
popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
