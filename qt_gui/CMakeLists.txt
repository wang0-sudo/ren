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

add_executable(recruitment_qt_gui
    main.cpp
    login_window.cpp
    dashboard_window.cpp
    recruitment_service.cpp
)

add_executable(study_assistant_demo
    study_assistant/demo_main.cpp
    study_assistant/study_service.cpp
)

add_executable(study_assistant_qt
    study_assistant/study_qt_main.cpp
    study_assistant/study_window.cpp
    study_assistant/openai_client.cpp
    study_assistant/study_service.cpp
)

target_include_directories(recruitment_qt_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(study_assistant_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/study_assistant
    ${SQLITE_ROOT}/include
)
target_include_directories(study_assistant_qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/study_assistant
    ${SQLITE_ROOT}/include
)

target_link_libraries(recruitment_qt_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(study_assistant_demo PRIVATE ${SQLITE_ROOT}/lib/libsqlite3.a)
target_link_libraries(study_assistant_qt PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${SQLITE_ROOT}/lib/libsqlite3.a
)

add_custom_command(TARGET study_assistant_qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:study_assistant_qt>/tls"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${QT_PLUGINS_DIR}/tls"
        "$<TARGET_FILE_DIR:study_assistant_qt>/tls"
)
