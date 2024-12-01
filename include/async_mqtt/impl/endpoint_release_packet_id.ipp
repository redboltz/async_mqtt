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

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
    con_.release_packet_id(packet_id);
}

} // namespace async_mqtt::detail

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#include <async_mqtt/detail/instantiate_helper.hpp>

#define ASYNC_MQTT_INSTANTIATE_EACH(a_role, a_size, a_protocol) \
namespace async_mqtt { \
namespace detail { \
template \
class basic_endpoint_impl<a_role, a_size, a_protocol>; \
} \
template \
class basic_endpoint<a_role, a_size, a_protocol>; \
} // namespace async_mqtt

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_ROLE)(ASYNC_MQTT_PP_SIZE)(ASYNC_MQTT_PP_PROTOCOL))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_RELEASE_PACKET_ID_IPP
