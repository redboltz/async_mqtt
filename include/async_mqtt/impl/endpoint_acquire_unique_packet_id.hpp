// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_ACQUIRE_UNIQUE_PACKET_ID_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_ACQUIRE_UNIQUE_PACKET_ID_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

namespace detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
acquire_unique_packet_id_op {
    this_type_sp ep;
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type> pid_opt = std::nullopt;
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
            pid_opt = a_ep.pid_man_.acquire_unique_id();
            state = complete;
            if (pid_opt) {
                self.complete(error_code{}, *pid_opt);
            }
            else {
                self.complete(
                    make_error_code(mqtt_error::packet_identifier_fully_used),
                    0
                );
            }
            break;
        }
    }
};

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::acquire_unique_packet_id() {
    return pid_man_.acquire_unique_id();
}

} // namespace detail

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename CompletionToken>
auto
basic_endpoint<Role, PacketIdBytes, NextLayer>::async_acquire_unique_packet_id(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "acquire_unique_packet_id";
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
        void(error_code, typename basic_packet_id_type<PacketIdBytes>::type)
        >(
            typename impl_type::acquire_unique_packet_id_op{
                impl_
            },
            token,
            get_executor()
        );
}

// sync version

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
basic_endpoint<Role, PacketIdBytes, NextLayer>::acquire_unique_packet_id() {
    BOOST_ASSERT(impl_);
    auto pid = impl_->acquire_unique_packet_id();
    if (pid) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "acquire_unique_packet_id:" << *pid;
    }
    else {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "acquire_unique_packet_id:full";
    }
    return pid;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_ACQUIRE_UNIQUE_PACKET_ID_HPP
