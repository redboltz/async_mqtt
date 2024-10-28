// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_REGISTER_PACKET_ID_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_REGISTER_PACKET_ID_HPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/impl/client_impl.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client<Version, NextLayer>::async_register_packet_id(
    packet_id_type pid,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void(error_code)
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl,
                packet_id_type pid
            ) {
                impl_type::async_register_packet_id(
                    force_move(impl),
                    pid,
                    force_move(handler)
                );
            },
            token,
            impl_,
            pid
        );
}

// sync version

template <protocol_version Version, typename NextLayer>
inline
bool
client<Version, NextLayer>::register_packet_id(packet_id_type packet_id) {
    BOOST_ASSERT(impl_);
    return impl_->register_packet_id(packet_id);
}


} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_REGISTER_PACKET_ID_HPP
