# Copyright Takatoshi Kondo 2023
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

list(APPEND check_PROGRAMS
    st_auth.cpp
    st_conflict_cid.cpp
    st_gencid.cpp
    st_mqtt_connect.cpp
    st_keep_alive.cpp
    st_inflight.cpp
    st_invalid.cpp
    st_offline.cpp
    st_pub.cpp
    st_retain.cpp
    st_reqres.cpp
    st_shared_sub.cpp
    st_sub.cpp
    st_will.cpp
)

if(ASYNC_MQTT_USE_TLS)
    list(APPEND check_PROGRAMS
        st_mqtts_connect.cpp
    )
endif()

if(ASYNC_MQTT_USE_WS)
    list(APPEND check_PROGRAMS
        st_ws_connect.cpp
    )
endif()

if(ASYNC_MQTT_USE_TLS AND ASYNC_MQTT_USE_WS)
    list(APPEND check_PROGRAMS
        st_wss_connect.cpp
    )
endif()

find_package(Boost 1.81.0 REQUIRED COMPONENTS unit_test_framework)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    link_directories(${Boost_LIBRARY_DIRS})
endif()

foreach(source_file ${check_PROGRAMS})
    get_filename_component(source_file_we ${source_file} NAME_WE)
    add_executable(${source_file_we} ${source_file})
    target_link_libraries(${source_file_we} async_mqtt_iface)
    target_compile_definitions(
        ${source_file_we}
        PUBLIC
        $<IF:$<BOOL:${ASYMC_MQTT_USE_STATIC_BOOST}>,,BOOST_TEST_DYN_LINK>
    )
    target_link_libraries(
        ${source_file_we} Boost::unit_test_framework
    )
    if(WIN32 AND ASYNC_MQTT_USE_STATIC_OPENSSL)
        TARGET_LINK_LIBRARIES (${source_file_we} Crypt32)
    endif()

    if(ASYNC_MQTT_USE_LOG)
        target_compile_definitions(
            ${source_file_we}
            PUBLIC
            $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
        )
        target_link_libraries(
            ${source_file_we} Boost::log
        )
    endif()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if(ASYNC_MQTT_CODECOV)
            set_property(
                TARGET ${source_file_we}
                APPEND_STRING PROPERTY COMPILE_FLAGS " -O0 -g --coverage -fno-inline"
            )
            set_property(
                TARGET ${source_file_we}
                APPEND_STRING PROPERTY LINK_FLAGS " --coverage"
            )
        endif()
    endif()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
            STRING(REGEX REPLACE "/W[0-4]" "/W3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else()
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
        endif()
    endif()

    # Running test with arguments
    # CTEST_ARGS="--log_level=all" ctest -V
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        add_test(
            NAME ${source_file_we}
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${source_file_we}
        )
    else()
        add_test(
            NAME ${source_file_we}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../args_provider.sh ${CMAKE_CURRENT_BINARY_DIR}/${source_file_we}
        )
    endif()
    set_tests_properties(${source_file_we} PROPERTIES TIMEOUT 400)
endforeach()

if(UNIX)
    file(COPY st_broker.conf DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY st_auth.json DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../certs/server.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../certs/server.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
endif()

if(MSVC)
    file(COPY st_broker.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY st_auth.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ../certs/server.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../certs/server.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

    file(COPY st_broker.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
    file(COPY st_auth.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
    file(COPY ../certs/server.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../certs/server.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")

    file(COPY st_broker.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    file(COPY st_auth.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    file(COPY ../certs/server.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../certs/server.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
endif()
