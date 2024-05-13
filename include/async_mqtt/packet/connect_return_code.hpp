// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_CONNECT_RETURN_CODE_HPP)
#define ASYNC_MQTT_PACKET_CONNECT_RETURN_CODE_HPP

#include <cstdint>
#include <ostream>


namespace async_mqtt {

/**
 * @ingroup connack_v3_1_1
 * @brief connect return code
 * See https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349256
 */
enum class connect_return_code : std::uint8_t {
    accepted                      = 0, ///< Connection accepted
    unacceptable_protocol_version = 1, ///< The Server does not support the level of the
                                       ///  MQTT protocol requested by the Client
    identifier_rejected           = 2, ///< The Client identifier is correct UTF-8 but not allowed by the Server
    server_unavailable            = 3, ///< The Network Connection has been made but the MQTT service is unavailable
    bad_user_name_or_password     = 4, ///< The data in the user name or password is malformed
    not_authorized                = 5, ///< The Client is not authorized to connect
};

constexpr
char const* connect_return_code_to_str(connect_return_code v) {
    char const * const str[] = {
        "accepted",
        "unacceptable_protocol_version",
        "identifier_rejected",
        "server_unavailable",
        "bad_user_name_or_password",
        "not_authorized"
    };
    if (static_cast<std::uint8_t>(v) < sizeof(str)) return str[static_cast<std::uint8_t>(v)];
    return "unknown_connect_return_code";
}

inline
std::ostream& operator<<(std::ostream& os, connect_return_code val)
{
    os << connect_return_code_to_str(val);
    return os;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_CONNECT_RETURN_CODE_HPP
