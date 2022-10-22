# Copyright Takatoshi Kondo 2022
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

if(ASYNC_MQTT_BUILD_UNIT_TESTS)
    add_subdirectory(unit)
    message(STATUS "Unit tests enabled")
else()
    message(STATUS "Unit tests disabled")
endif()

if(ASYNC_MQTT_BUILD_SYSTEM_TESTS)
    add_subdirectory (system)
    message(STATUS "System tests enabled")
else()
    message(STATUS "System tests disabled")
endif()
