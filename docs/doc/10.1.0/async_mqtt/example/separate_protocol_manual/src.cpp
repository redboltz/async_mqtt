// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// example: overriding custom instantiate configuration
#define ASYNC_MQTT_PP_ROLE (async_mqtt::role::client)
#define ASYNC_MQTT_PP_SIZE (2)

#include <async_mqtt/separate/src_connection.hpp>
