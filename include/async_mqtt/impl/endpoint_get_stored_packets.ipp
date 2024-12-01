// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_IPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/endpoint_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

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
            packets = a_ep.con_.get_stored_packets();
            self.complete(error_code{}, force_move(packets));
            break;
        }
    }
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::async_get_stored_packets(
    this_type_sp impl,
    as::any_completion_handler<
        void(error_code, std::vector<basic_store_packet_variant<PacketIdBytes>>)
    > handler
) {
    auto exe = impl->get_executor();
    as::async_compose<
        as::any_completion_handler<
            void(error_code, std::vector<basic_store_packet_variant<PacketIdBytes>>)
        >,
        void(error_code, std::vector<basic_store_packet_variant<PacketIdBytes>>)
    >(
        get_stored_packets_op{
            force_move(impl)
        },
        handler,
        exe
    );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_store_packet_variant<PacketIdBytes>>
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::get_stored_packets() const {
    return con_.get_stored_packets();
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

#endif // ASYNC_MQTT_IMPL_ENDPOINT_GET_STORED_PACKETS_IPP
