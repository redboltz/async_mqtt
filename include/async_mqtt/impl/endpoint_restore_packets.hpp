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

namespace detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
restore_packets_op {
    this_type_sp ep;
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs;
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
            a_ep.con_.restore_packets(force_move(pvs));
            self.complete();
            break;
        }
    }
};

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
) {
    con_.restore_packets(force_move(pvs));
}

} // namespace detail

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
        as::async_compose<
            CompletionToken,
            void()
        >(
            typename impl_type::restore_packets_op{
                impl_,
                force_move(pvs)
            },
            token,
            get_executor()
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

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_HPP
