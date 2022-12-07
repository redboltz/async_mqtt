list(APPEND exec_PROGRAMS
    send_packet.cpp
    recv_packet.cpp
    epsp_recv_packet.cpp
    ep_send_packet.cpp
    ws_send_packet.cpp
    ws_recv_packet.cpp
    future.cpp
    broker.cpp
)

find_package(Boost 1.74.0 REQUIRED COMPONENTS program_options)

# Without this setting added, azure pipelines completely fails to find the boost libraries. No idea why.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
endif()

foreach(source_file ${exec_PROGRAMS})
    get_filename_component(source_file_we ${source_file} NAME_WE)
    add_executable(${source_file_we} ${source_file})
    target_link_libraries(${source_file_we} async_mqtt_iface)

    if(WIN32 AND ASYNC_MQTT_USE_STATIC_OPENSSL)
        target_link_libraries(${source_file_we} Crypt32)
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
    target_compile_definitions(
        ${source_file_we}
        PUBLIC
        $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_PROGRAM_OPTIONS_DYN_LINK>
    )
    target_link_libraries(${source_file_we} Boost::program_options)
endforeach()
