cmake_minimum_required(VERSION 3.16)
project(QtTest)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Qt6 REQUIRED COMPONENTS Quick SerialPort)
qt_policy(SET QTP0001 NEW)
qt_add_executable(appQtTest
    src/main.cpp
)
qt_add_qml_module(appQtTest
    URI QtTest
    QML_FILES qml/Main.qml
    SOURCES
        src/serialporthandler.h
        src/serialporthandler.cpp
)
qt_add_resources(appQtTest "qtquickcontrols2"
    PREFIX "/"
    FILES qml/qtquickcontrols2.conf
)
target_include_directories(appQtTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(appQtTest PRIVATE Qt6::Quick Qt6::SerialPort)
