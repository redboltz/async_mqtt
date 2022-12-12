# Copyright Takatoshi Kondo 2022
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required (VERSION 3.13.0)
project(async_mqtt_iface VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ASYNC_MQTT_USE_TLS "Enable building TLS code" OFF)
option(ASYNC_MQTT_USE_WS "Enable building WebSockets code" OFF)
option(ASYNC_MQTT_BUILD_TESTS "Enable building test applications" ON)
option(ASYNC_MQTT_BUILD_EXAMPLES "Enable building example applications" ON)
OPTION(ASYNC_MQTT_USE_LOG "Enable building logging code" OFF)

# Don't use GNU extensions
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Setting minimum C++ standard to C++17")

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

if(ASYNC_MQTT_USE_STATIC_BOOST)
    message(STATUS "Staticly linking with Boost")
    set(Boost_USE_STATIC_LIBS TRUE)
else()
    message(STATUS "Dynamically linking with Boost")
    set(Boost_USE_STATIC_LIBS FALSE)
endif()

if(ASYNC_MQTT_USE_STATIC_OPENSSL)
    message(STATUS "Staticly linking with Openssl")
    set(OPENSSL_USE_STATIC_LIBS TRUE)
else()
    message(STATUS "Dynamically linking with Openssl")
    set(OPENSSL_USE_STATIC_LIBS FALSE)
endif()

set(Boost_USE_MULTITHREADED ON)
find_package(Threads REQUIRED)

if(ASYNC_MQTT_USE_TLS)
    message(STATUS "TLS enabled")
else()
    message(STATUS "TLS disabled")
endif()

if(ASYNC_MQTT_USE_WS)
    message(STATUS "WS enabled")
else()
    message(STATUS "WS disabled")
endif()

set(ASYNC_MQTT_BOOST_COMPONENTS)
if(ASYNC_MQTT_USE_LOG)
    message(STATUS "Logging enabled")
#    set(ASYNC_MQTT_BOOST_COMPONENTS system date_time log filesystem thread program_options)
    set(ASYNC_MQTT_BOOST_COMPONENTS log)
else()
    message(STATUS "Logging disabled")
#    set(ASYNC_MQTT_BOOST_COMPONENTS system date_time program_options)
endif()
find_package(Boost 1.74.0 REQUIRED COMPONENTS ${ASYNC_MQTT_BOOST_COMPONENTS})

if(ASYNC_MQTT_USE_TLS)
    find_package(OpenSSL REQUIRED)
    set(ASYNC_MQTT_DEPENDS_OPENSSL "FIND_DEPENDENCY (OpenSSL)")
    if(ASYNC_MQTT_USE_STATIC_OPENSSL)
        find_package(ZLIB REQUIRED)
        set(ASYNC_MQTT_DEPENDS_ZLIB "FIND_DEPENDENCY (ZLIB)")
    endif()
endif()


#include_directories(include)

set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)
add_subdirectory(include)

if(ASYNC_MQTT_BUILD_EXAMPLES)
    message(STATUS "Examples enabled")
    add_subdirectory(example)
endif()

if(ASYNC_MQTT_BUILD_TESTS)
    message(STATUS "Tests enabled")
    enable_testing()
    add_subdirectory(test)
endif()

include(cmake/createcmakeconfig.cmake)
