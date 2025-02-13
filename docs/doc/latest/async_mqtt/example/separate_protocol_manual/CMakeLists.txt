project(separate_protocol_manual)

add_executable(
   separate_protocol_manual
   src.cpp
   main.cpp
)

# Without this setting added, azure pipelines completely fails to find the boost libraries. No idea why.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
endif()

target_compile_definitions(separate_protocol_manual PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)

if(ASYNC_MQTT_USE_LOG)
    target_compile_definitions(
        separate_protocol_manual
        PRIVATE
        $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
    )
    target_link_libraries(
        separate_protocol_manual PRIVATE async_mqtt_iface Boost::log Boost::log_setup
    )
endif()

target_compile_definitions(
    separate_protocol_manual
    PRIVATE
    $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_PROGRAM_OPTIONS_DYN_LINK>
)
