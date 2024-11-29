# Separate compilation library

file(GLOB_RECURSE ASIO_BIND_IPP ${CMAKE_CURRENT_SOURCE_DIR} "../include/async_mqtt/impl/*.ipp")
FOREACH (IPP ${ASIO_BIND_IPP})
    GET_FILENAME_COMPONENT (IPP_NAME ${IPP} NAME)
    FILE (COPY ${IPP} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    FILE (RENAME "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}.cpp")
    LIST (APPEND ASIO_BIND_LIB_SRC "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}.cpp")
ENDFOREACH ()
add_library(async_mqtt_asio_bind ${ASIO_BIND_LIB_SRC})
target_compile_definitions(async_mqtt_asio_bind PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)

file(GLOB_RECURSE PROTOCOL_IPP ${CMAKE_CURRENT_SOURCE_DIR} "../include/async_mqtt/protocol/*.ipp")
FOREACH (IPP ${PROTOCOL_IPP})
    GET_FILENAME_COMPONENT (IPP_NAME ${IPP} NAME)
    FILE (COPY ${IPP} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    FILE (RENAME "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}.cpp")
    LIST (APPEND PROTOCOL_LIB_SRC "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}.cpp")
ENDFOREACH ()
add_library(async_mqtt_protocol ${PROTOCOL_LIB_SRC})
target_compile_definitions(async_mqtt_protocol PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)

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

target_link_libraries(async_mqtt_asio_bind async_mqtt_iface)
target_link_libraries(async_mqtt_protocol async_mqtt_iface)
