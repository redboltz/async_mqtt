list(APPEND exec_PROGRAMS
    bench.cpp
    broker.cpp
    client_cli.cpp
)

find_package(Boost 1.81.0 REQUIRED COMPONENTS program_options)

# Without this setting added, azure pipelines completely fails to find the boost libraries. No idea why.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
endif()

include(FetchContent)
FetchContent_Declare(
  cli
  GIT_REPOSITORY https://github.com/daniele77/cli.git
  GIT_TAG v2.0.2
)
FetchContent_MakeAvailable(cli)

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
    target_link_libraries(${source_file_we} Boost::program_options cli::cli)
endforeach()

if(UNIX)
    file(COPY broker.conf DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY auth.json DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY bench.conf DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY cli.conf DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/mosquitto.org.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/server.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/server.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
    file(COPY ../test/certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
endif()

if(MSVC)
    file(COPY broker.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY auth.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY bench.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY cli.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ../test/certs/mosquitto.org.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../test/certs/server.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../test/certs/server.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../test/certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../test/certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ../test/certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

    file(COPY broker.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
    file(COPY auth.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
    file(COPY bench.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
    file(COPY cli.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release )
    file(COPY ../test/certs/mosquitto.org.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../test/certs/server.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../test/certs/server.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../test/certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../test/certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
    file(COPY ../test/certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")

    file(COPY broker.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    file(COPY auth.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    file(COPY bench.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    file(COPY cli.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    file(COPY ../test/certs/mosquitto.org.crt DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../test/certs/server.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../test/certs/server.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../test/certs/client.crt.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../test/certs/client.key.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    file(COPY ../test/certs/cacert.pem DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
endif()
