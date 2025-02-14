list(APPEND exec_PROGRAMS
    bench.cpp
    broker.cpp
    client_cli.cpp
)

if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(CMAKE_CXX_STANDARD 20)
    message(STATUS "C++20 tools added")
    list(APPEND exec_PROGRAMS
        cpp20coro_broker.cpp
    )
endif()

find_package(Boost 1.82.0 REQUIRED COMPONENTS program_options)

# Without this setting added, azure pipelines completely fails to find the boost libraries. No idea why.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
endif()

include(FetchContent)
FetchContent_Declare(
  cli
  GIT_REPOSITORY https://github.com/daniele77/cli.git
  GIT_TAG 80541c47bc4c7a6baa205136ca558860cb5e61af
)
FetchContent_MakeAvailable(cli)

foreach(source_file ${exec_PROGRAMS})
    get_filename_component(source_file_we ${source_file} NAME_WE)
    add_executable(${source_file_we} ${source_file})
    target_include_directories(${source_file_we} PRIVATE include ${Boost_INCLUDE_DIRS})
    target_link_libraries(${source_file_we} async_mqtt_iface)

    if(ASYNC_MQTT_MRDOCS)
        target_compile_definitions(
            ${source_file_we}
            PUBLIC
            ASYNC_MQTT_MRDOCS
        )
    endif()


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

# Separate compiled broker
if(ASYNC_MQTT_BUILD_LIB)
    add_executable(broker_separate broker.cpp)
    add_dependencies(broker_separate async_mqtt_asio_bind async_mqtt_protocol)
    target_compile_definitions(broker_separate PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)
    target_include_directories(broker_separate PRIVATE include ${Boost_INCLUDE_DIRS})
    target_link_libraries(broker_separate async_mqtt_iface async_mqtt_asio_bind async_mqtt_protocol)
    if(ASYNC_MQTT_USE_LOG)
        target_compile_definitions(
            broker_separate
            PUBLIC
            $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
        )
        target_link_libraries(
            broker_separate Boost::log
        )
    endif()
    target_compile_definitions(
        broker_separate
        PUBLIC
        $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_PROGRAM_OPTIONS_DYN_LINK>
    )
    target_link_libraries(broker_separate Boost::program_options)

    if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        set(CMAKE_CXX_STANDARD 20)
        message(STATUS "C++20 separate broker added")
        add_executable(cpp20coro_broker_separate cpp20coro_broker.cpp)
        add_dependencies(cpp20coro_broker_separate async_mqtt_asio_bind async_mqtt_protocol)
        target_compile_definitions(cpp20coro_broker_separate PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)
        target_include_directories(cpp20coro_broker_separate PRIVATE include ${Boost_INCLUDE_DIRS})
        target_link_libraries(cpp20coro_broker_separate async_mqtt_iface async_mqtt_asio_bind async_mqtt_protocol)
        if(ASYNC_MQTT_USE_LOG)
            target_compile_definitions(
                cpp20coro_broker_separate
                PUBLIC
                $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
            )
            target_link_libraries(
                cpp20coro_broker_separate Boost::log
            )
        endif()
        target_compile_definitions(
            cpp20coro_broker_separate
            PUBLIC
            $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_PROGRAM_OPTIONS_DYN_LINK>
        )
        target_link_libraries(cpp20coro_broker_separate Boost::program_options)
    endif()
endif()

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
