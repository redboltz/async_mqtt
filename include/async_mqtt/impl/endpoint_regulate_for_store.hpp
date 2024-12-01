// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_REGULATE_FOR_STORE_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_REGULATE_FOR_STORE_HPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_regulate_for_store(
    v5::basic_publish_packet<PacketIdBytes> packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "regulate_for_store:" << packet;
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void(error_code)
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl,
                v5::basic_publish_packet<PacketIdBytes> packet
            ) {
                impl_type::async_regulate_for_store(
                    force_move(impl),
                    force_move(handler),
                    force_move(packet)
                );
            },
            token,
            impl_,
            force_move(packet)
        );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::regulate_for_store(
    v5::basic_publish_packet<PacketIdBytes>& packet,
    error_code& ec
) const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "regulate_for_store:" << packet;
    BOOST_ASSERT(impl_);
    impl_->regulate_for_store(packet, ec);
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/endpoint_regulate_for_store.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_REGULATE_FOR_STORE_HPP
