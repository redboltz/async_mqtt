// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_IPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/asio_bind/impl/endpoint_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

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

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::async_restore_packets(
    this_type_sp impl,
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
    as::any_completion_handler<
        void()
    > handler
) {
    auto exe = impl->get_executor();
    as::async_compose<
        as::any_completion_handler<
            void()
        >,
        void()
    >(
        restore_packets_op{
            force_move(impl),
            force_move(pvs)
        },
        handler,
        exe
    );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
) {
    con_.restore_packets(force_move(pvs));
}

} // namespace async_mqtt::detail

#include <async_mqtt/asio_bind/impl/endpoint_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_IPP
