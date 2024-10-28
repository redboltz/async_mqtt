// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_CONNECTION_HPP)
#define ASYNC_MQTT_PROTOCOL_CONNECTION_HPP

#include <memory>

#include <async_mqtt/protocol/detail/connection_impl.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes>
class basic_connection {
    using impl_type = detail::basic_connection_impl<Role, PacketIdBytes>;

public:
    basic_connection(protocol_version ver);

    template <typename Packet>
    std::tuple<error_code, std::vector<basic_event_variant<PacketIdBytes>>>
    send(Packet packet);

private:
    std::shared_ptr<impl_type> impl_;
};

} // namespace async_mqtt

#include <async_mqtt/protocol/impl/connection_send.hpp>

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/connection_impl.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_CONNECTION_HPP
