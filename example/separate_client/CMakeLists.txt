project(separate_client)

add_executable(
   separate_client
   main.cpp
)
add_dependencies(separate_client async_mqtt_asio_bind async_mqtt_protocol)

# Without this setting added, azure pipelines completely fails to find the boost libraries. No idea why.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
endif()

target_link_libraries(separate_client PRIVATE async_mqtt_iface async_mqtt_asio_bind async_mqtt_protocol)
target_compile_definitions(separate_client PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)

if(ASYNC_MQTT_USE_LOG)
    target_compile_definitions(
        separate_client
        PRIVATE
        $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
    )
    target_link_libraries(
        separate_client PRIVATE Boost::log Boost::log_setup
    )
endif()

target_compile_definitions(
    separate_client
    PRIVATE
    $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_PROGRAM_OPTIONS_DYN_LINK>
)
