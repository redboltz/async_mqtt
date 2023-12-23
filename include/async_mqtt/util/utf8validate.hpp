// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP)
#define ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP

#include <string_view>
#include <boost/beast/websocket/detail/utf8_checker.hpp>

namespace async_mqtt {

inline bool utf8string_check(std::string_view buf) {
    if (buf.empty()) return true;
    return boost::beast::websocket::detail::check_utf8(buf.data(), buf.size());
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP
