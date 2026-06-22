@echo off
setlocal

set "MINGW_ROOT=D:\QT\Tools\mingw1310_64"
set "SQLITE_DLL_DIR=D:\QT\Tools\mingw1310_64\opt\lib\sqlite3.34.0"
pushd "%~dp0"

call "build_study_assistant.bat" || goto :fail

set "PATH=%MINGW_ROOT%\bin;%SQLITE_DLL_DIR%;%PATH%"
"study_assistant\study_assistant_demo.exe"

popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
