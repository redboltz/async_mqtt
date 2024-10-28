// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_HPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "restore_packets";
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void()
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl,
                std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
            ) {
                impl_type::async_restore_packets(
                    force_move(impl),
                    force_move(pvs),
                    force_move(handler)
                );
            },
            token,
            impl_,
            force_move(pvs)
        );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "restore_packets";
    BOOST_ASSERT(impl_);
    impl_->restore_packets(force_move(pvs));
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/endpoint_restore_packets.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_HPP
