@echo off
chcp 65001 >nul
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

if not exist "学习助手\platforms" mkdir "学习助手\platforms"
if not exist "学习助手\tls" mkdir "学习助手\tls"

echo Building 学习助手图形界面.exe...
"%MINGW_ROOT%\bin\g++.exe" -std=c++17 -Wall -Wextra -I"学习助手" ^
 -I"%QT_ROOT%\include" ^
 -I"%QT_ROOT%\include\QtWidgets" ^
 -I"%QT_ROOT%\include\QtCore" ^
 -I"%QT_ROOT%\include\QtGui" ^
 -I"%QT_ROOT%\include\QtNetwork" ^
 -I"%SQLITE_ROOT%\include" ^
 "学习助手\学习助手图形主程序.cpp" ^
 "学习助手\学习窗口.cpp" ^
 "学习助手\OpenAI客户端.cpp" ^
 "学习助手\学习服务.cpp" ^
 -L"%QT_ROOT%\lib" -L"%SQLITE_ROOT%\lib" -lQt6Widgets -lQt6Gui -lQt6Core -lQt6Network -lsqlite3 ^
 -o "学习助手\学习助手图形界面.exe" || goto :fail

copy /Y "%QT_ROOT%\bin\Qt6Widgets.dll" "学习助手\" >nul || goto :fail
copy /Y "%QT_ROOT%\bin\Qt6Gui.dll" "学习助手\" >nul || goto :fail
copy /Y "%QT_ROOT%\bin\Qt6Core.dll" "学习助手\" >nul || goto :fail
copy /Y "%QT_ROOT%\bin\Qt6Network.dll" "学习助手\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libstdc++-6.dll" "学习助手\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libgcc_s_seh-1.dll" "学习助手\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libwinpthread-1.dll" "学习助手\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\platforms\qwindows.dll" "学习助手\platforms\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\tls\qcertonlybackend.dll" "学习助手\tls\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\tls\qopensslbackend.dll" "学习助手\tls\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\tls\qschannelbackend.dll" "学习助手\tls\" >nul || goto :fail
copy /Y "%SQLITE_DLL_DIR%\sqlite3340.dll" "学习助手\" >nul || goto :fail

echo Build finished: 学习助手\学习助手图形界面.exe
popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
