cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(agilepf_hardware)

SET(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH "/opt/legged_sdk/lib/cmake")
find_package(legged_sdk REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PkgConfig)
pkg_check_modules(SERIAL libserial)
find_package(gainput REQUIRED)
set(GAINPUT_LIBRARIES "gainput" "X11" "GL" "rt" CACHE PATH "Gainput library directory")

include_directories(
    include 
    /opt/legged_sdk/include
    ${SERIAL_INCLUDE_DIRS}
    ${GAINPUT_INCLUDE_DIRS})
link_directories(
    /usr/local/lib/gainput # add gainput library path
)

add_executable(agilepf_hardware
    src/main.cpp
    src/ch_serial.c
    src/updown_msg.cpp)

target_link_libraries(agilepf_hardware
    legged_sdk 
    yaml-cpp
    ${SERIAL_LDFLAGS}
    ${GAINPUT_LIBRARIES})

add_executable(test_gainput
    src/test_gainput2.cc)
target_link_libraries(test_gainput
    legged_sdk
    ${GAINPUT_LIBRARIES})
