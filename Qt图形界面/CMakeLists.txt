cmake_minimum_required(VERSION 3.16)
project(RecruitmentQtGui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(SQLITE_ROOT "D:/QT/Tools/mingw1310_64/opt")
get_target_property(QT_QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
get_filename_component(QT_ROOT_DIR "${QT_BIN_DIR}" DIRECTORY)
set(QT_PLUGINS_DIR "${QT_ROOT_DIR}/plugins")

add_executable(招聘图形界面
    主程序.cpp
    登录窗口.cpp
    工作台窗口.cpp
    招聘服务.cpp
)

add_executable(学习助手控制台版
    学习助手/演示主程序.cpp
    学习助手/学习服务.cpp
)

add_executable(学习助手图形界面
    学习助手/学习助手图形主程序.cpp
    学习助手/学习窗口.cpp
    学习助手/OpenAI客户端.cpp
    学习助手/学习服务.cpp
)

target_include_directories(招聘图形界面 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(学习助手控制台版 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/学习助手
    ${SQLITE_ROOT}/include
)
target_include_directories(学习助手图形界面 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/学习助手
    ${SQLITE_ROOT}/include
)

target_link_libraries(招聘图形界面 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(学习助手控制台版 PRIVATE ${SQLITE_ROOT}/lib/libsqlite3.a)
target_link_libraries(学习助手图形界面 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${SQLITE_ROOT}/lib/libsqlite3.a
)

add_custom_command(TARGET 学习助手图形界面 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:学习助手图形界面>/tls"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${QT_PLUGINS_DIR}/tls"
        "$<TARGET_FILE_DIR:学习助手图形界面>/tls"
)
