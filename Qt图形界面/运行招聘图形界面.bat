@echo off
chcp 65001 >nul
setlocal

set "QT_ROOT=D:\QT\6.10.2\mingw_64"
set "MINGW_ROOT=D:\QT\Tools\mingw1310_64"
pushd "%~dp0"
set "APP_DIR=build_manual"
set "APP_EXE=%APP_DIR%\招聘图形界面.exe"

echo Rebuilding GUI before launch...
call "构建招聘图形界面.bat" || goto :fail

set "PATH=%MINGW_ROOT%\bin;%QT_ROOT%\bin;%PATH%"
start "" "%APP_EXE%"

popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
