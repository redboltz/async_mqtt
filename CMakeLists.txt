# Copyright Takatoshi Kondo 2022
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required (VERSION 3.24.0)
project(async_mqtt_iface VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use GNU extensions
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Setting minimum C++ standard to C++17")

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

include_directories(include)

option(ASYNC_MQTT_BUILD_TESTS "Enable building test applications" ON)
option(ASYNC_MQTT_BUILD_EXAMPLES "Enable building example applications" ON)


set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)
add_subdirectory(include)

add_subdirectory(example)

if(ASYNC_MQTT_BUILD_TESTS)
    message(STATUS "Tests enabled")
    enable_testing()
    add_subdirectory(test)
endif()
