// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_ENDIAN_CONVERT_HPP)
#define ASYNC_MQTT_UTIL_ENDIAN_CONVERT_HPP

#include <algorithm>

#include <boost/endian/conversion.hpp>

#include <async_mqtt/util/static_vector.hpp>

namespace async_mqtt {

template <typename T>
T endian_load(char const* buf) {
    static_vector<unsigned char, sizeof(T)> usbuf(sizeof(T));
    std::copy(buf, buf + sizeof(T), usbuf.data());
    return boost::endian::endian_load<T, sizeof(T), boost::endian::order::big>(usbuf.data());
}

template <typename T>
void endian_store(T val, char* buf) {
    static_vector<unsigned char, sizeof(T)> usbuf(sizeof(T));
    boost::endian::endian_store<T, sizeof(T), boost::endian::order::big>(usbuf.data(), val);
    std::copy(usbuf.begin(), usbuf.end(), buf);
}

template <typename T>
static_vector<char, sizeof(T)> endian_static_vector(T val) {
    static_vector<unsigned char, sizeof(T)> usbuf(sizeof(T));
    boost::endian::endian_store<T, sizeof(T), boost::endian::order::big>(usbuf.data(), val);
    static_vector<char, sizeof(T)> cbuf(sizeof(T));
    std::copy(usbuf.begin(), usbuf.end(), cbuf.begin());
    return cbuf;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_ENDIAN_CONVERT_HPP
