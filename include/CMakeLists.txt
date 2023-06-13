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

# You might wonder why we don't simply add the list of header files to the check_deps
# executable directly, and let cmake figure everything out on it's own.
# The reason we can't do that is because cmake has built in rules that can't be disabled
# or overridden that prevent it from ever treating a file that ends in .h as a source file.
# So the work around for this problem is that we copy the header files to a different folder,
# one that's owned by CMake (the binary dir), and compile the header files there to perform
# the dependency checking.
set(HDR_DEPS_CHK_FOLDER "${CMAKE_BINARY_DIR}/check_deps_headers")
file (MAKE_DIRECTORY "${HDR_DEPS_CHK_FOLDER}")
list(APPEND CHK_ASYNC_MQTT)
file(GLOB_RECURSE ALL_HPP "async_mqtt/*.hpp")
foreach(HDR ${ALL_HPP})
    get_filename_component(HDR_NAME ${HDR} NAME)
    # Of course, the FILE macro always treats the destination name as if it were a folder name
    # So we have to first copy the header file, and THEN rename the header file to have an
    # extension that cmake will treat as a source file.
    file(COPY ${HDR} DESTINATION "${HDR_DEPS_CHK_FOLDER}")
    file(RENAME "${HDR_DEPS_CHK_FOLDER}/${HDR_NAME}" "${HDR_DEPS_CHK_FOLDER}/${HDR_NAME}.cpp")
    list(APPEND CHK_ASYNC_MQTT "${HDR_DEPS_CHK_FOLDER}/${HDR_NAME}.cpp")
endforeach()

# The EXCLUDE_FROM_ALL property prevents check_deps from being included in the default target
add_library(check_deps OBJECT EXCLUDE_FROM_ALL ${CHK_ASYNC_MQTT})

# We don't need to actually run the whole compiler. We're just checking that all the includes are valid
# So here we ask for only the syntax checking mode to be used.
# We also don't mind that there might be unused constant variables when doing deps checking.
target_compile_options(check_deps PUBLIC -Wno-unused-const-variable $<IF:$<CXX_COMPILER_ID:MSVC>,/Zs,-fsyntax-only>)

target_link_libraries(check_deps PUBLIC ${PROJECT_NAME})

install(DIRECTORY . DESTINATION include FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
