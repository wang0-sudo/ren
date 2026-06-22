@echo off
chcp 65001 >nul
setlocal

set "MINGW_ROOT=D:\QT\Tools\mingw1310_64"
set "SQLITE_DLL_DIR=D:\QT\Tools\mingw1310_64\opt\lib\sqlite3.34.0"
pushd "%~dp0"

call "构建学习助手控制台版.bat" || goto :fail

set "PATH=%MINGW_ROOT%\bin;%SQLITE_DLL_DIR%;%PATH%"
"学习助手\学习助手控制台版.exe"

popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
