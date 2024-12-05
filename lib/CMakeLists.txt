# Separate compilation library
file(GLOB_RECURSE ALL_IPP ${CMAKE_CURRENT_SOURCE_DIR} "../include/*.ipp")

FOREACH (IPP ${ALL_IPP})
    GET_FILENAME_COMPONENT (IPP_NAME ${IPP} NAME)
    FILE (COPY ${IPP} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    FILE (RENAME "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}.cpp")
    LIST (APPEND LIB_SRC "${CMAKE_CURRENT_BINARY_DIR}/${IPP_NAME}.cpp")
ENDFOREACH ()

add_library(async_mqtt_separate ${LIB_SRC})

if(ASYNC_MQTT_USE_LOG)
    target_compile_definitions(
        async_mqtt_separate
        PRIVATE
        $<IF:$<BOOL:${ASYNC_MQTT_USE_STATIC_BOOST}>,,BOOST_LOG_DYN_LINK>
    )
endif()
target_compile_definitions(async_mqtt_separate PRIVATE ASYNC_MQTT_SEPARATE_COMPILATION)

macro(fix_msvc_build)
    message(STATUS "MSVC build fix applied")
    target_compile_options(async_mqtt_separate PRIVATE "/Zc:preprocessor" "/bigobj")
endmacro()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    fix_msvc_build()
endif()

target_link_libraries(async_mqtt_separate async_mqtt_iface)
