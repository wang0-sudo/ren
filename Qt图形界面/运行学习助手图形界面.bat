@echo off
chcp 65001 >nul
setlocal

set "QT_ROOT=D:\QT\6.10.2\mingw_64"
set "MINGW_ROOT=D:\QT\Tools\mingw1310_64"
set "SQLITE_DLL_DIR=D:\QT\Tools\mingw1310_64\opt\lib\sqlite3.34.0"
pushd "%~dp0"

call "构建学习助手图形界面.bat" || goto :fail

set "PATH=%MINGW_ROOT%\bin;%QT_ROOT%\bin;%SQLITE_DLL_DIR%;%PATH%"
set "QT_PLUGIN_PATH=%CD%\学习助手"
"学习助手\学习助手图形界面.exe"

popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
