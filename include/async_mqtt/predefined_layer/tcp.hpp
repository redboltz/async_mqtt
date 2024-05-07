// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_TCP_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_TCP_HPP

#include <boost/asio.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;

namespace protocol {

/**
 * @breif Type alias of Boost.Asio TCP socket
 */
using mqtt = as::basic_stream_socket<as::ip::tcp, as::any_io_executor>;

} // namespace protocol


} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_TCP_HPP
