// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::
restore_packets_op {
    this_type& ep;
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs;
    enum { dispatch, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        switch (state) {
        case dispatch: {
            state = complete;
            auto& a_ep{ep};
            as::dispatch(
                as::bind_executor(
                    a_ep.get_executor(),
                    force_move(self)
                )
            );
        } break;
        case complete:
            ep.restore_packets(force_move(pvs));
            self.complete();
            break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void()
)
basic_endpoint<Role, PacketIdBytes, NextLayer>::restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "restore_packets";
    return
        as::async_compose<
            CompletionToken,
            void()
        >(
            restore_packets_op{
                *this,
                force_move(pvs)
            },
            token
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
    for (auto& pv : pvs) {
        pv.visit(
            [&](auto& p) {
                if (pid_man_.register_id(p.packet_id())) {
                    store_.add(force_move(p));
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_impl", error)
                        << ASYNC_MQTT_ADD_VALUE(address, this)
                        << "packet_id:" << p.packet_id()
                        << " has already been used. Skip it";
                }
            }
        );
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RESTORE_PACKETS_HPP
