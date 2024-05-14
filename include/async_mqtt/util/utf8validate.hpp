// Copyright 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//                     Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP)
#define ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP

#include <string_view>
#include <cstdint>

#include <boost/assert.hpp>

#include <async_mqtt/util/detail/utf8_checker.hpp>

namespace async_mqtt {

inline bool utf8string_check(std::string_view buf) {
    if (buf.empty()) return true;
    detail::utf8_checker c;
    if(! c.write(reinterpret_cast<std::uint8_t const*>(buf.data()), buf.size()))
        return false;
    return c.finish();
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP
