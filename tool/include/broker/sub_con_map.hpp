// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_SUB_CON_MAP_HPP)
#define ASYNC_MQTT_BROKER_SUB_CON_MAP_HPP

#include <broker/subscription_map.hpp>
#include <broker/subscription.hpp>

namespace async_mqtt {

template <typename Sp>
using sub_con_map = multiple_subscription_map<std::string, subscription<Sp>>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_SUB_CON_MAP_HPP
