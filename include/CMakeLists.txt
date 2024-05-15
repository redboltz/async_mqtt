add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME} INTERFACE Threads::Threads Boost::boost)

if(ASYNC_MQTT_USE_STATIC_OPENSSL)
    # Unfortunately, cmake doesn't automatically detect that statically linked openssl
    # requires at least two more libs to be linked. "dl" is for dynamic library linking
    # and zlib is for basic compression functionality.
    # There is currently no way to specify statically linked zlib.
    target_link_libraries(${PROJECT_NAME} INTERFACE $<$<BOOL:${ASYNC_MQTT_USE_TLS}>:${CMAKE_DL_LIBS} ZLIB::ZLIB>)
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE $<$<BOOL:${ASYNC_MQTT_USE_TLS}>:OpenSSL::SSL>)

target_include_directories(${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<BOOL:${ASYNC_MQTT_USE_TLS}>:ASYNC_MQTT_USE_TLS>)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<BOOL:${ASYNC_MQTT_USE_WS}>:ASYNC_MQTT_USE_WS>)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<BOOL:${ASYNC_MQTT_USE_STR_CHECK}>:ASYNC_MQTT_USE_STR_CHECK>)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<BOOL:${ASYNC_MQTT_USE_LOG}>:ASYNC_MQTT_USE_LOG>)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<BOOL:${ASYNC_MQTT_PRINT_PAYLOAD}>:ASYNC_MQTT_PRINT_PAYLOAD>)

install(DIRECTORY . DESTINATION include FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
