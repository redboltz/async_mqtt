# Copyright Takatoshi Kondo 2022
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required (VERSION 3.13.0)
project(async_mqtt_iface VERSION 1.0.9)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ASYNC_MQTT_USE_TLS "Enable building TLS code" OFF)
option(ASYNC_MQTT_USE_WS "Enable building WebSockets code" OFF)
option(ASYNC_MQTT_USE_LOG "Enable building logging code" OFF)
option(ASYNC_MQTT_BUILD_UNIT_TESTS "Enable building unit tests" OFF)
option(ASYNC_MQTT_BUILD_SYSTEM_TESTS "Enable building system tests" OFF)
option(ASYNC_MQTT_BUILD_TOOLS "Enable building tools (broker, bench, etc.." OFF)
option(ASYNC_MQTT_BUILD_EXAMPLES "Enable building example applications" OFF)

# Not implemented yet
option(ASYNC_MQTT_USE_STR_CHECK "Enable UTF8 String check" OFF)

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
    set(ASYNC_MQTT_BOOST_COMPONENTS log)
else()
    message(STATUS "Logging disabled")
endif()
find_package(Boost 1.81.0 REQUIRED COMPONENTS ${ASYNC_MQTT_BOOST_COMPONENTS})

if(ASYNC_MQTT_USE_TLS)
    find_package(OpenSSL REQUIRED)
    set(ASYNC_MQTT_DEPENDS_OPENSSL "FIND_DEPENDENCY (OpenSSL)")
endif()


#include_directories(include)

set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)
add_subdirectory(include)

if(ASYNC_MQTT_BUILD_EXAMPLES)
    message(STATUS "Examples enabled")
    add_subdirectory(example)
else()
    message(STATUS "Examples disabled")
endif()

if(ASYNC_MQTT_BUILD_TOOLS)
    message(STATUS "Tools enabled")
    add_subdirectory(tool)
else()
    message(STATUS "Tools disabled")
endif()

if(ASYNC_MQTT_BUILD_UNIT_TESTS OR ASYNC_MQTT_BUILD_SYSTEM_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    list(APPEND Doxyfile_CONTENT
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "FILE_PATTERNS          = *.hpp" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "OUTPUT_DIRECTORY       = doc" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "PROJECT_NAME           = async_mqtt" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "PROJECT_NUMBER         = 1.0.9" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "RECURSIVE              = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "PREDEFINED             = _DOXYGEN_ ASYNC_MQTT_USE_TLS ASYNC_MQTT_USE_WS" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "INPUT                  = ${CMAKE_CURRENT_SOURCE_DIR}/include/async_mqtt" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "EXCLUDE                = ${CMAKE_CURRENT_SOURCE_DIR}/include/async_mqtt/broker" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "GENERATE_LATEX         = NO" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "HIDE_UNDOC_MEMBERS     = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "HIDE_UNDOC_CLASSES     = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
#        COMMAND ${CMAKE_COMMAND} -E echo "HIDE_FRIEND_COMPOUNDS  = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
#        COMMAND ${CMAKE_COMMAND} -E echo "HIDE_IN_BODY_DOCS      = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "CLASS_GRAPH            = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "INCLUDE_GRAPH          = NO" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "INCLUDED_BY_GRAPH      = NO" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile

    )
    if(DOXYGEN_DOT_FOUND)
        list(APPEND Doxyfile_CONTENT
            COMMAND ${CMAKE_COMMAND} -E echo "HAVE_DOT       = YES" >> ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        )
    endif()
    add_custom_target(
        doxygen
        ${Doxyfile_CONTENT}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        VERBATIM
    )
endif()

include(cmake/createcmakeconfig.cmake)
