cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

project(spp_fw_updater)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5SerialPort REQUIRED)

set(SRCS main.cpp updater.cpp)
set(INSTALLDIR bin)
add_executable(spp-fw-updater ${SRCS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
target_link_libraries(spp-fw-updater Qt5::Core Qt5::SerialPort)

install(TARGETS spp-fw-updater DESTINATION ${INSTALLDIR})
