// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_IPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>
#include <async_mqtt/protocol/event/packet_id_released.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
release_packet_id_op {
    this_type_sp ep;
    typename basic_packet_id_type<PacketIdBytes>::type packet_id;
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
        case complete: {
            auto events = a_ep.con_.release_packet_id(packet_id);
            for (auto& event : events) {
                std::visit(
                    overload {
                        [&](async_mqtt::event::basic_packet_id_released<PacketIdBytes> const& ev) {
                            a_ep.notify_release_pid(ev.get());
                        },
                        [](auto const&) {
                            BOOST_ASSERT(false);
                        }
                    },
                    event
                );
            }
            state = complete;
            self.complete();
        } break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
async_release_packet_id(
    this_type_sp impl,
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
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
        release_packet_id_op{
            force_move(impl),
            packet_id
        },
        handler,
        exe
    );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
    con_.release_packet_id(packet_id);
}

} // namespace async_mqtt::detail

#include <async_mqtt/impl/endpoint_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_IPP
