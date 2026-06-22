@echo off
setlocal

set "QT_ROOT=D:\QT\6.10.2\mingw_64"
set "MINGW_ROOT=D:\QT\Tools\mingw1310_64"
set "SQLITE_ROOT=D:\QT\Tools\mingw1310_64\opt"
set "SQLITE_DLL_DIR=%SQLITE_ROOT%\lib\sqlite3.34.0"
pushd "%~dp0"

if not exist "%QT_ROOT%\bin\qmake.exe" (
    echo Qt not found: %QT_ROOT%\bin\qmake.exe
    exit /b 1
)

if not exist "%MINGW_ROOT%\bin\g++.exe" (
    echo MinGW g++ not found: %MINGW_ROOT%\bin\g++.exe
    exit /b 1
)

if not exist "%SQLITE_ROOT%\include\sqlite3.h" (
    echo sqlite3 header not found: %SQLITE_ROOT%\include\sqlite3.h
    exit /b 1
)

if not exist "study_assistant\platforms" mkdir "study_assistant\platforms"
if not exist "study_assistant\tls" mkdir "study_assistant\tls"

echo Building study_assistant_qt.exe...
"%MINGW_ROOT%\bin\g++.exe" -std=c++17 -Wall -Wextra -I"study_assistant" ^
 -I"%QT_ROOT%\include" ^
 -I"%QT_ROOT%\include\QtWidgets" ^
 -I"%QT_ROOT%\include\QtCore" ^
 -I"%QT_ROOT%\include\QtGui" ^
 -I"%QT_ROOT%\include\QtNetwork" ^
 -I"%SQLITE_ROOT%\include" ^
 "study_assistant\study_qt_main.cpp" ^
 "study_assistant\study_window.cpp" ^
 "study_assistant\openai_client.cpp" ^
 "study_assistant\study_service.cpp" ^
 -L"%QT_ROOT%\lib" -L"%SQLITE_ROOT%\lib" -lQt6Widgets -lQt6Gui -lQt6Core -lQt6Network -lsqlite3 ^
 -o "study_assistant\study_assistant_qt.exe" || goto :fail

copy /Y "%QT_ROOT%\bin\Qt6Widgets.dll" "study_assistant\" >nul || goto :fail
copy /Y "%QT_ROOT%\bin\Qt6Gui.dll" "study_assistant\" >nul || goto :fail
copy /Y "%QT_ROOT%\bin\Qt6Core.dll" "study_assistant\" >nul || goto :fail
copy /Y "%QT_ROOT%\bin\Qt6Network.dll" "study_assistant\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libstdc++-6.dll" "study_assistant\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libgcc_s_seh-1.dll" "study_assistant\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libwinpthread-1.dll" "study_assistant\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\platforms\qwindows.dll" "study_assistant\platforms\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\tls\qcertonlybackend.dll" "study_assistant\tls\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\tls\qopensslbackend.dll" "study_assistant\tls\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\tls\qschannelbackend.dll" "study_assistant\tls\" >nul || goto :fail
copy /Y "%SQLITE_DLL_DIR%\sqlite3340.dll" "study_assistant\" >nul || goto :fail

echo Build finished: study_assistant\study_assistant_qt.exe
popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
