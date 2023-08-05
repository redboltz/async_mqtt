list(APPEND exec_PROGRAMS
    ep_cb_mqtt_client.cpp
    ep_future_mqtt_client.cpp
    ep_slcoro_mqtt_client.cpp
)

if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(CMAKE_CXX_STANDARD 20)
    message(STATUS "C++20 examples added")
    list(APPEND exec_PROGRAMS
        ep_cpp20coro_mqtt_client.cpp
    )
endif()

if(ASYNC_MQTT_USE_TLS)
    list(APPEND exec_PROGRAMS
        ep_slcoro_mqtts_client.cpp
    )
endif()

if(ASYNC_MQTT_USE_WS)
    list(APPEND exec_PROGRAMS
        ep_slcoro_ws_client.cpp
    )
endif()

if(ASYNC_MQTT_USE_TLS AND ASYNC_MQTT_USE_WS)
    list(APPEND exec_PROGRAMS
        ep_slcoro_wss_client.cpp
    )
endif()

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
endforeach()

if(UNIX)
    file(COPY ../test/certs/mosquitto.org.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
endif()

if(MSVC)
    file(COPY ../test/certs/mosquitto.org.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../test/certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../test/certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../test/certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

    file(COPY ../test/certs/mosquitto.org.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../test/certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../test/certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../test/certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")

    file(COPY ../test/certs/mosquitto.org.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../test/certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../test/certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../test/certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
endif()
