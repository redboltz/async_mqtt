// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_HPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>
#include <async_mqtt/protocol/event/packet_id_released.hpp>

namespace async_mqtt {

namespace detail {

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
                        [&](event::basic_packet_id_released<PacketIdBytes> const& ev) {
                            // TBD naming? notify_packet_id_released
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

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
    con_.release_packet_id(packet_id);
}

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_release_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "release_packet_id pid:" << packet_id;
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void()
        >(
            typename impl_type::release_packet_id_op{
                impl_,
                packet_id
            },
            token,
            get_executor()
        );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::
release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "release_packet_id:" << packet_id;
    BOOST_ASSERT(impl_);
    impl_->release_packet_id(packet_id);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_HPP
