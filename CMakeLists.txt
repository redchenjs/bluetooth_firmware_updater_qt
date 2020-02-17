cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

project(easy-firmware-updater)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5SerialPort REQUIRED)

set(SOURCES src/main.cpp src/updater.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::SerialPort)

set(INSTALL_DIR bin)
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_DIR})
