@echo off
setlocal

set "QT_ROOT=D:\QT\6.10.2\mingw_64"
set "MINGW_ROOT=D:\QT\Tools\mingw1310_64"
set "SQLITE_DLL_DIR=D:\QT\Tools\mingw1310_64\opt\lib\sqlite3.34.0"
pushd "%~dp0"

call "build_study_assistant_qt.bat" || goto :fail

set "PATH=%MINGW_ROOT%\bin;%QT_ROOT%\bin;%SQLITE_DLL_DIR%;%PATH%"
set "QT_PLUGIN_PATH=%CD%\study_assistant"
"study_assistant\study_assistant_qt.exe"

popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
