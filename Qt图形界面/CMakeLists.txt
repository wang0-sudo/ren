cmake_minimum_required(VERSION 3.16)
project(RecruitmentQtGui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(招聘图形界面
    主程序.cpp
    登录窗口.cpp
    工作台窗口.cpp
    招聘服务.cpp
)

target_include_directories(招聘图形界面 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(招聘图形界面 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
