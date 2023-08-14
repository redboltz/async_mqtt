// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_HOST_PORT_HPP)
#define ASYNC_MQTT_HOST_PORT_HPP

#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/asio/ip/address.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/log.hpp>

namespace async_mqtt {

namespace as = boost::asio;

struct host_port {
    host_port(std::string host, std::uint16_t port)
        :host{force_move(host)},
         port{port}
    {}
    std::string host;
    std::uint16_t port;
};

inline std::string to_string(host_port const& hp) {
    return hp.host + ':' + std::to_string(hp.port);
}

inline std::ostream& operator<<(std::ostream& s, host_port const& hp) {
    s << to_string(hp);
    return s;
}

inline bool operator==(host_port const& lhs, host_port const& rhs) {
    return std::tie(lhs.host, lhs.port) == std::tie(rhs.host, rhs.port);
}

inline bool operator!=(host_port const& lhs, host_port const& rhs) {
    return !(lhs == rhs);
}

inline optional<host_port> host_port_from_string(std::string_view str) {
    // parse port ...:1234
    auto last_colon = str.find_last_of(':');
    if (last_colon == std::string::npos) return nullopt;

    std::uint16_t port;
    try {
        auto port_size = std::size_t(str.size() - (last_colon + 1));
        port = boost::lexical_cast<std::uint16_t>(
            std::string_view(&str[last_colon + 1], port_size)
        );
        str.remove_suffix(port_size + 1); // remove :1234
    }
    catch (std::exception const& e) {
        ASYNC_MQTT_LOG("mqtt_api", error)
            << "invalid host port string:" << str
            << " " << e.what();
        return nullopt;
    }

    if (str.empty()) return nullopt;
    if (str.front() == '[' && str.back() == ']') {
        // IPv6 [host]
        str.remove_prefix(1);
        str.remove_suffix(1);
        if (str.empty()) return nullopt;
    }

    return host_port{std::string(str), port};
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_HOST_PORT_HPP
