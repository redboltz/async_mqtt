// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// example: overriding custom instantiate configuration
#define ASYNC_MQTT_PP_ROLE (async_mqtt::role::client)
#define ASYNC_MQTT_PP_SIZE (2)
#define ASYNC_MQTT_PP_PROTOCOL (async_mqtt::protocol::mqtt)

#include <async_mqtt/predefined_layer/mqtt.hpp>
#include <async_mqtt/separate/src_endpoint.hpp>
