list(APPEND exec_PROGRAMS
    send_packet.cpp
    recv_packet.cpp
)

find_package(Boost 1.74.0 REQUIRED COMPONENTS program_options)

# Without this setting added, azure pipelines completely fails to find the boost libraries. No idea why.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
endif()

foreach(source_file ${exec_PROGRAMS})
    get_filename_component(source_file_we ${source_file} NAME_WE)
    add_executable(${source_file_we} ${source_file})

    IF (WIN32 AND MQTT_USE_STATIC_OPENSSL)
        TARGET_LINK_LIBRARIES (${source_file_we} Crypt32)
    ENDIF ()

    if(ASYNC_MQTT_USE_LOG)
        target_compile_definitionss(
            ${source_file_we}
            PUBLIC
            $<IF:$<BOOL:${MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
        )
        target_link_libraries(
            ${source_file_we} Boost::log
        )
    endif()
    target_compile_definitions(
        ${source_file_we}
        PUBLIC
        $<IF:$<BOOL:${MQTT_USE_STATIC_BOOST}>,,BOOST_PROGRAM_OPTIONS_DYN_LINK>
    )
    target_link_libraries(${source_file_we} Boost::program_options)
endforeach()
