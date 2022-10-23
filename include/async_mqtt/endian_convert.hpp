// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDIAN_CONVERT_HPP)
#define ASYNC_MQTT_ENDIAN_CONVERT_HPP

#include <boost/endian/conversion.hpp>

namespace async_mqtt {

template <typename T>
T endian_load(char const* buf) {
    return boost::endian::endian_load<T, sizeof(T), boost::endian::order::big>(buf);
}

template <typename T>
void endian_store(T val, char* buf) {
    return boost::endian::endian_load<T, sizeof(T), boost::endian::order::big>(buf, val);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDIAN_CONVERT_HPP
