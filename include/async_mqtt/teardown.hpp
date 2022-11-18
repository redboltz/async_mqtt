// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TEARDOWN_HPP)
#define ASYNC_MQTT_TEARDOWN_HPP

#include <boost/beast/websocket/teardown.hpp>

namespace async_mqtt {

namespace bs = boost::beast;

using bs::websocket::async_teardown;
using bs::role_type;

} // namespace async_mqtt

#endif // ASYNC_MQTT_TEARDOWN_HPP
