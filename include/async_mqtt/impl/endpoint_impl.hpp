// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_IMPL_HPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_IMPL_HPP

#include <async_mqtt/endpoint.hpp>

namespace async_mqtt {

namespace detail {

// classes

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
struct basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::tim_cancelled {
    tim_cancelled(
        std::shared_ptr<as::steady_timer> tim,
        bool cancelled = false
    ):tim{force_move(tim)}, cancelled{cancelled}
    {}
    std::shared_ptr<as::steady_timer> tim;
    bool cancelled;
};

// member functions

// public

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename... Args>
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::basic_endpoint_impl(
    protocol_version ver,
    Args&&... args
): stream_{std::forward<Args>(args)...},
   con_{ver},
   tim_pingreq_send_{stream_.get_executor()},
   tim_pingreq_recv_{stream_.get_executor()},
   tim_pingresp_recv_{stream_.get_executor()},
   tim_close_by_disconnect_{stream_.get_executor()}
{
    BOOST_ASSERT(
        (Role == role::client && ver != protocol_version::undetermined) ||
        Role != role::client
    );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
as::any_io_executor
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::get_executor() {
    return stream_.get_executor();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
typename basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::next_layer_type const&
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::next_layer() const {
    return stream_.next_layer();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
typename basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::next_layer_type&
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::next_layer() {
    return stream_.next_layer();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
typename basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::lowest_layer_type const&
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::lowest_layer() const {
    return stream_.lowest_layer();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
typename basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::lowest_layer_type&
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::lowest_layer() {
    return stream_.lowest_layer();
}


// private

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Other>
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::basic_endpoint_impl(
    basic_endpoint_impl<Role, PacketIdBytes, Other>&& other
): stream_{force_move(other.stream_)},
   con_{force_move(other.con_)},
   tim_pingreq_send_{stream_.get_executor()},
   tim_pingreq_recv_{stream_.get_executor()},
   tim_pingresp_recv_{stream_.get_executor()}
{
}

} // namespace detail

// member functions

// public

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename... Args>
basic_endpoint<Role, PacketIdBytes, NextLayer>::basic_endpoint(
    protocol_version ver,
    Args&&... args
): impl_{
    std::make_shared<impl_type>(
        ver,
        std::forward<Args>(args)...
    )
}
{
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
as::any_io_executor
basic_endpoint<Role, PacketIdBytes, NextLayer>::get_executor() {
    return impl_->get_executor();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
typename basic_endpoint<Role, PacketIdBytes, NextLayer>::next_layer_type const&
basic_endpoint<Role, PacketIdBytes, NextLayer>::next_layer() const {
    return impl_->next_layer();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
typename basic_endpoint<Role, PacketIdBytes, NextLayer>::next_layer_type&
basic_endpoint<Role, PacketIdBytes, NextLayer>::next_layer() {
    return impl_->next_layer();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
typename basic_endpoint<Role, PacketIdBytes, NextLayer>::lowest_layer_type const&
basic_endpoint<Role, PacketIdBytes, NextLayer>::lowest_layer() const {
    return impl_->lowest_layer();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
typename basic_endpoint<Role, PacketIdBytes, NextLayer>::lowest_layer_type&
basic_endpoint<Role, PacketIdBytes, NextLayer>::lowest_layer() {
    return impl_->lowest_layer();
}

// private

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Other>
basic_endpoint<Role, PacketIdBytes, NextLayer>::basic_endpoint(
    basic_endpoint<Role, PacketIdBytes, Other>&& other
): impl_{std::move(other.impl_)}
{
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/endpoint_impl.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_IMPL_HPP
