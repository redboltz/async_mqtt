# Copyright Takatoshi Kondo 2022
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

list(APPEND check_PROGRAMS
    ut_broker_security.cpp
    ut_buffer.cpp
    ut_code.cpp
    ut_ep_basic.cpp
    ut_ep_con_discon.cpp
    ut_ep_topic_alias.cpp
    ut_ep_recv_filter.cpp
    ut_ep_recv_max.cpp
    ut_ep_size_max.cpp
    ut_ep_store.cpp
    ut_host_port.cpp
    ut_packet_id.cpp
    ut_packet_v3_1_1_connect.cpp
    ut_packet_v3_1_1_connack.cpp
    ut_packet_v3_1_1_publish.cpp
    ut_packet_v3_1_1_puback.cpp
    ut_packet_v3_1_1_pubrec.cpp
    ut_packet_v3_1_1_pubrel.cpp
    ut_packet_v3_1_1_pubcomp.cpp
    ut_packet_v3_1_1_subscribe.cpp
    ut_packet_v3_1_1_suback.cpp
    ut_packet_v3_1_1_unsubscribe.cpp
    ut_packet_v3_1_1_unsuback.cpp
    ut_packet_v3_1_1_pingreq.cpp
    ut_packet_v3_1_1_pingresp.cpp
    ut_packet_v3_1_1_disconnect.cpp
    ut_packet_v5_connect.cpp
    ut_packet_v5_connack.cpp
    ut_packet_v5_publish.cpp
    ut_packet_v5_puback.cpp
    ut_packet_v5_pubrec.cpp
    ut_packet_v5_pubrel.cpp
    ut_packet_v5_pubcomp.cpp
    ut_packet_v5_subscribe.cpp
    ut_packet_v5_suback.cpp
    ut_packet_v5_unsubscribe.cpp
    ut_packet_v5_unsuback.cpp
    ut_packet_v5_pingreq.cpp
    ut_packet_v5_pingresp.cpp
    ut_packet_v5_disconnect.cpp
    ut_packet_v5_auth.cpp
    ut_property.cpp
    ut_prop_variant.cpp
    ut_retained_topic_map.cpp
    ut_retained_topic_map_broker.cpp
    ut_strm.cpp
    ut_subscription_map.cpp
    ut_subscription_map_broker.cpp
    ut_system_error.cpp
    ut_topic_alias.cpp
    ut_topic_sharename.cpp
    ut_topic_subopts.cpp
    ut_value_allocator.cpp
)

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

ADD_EXECUTABLE (
    ut_multi_file
    ut_multi_file1.cpp ut_multi_file2.cpp
)
target_link_libraries(ut_multi_file async_mqtt_iface)
