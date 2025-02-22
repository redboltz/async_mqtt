// Copyright Takatoshi Kondo 2025
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_STATUS_IMPL_IPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_STATUS_IMPL_IPP

#include <ostream>

#include <async_mqtt/protocol/connection_status.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

ASYNC_MQTT_HEADER_ONLY_INLINE
constexpr
char const* connection_status_to_string(connection_status v) {
    switch (v) {
    case connection_status::connecting:   return "connecting";
    case connection_status::connected:    return "connected";
    case connection_status::disconnected: return "disconnected";
    default:                              return "unknown_connection_status";
    }
}

inline
std::ostream& operator<<(std::ostream& o, connection_status v)
{
    o << connection_status_to_string(v);
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_STATUS_IMPL_IPP
