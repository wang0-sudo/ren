@echo off
chcp 65001 >nul
setlocal

set "QT_ROOT=D:\QT\6.10.2\mingw_64"
set "MINGW_ROOT=D:\QT\Tools\mingw1310_64"

pushd "%~dp0"
set "BUILD_DIR=build_manual"

if not exist "%QT_ROOT%\bin\moc.exe" (
    echo Qt moc not found: %QT_ROOT%\bin\moc.exe
    exit /b 1
)

if not exist "%MINGW_ROOT%\bin\g++.exe" (
    echo MinGW g++ not found: %MINGW_ROOT%\bin\g++.exe
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set "PATH=%MINGW_ROOT%\bin;%QT_ROOT%\bin;%PATH%"

echo Generating moc files...
"%QT_ROOT%\bin\moc.exe" "登录窗口.h" -o "%BUILD_DIR%\moc_login_window.cpp" || goto :fail
"%QT_ROOT%\bin\moc.exe" "工作台窗口.h" -o "%BUILD_DIR%\moc_dashboard_window.cpp" || goto :fail

echo Building 招聘图形界面.exe...
"%MINGW_ROOT%\bin\g++.exe" -std=c++17 -Wall -Wextra -I"." ^
 -I"%QT_ROOT%\include" ^
 -I"%QT_ROOT%\include\QtWidgets" ^
 -I"%QT_ROOT%\include\QtCore" ^
 -I"%QT_ROOT%\include\QtGui" ^
 "主程序.cpp" ^
 "登录窗口.cpp" ^
 "工作台窗口.cpp" ^
 "招聘服务.cpp" ^
 "%BUILD_DIR%\moc_login_window.cpp" ^
 "%BUILD_DIR%\moc_dashboard_window.cpp" ^
 -L"%QT_ROOT%\lib" -lQt6Widgets -lQt6Gui -lQt6Core ^
 -o "%BUILD_DIR%\招聘图形界面.exe" || goto :fail

echo Copying Qt runtime files...
if not exist "%BUILD_DIR%\platforms" mkdir "%BUILD_DIR%\platforms"
copy /Y "%QT_ROOT%\bin\Qt6Widgets.dll" "%BUILD_DIR%\" >nul || goto :fail
copy /Y "%QT_ROOT%\bin\Qt6Gui.dll" "%BUILD_DIR%\" >nul || goto :fail
copy /Y "%QT_ROOT%\bin\Qt6Core.dll" "%BUILD_DIR%\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libstdc++-6.dll" "%BUILD_DIR%\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libgcc_s_seh-1.dll" "%BUILD_DIR%\" >nul || goto :fail
copy /Y "%MINGW_ROOT%\bin\libwinpthread-1.dll" "%BUILD_DIR%\" >nul || goto :fail
copy /Y "%QT_ROOT%\plugins\platforms\qwindows.dll" "%BUILD_DIR%\platforms\" >nul || goto :fail

echo Build finished: %BUILD_DIR%\招聘图形界面.exe
popd
endlocal
exit /b 0

:fail
popd
endlocal
exit /b 1
