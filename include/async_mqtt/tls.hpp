// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TLS_HPP)
#define ASYNC_MQTT_TLS_HPP

#if defined(ASYNC_MQTT_USE_TLS)

#if !defined(ASYNC_MQTT_TLS_INCLUDE)
#define ASYNC_MQTT_TLS_INCLUDE <boost/asio/ssl.hpp>
#endif // !defined(ASYNC_MQTT_TLS_INCLUDE)

#include ASYNC_MQTT_TLS_INCLUDE

#if !defined(ASYNC_MQTT_TLS_NS)
#define ASYNC_MQTT_TLS_NS boost::asio::ssl
#endif // !defined(ASYNC_MQTT_TLS_NS)

namespace async_mqtt {
namespace tls = ASYNC_MQTT_TLS_NS;
} // namespace async_mqtt


#if defined(ASYNC_MQTT_USE_WS)

#if !defined(ASYNC_MQTT_TLS_WS_INCLUDE)
#define ASYNC_MQTT_TLS_WS_INCLUDE <boost/beast/websocket/ssl.hpp>
#endif // !defined(ASYNC_MQTT_TLS_WS_INCLUDE)

#include ASYNC_MQTT_TLS_WS_INCLUDE

#endif // defined(ASYNC_MQTT_USE_WS)

#endif // defined(ASYNC_MQTT_USE_TLS)


#endif // ASYNC_MQTT_TLS_HPP
