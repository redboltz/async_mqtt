# Copyright Takatoshi Kondo 2022
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required (VERSION 3.13.0)
project(async_mqtt_iface VERSION 10.2.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ASYNC_MQTT_USE_TLS "Enable building TLS code" OFF)
option(ASYNC_MQTT_USE_WS "Enable building WebSockets code" OFF)
option(ASYNC_MQTT_USE_LOG "Enable building logging code" OFF)
option(ASYNC_MQTT_PRINT_PAYLOAD "Enable output payload when publish packet output" OFF)
option(ASYNC_MQTT_BUILD_UNIT_TESTS "Enable building unit tests" OFF)
option(ASYNC_MQTT_BUILD_SYSTEM_TESTS "Enable building system tests" OFF)
option(ASYNC_MQTT_BUILD_TOOLS "Enable building tools (broker, bench, etc.." OFF)
option(ASYNC_MQTT_BUILD_EXAMPLES "Enable building example applications" OFF)
option(ASYNC_MQTT_BUILD_EXAMPLES_SEPARATE "Enable building separate library build example applications(It requires much memory)" OFF)
option(ASYNC_MQTT_BUILD_LIB "Enable building separate compilation library" OFF)
option(ASYNC_MQTT_MRDOCS "For Mr.Docs document generation" OFF)

# Not implemented yet
option(ASYNC_MQTT_USE_STR_CHECK "Enable UTF8 String check" OFF)

# Don't use GNU extensions
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Setting minimum C++ standard to C++17")

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

if(POLICY CMP0167)
  if (MSVC)
    cmake_policy(SET CMP0167 OLD)
  else ()
    cmake_policy(SET CMP0167 NEW)
  endif()
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
    list(APPEND ASYNC_MQTT_BOOST_COMPONENTS log log_setup)
else()
    message(STATUS "Logging disabled")
endif()

if(ASYNC_MQTT_PRINT_PAYLOAD)
    message(STATUS "Print payload enabled")
else()
    message(STATUS "Print payload disabled")
endif()

find_package(Boost 1.82.0 REQUIRED COMPONENTS ${ASYNC_MQTT_BOOST_COMPONENTS})
message(STATUS "Found Boost version: ${Boost_VERSION}")

if(ASYNC_MQTT_USE_TLS)
    find_package(OpenSSL REQUIRED)
    set(ASYNC_MQTT_DEPENDS_OPENSSL "FIND_DEPENDENCY (OpenSSL)")
endif()


set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)
add_subdirectory(include)

if(ASYNC_MQTT_BUILD_MRDOCS)
    message(STATUS "MrDocs generation")
    set(CMAKE_SYSROOT /usr)
    file(
        WRITE
        "${CMAKE_CURRENT_BINARY_DIR}/mrdocs.cpp"
        "#include <async_mqtt/all.hpp>\n"
        "#include <async_mqtt/asio_bind/predefined_layer/mqtts.hpp>\n"
        "#include <async_mqtt/asio_bind/predefined_layer/ws.hpp>\n"
        "#include <async_mqtt/asio_bind/predefined_layer/wss.hpp>\n"
    )
    add_library(async_mqtt_mrdocs "${CMAKE_CURRENT_BINARY_DIR}/mrdocs.cpp")

    target_compile_features(async_mqtt_mrdocs PUBLIC cxx_constexpr)
    target_compile_definitions(async_mqtt_mrdocs PUBLIC ASYNC_MQTT_MRDOCS ASYNC_MQTT_USE_LOG)
    set_target_properties(async_mqtt_mrdocs PROPERTIES NO_SYSTEM_FROM_IMPORTED OFF)
    target_include_directories(async_mqtt_mrdocs PUBLIC include ${Boost_INCLUDE_DIRS})
    return()
endif()

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

if(ASYNC_MQTT_BUILD_LIB)
    message(STATUS "Library build enabled")
    add_subdirectory(lib)
else()
    message(STATUS "Library build disabled")
endif()

# Heder dependency checking
add_custom_target("check_deps")
if(MSVC)
    set(OPTION_SYNTAX_ONLY "/Zs")
    set(OPTION_INCLUDE "/I")
else()
    set(OPTION_SYNTAX_ONLY "-fsyntax-only")
    set(OPTION_INCLUDE "-I")
    set(OPTION_STANDARD "-std=c++17")
    set(OPTION_LANGUAGE "-xc++")
endif()
file(GLOB_RECURSE ALL_HDR RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "include/*.hpp" "include/*.ipp" "tool/broker/*.hpp")
list(FILTER ALL_HDR EXCLUDE REGEX ".*/src.hpp")
foreach(HDR ${ALL_HDR})
    string(REPLACE "/" "_" REPLACED ${HDR})
    set(TARGET "check_deps_${REPLACED}")
    add_custom_target(
      ${TARGET}
      COMMAND ${CMAKE_CXX_COMPILER} "${OPTION_STANDARD}" "${OPTION_INCLUDE}${Boost_INCLUDE_DIRS}" "${OPTION_INCLUDE}${CMAKE_CURRENT_SOURCE_DIR}/include" "${OPTION_SYNTAX_ONLY}" "${OPTION_LANGUAGE}" "${CMAKE_CURRENT_SOURCE_DIR}/${HDR}"
    )
    add_dependencies("check_deps" ${TARGET})
endforeach()

include(cmake/createcmakeconfig.cmake)
