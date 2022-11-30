// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_SESSION_STATE_FWD_HPP)
#define ASYNC_MQTT_BROKER_SESSION_STATE_FWD_HPP

#include <functional> // reference_wrapper

namespace async_mqtt {

template <typename... NextLayer>
struct session_state;

template <typename... NextLayer>
using session_state_ref = std::reference_wrapper<session_state<NextLayer...>>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_SESSION_STATE_FWD_HPP
