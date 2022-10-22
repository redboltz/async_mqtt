file(GLOB HDR_ASYNC_MQTT "asunc_mqtt/*.hpp")
set(ROOT_ASYNC_MQTT_TARGET include/async_mqtt)

install(FILES ${HDR_ASYNC_MQTT} DESTINATION ${ROOT_ASYNC_MQTT_TARGET})
