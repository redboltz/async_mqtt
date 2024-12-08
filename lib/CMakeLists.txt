# Separate compilation library

file(GLOB_RECURSE ASIO_BIND_IPP ${CMAKE_CURRENT_SOURCE_DIR} "../include/async_mqtt/impl/*.ipp")
set_source_files_properties(${ASIO_BIND_IPP} PROPERTIES LANGUAGE CXX)
add_library(async_mqtt_asio_bind ${ASIO_BIND_IPP})
target_compile_definitions(async_mqtt_asio_bind PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(async_mqtt_asio_bind PRIVATE -x c++)
endif()

file(GLOB_RECURSE PROTOCOL_IPP ${CMAKE_CURRENT_SOURCE_DIR} "../include/async_mqtt/protocol/*.ipp")
set_source_files_properties(${PROTOCOL_IPP} PROPERTIES LANGUAGE CXX)
add_library(async_mqtt_protocol ${PROTOCOL_IPP})
target_compile_definitions(async_mqtt_protocol PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(async_mqtt_protocol PRIVATE -x c++)
endif()

if(ASYNC_MQTT_USE_LOG)
    target_compile_definitions(
        async_mqtt_asio_bind
        PRIVATE
        $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
    )
    target_compile_definitions(
        async_mqtt_protocol
        PRIVATE
        $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
    )
endif()

macro(fix_msvc_build)
    message(STATUS "MSVC build fix applied")
    target_compile_options(async_mqtt_asio_bind PRIVATE "/Zc:preprocessor" "/bigobj")
    target_compile_options(async_mqtt_protocol PRIVATE "/Zc:preprocessor" "/bigobj")
endmacro()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    fix_msvc_build()
endif()

target_link_libraries(async_mqtt_asio_bind async_mqtt_iface)
target_link_libraries(async_mqtt_protocol async_mqtt_iface)
