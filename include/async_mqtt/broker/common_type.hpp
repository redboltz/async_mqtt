// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_COMMON_TYPE_HPP)
#define ASYNC_MQTT_BROKER_COMMON_TYPE_HPP

#include <async_mqtt/packet/packet_id_type.hpp>

namespace async_mqtt {

using packet_id_t = typename packet_id_type<2>::type;

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_COMMON_TYPE_HPP
