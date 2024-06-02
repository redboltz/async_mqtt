// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_IMPL_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_IMPL_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

// classes

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint<Role, PacketIdBytes, NextLayer>::tim_cancelled {
    tim_cancelled(
        std::shared_ptr<as::steady_timer> tim,
        bool cancelled = false
    ):tim{force_move(tim)}, cancelled{cancelled}
    {}
    std::shared_ptr<as::steady_timer> tim;
    bool cancelled;
};

// member functions

// private

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
constexpr bool
basic_endpoint<Role, PacketIdBytes, NextLayer>::can_send_as_client(role r) {
    return
        static_cast<int>(r) &
        static_cast<int>(role::client);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
constexpr bool
basic_endpoint<Role, PacketIdBytes, NextLayer>::can_send_as_server(role r) {
    return static_cast<int>(r) & static_cast<int>(role::server);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename... Args>
basic_endpoint<Role, PacketIdBytes, NextLayer>::basic_endpoint(
    protocol_version ver,
    Args&&... args
): protocol_version_{ver},
   stream_{stream_type::create(std::forward<Args>(args)...)},
   store_{stream_->get_executor()},
   tim_pingreq_send_{std::make_shared<as::steady_timer>(stream_->get_executor())},
   tim_pingreq_recv_{std::make_shared<as::steady_timer>(stream_->get_executor())},
   tim_pingresp_recv_{std::make_shared<as::steady_timer>(stream_->get_executor())}
{
    BOOST_ASSERT(
        (Role == role::client && ver != protocol_version::undetermined) ||
        Role != role::client
    );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Other>
basic_endpoint<Role, PacketIdBytes, NextLayer>::basic_endpoint(
    basic_endpoint<Role, PacketIdBytes, Other>&& other
): protocol_version_{other.ver},
   stream_{force_move(other.stream_)},
   store_{stream_->get_executor()},
   tim_pingreq_send_{std::make_shared<as::steady_timer>(stream_->get_executor())},
   tim_pingreq_recv_{std::make_shared<as::steady_timer>(stream_->get_executor())},
   tim_pingresp_recv_{std::make_shared<as::steady_timer>(stream_->get_executor())}
{
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/endpoint_impl.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_IMPL_HPP
