@echo off
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

echo Building study_assistant_demo.exe...
"%MINGW_ROOT%\bin\g++.exe" -std=c++17 -Wall -Wextra -I"study_assistant" -I"%SQLITE_ROOT%\include" ^
 "study_assistant\demo_main.cpp" ^
 "study_assistant\study_service.cpp" ^
 -L"%SQLITE_ROOT%\lib" -lsqlite3 ^
 -o "study_assistant\study_assistant_demo.exe" || goto :fail

copy /Y "%SQLITE_DLL_DIR%\sqlite3340.dll" "study_assistant\" >nul || goto :fail

echo Build finished: study_assistant\study_assistant_demo.exe
popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
