cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(bluetooth-firmware-updater)

set(TARGET_NAME btfwupd)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Bluetooth REQUIRED)

set(SOURCES src/main.cpp src/fwupd.cpp)
add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME} Qt5::Core Qt5::Bluetooth)

set(INSTALL_DIR bin)
install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DIR})
