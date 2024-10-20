// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

namespace detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
get_stored_packets_op {
    this_type_sp ep;
    std::vector<basic_store_packet_variant<PacketIdBytes>> packets = {};
    enum { dispatch, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_ep{*ep};
        switch (state) {
        case dispatch: {
            state = complete;
            as::dispatch(
                a_ep.get_executor(),
                force_move(self)
            );
        } break;
        case complete:
            packets = a_ep.get_stored_packets();
            self.complete(error_code{}, force_move(packets));
            break;
        }
    }
};

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
std::vector<basic_store_packet_variant<PacketIdBytes>>
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::get_stored_packets() const {
    return store_.get_stored();
}

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_get_stored_packets(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_stored_packets";
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::vector<basic_store_packet_variant<PacketIdBytes>>)
        >(
            typename impl_type::get_stored_packets_op{
                impl_
            },
            token,
            get_executor()
        );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
std::vector<basic_store_packet_variant<PacketIdBytes>>
basic_endpoint<Role, PacketIdBytes, NextLayer>::get_stored_packets() const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_stored_packets";
    BOOST_ASSERT(impl_);
    return impl_->get_stored_packets();
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_HPP
