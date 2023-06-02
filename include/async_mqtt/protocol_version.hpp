// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_VERSION_HPP)
#define ASYNC_MQTT_PROTOCOL_VERSION_HPP

#include <cstdint>
#include <ostream>

/// @file

namespace async_mqtt {

/**
 * @brief MQTT protocol version
 */
enum class protocol_version {
    undetermined  = 0, ///< both v3.1.1 and v5.0 are accepted for broker (server)
    v3_1_1        = 4, ///< version 3.1.1
    v5            = 5, ///< version 5.0
};

/**
 * @brief stringize protocol_version
 */
constexpr char const* protocol_version_to_str(protocol_version v) {
    switch(v) {
    case protocol_version::undetermined: return "undetermined";
    case protocol_version::v3_1_1: return "v3_1_1";
    case protocol_version::v5: return "v5";
    default: return "unknown_protocol_version";
    }
}

/**
 * @brief output to the stream protocol_version
 */
inline
std::ostream& operator<<(std::ostream& os, protocol_version val)
{
    os << protocol_version_to_str(val);
    return os;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_VERSION_HPP
