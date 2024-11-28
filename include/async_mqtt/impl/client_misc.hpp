// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_MISC_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_MISC_HPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/impl/client_impl.hpp>

namespace async_mqtt {
namespace mi = boost::multi_index;

namespace detail {

// member functions

template <protocol_version Version, typename NextLayer>
template <typename... Args>
client_impl<Version, NextLayer>::client_impl(
    Args&&... args
): ep_{Version, std::forward<Args>(args)...},
   tim_notify_publish_recv_{ep_.get_executor()}
{
    ep_.set_auto_pub_response(true);
    ep_.set_auto_ping_response(true);
}

template <protocol_version Version, typename NextLayer>
template <typename Other>
client_impl<Version, NextLayer>::client_impl(
    client_impl<Version, Other>&& other
): ep_{Version, force_move(other.next_layer())},
   tim_notify_publish_recv_{ep_.get_executor()}
{
    ep_.set_auto_pub_response(true);
    ep_.set_auto_ping_response(true);
}

template <protocol_version Version, typename NextLayer>
inline
as::any_io_executor
client_impl<Version, NextLayer>::get_executor() {
    return ep_.get_executor();
}

template <protocol_version Version, typename NextLayer>
inline
typename client_impl<Version, NextLayer>::next_layer_type const&
client_impl<Version, NextLayer>::next_layer() const {
    return ep_.next_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client_impl<Version, NextLayer>::next_layer_type&
client_impl<Version, NextLayer>::next_layer() {
    return ep_.next_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client_impl<Version, NextLayer>::lowest_layer_type const&
client_impl<Version, NextLayer>::lowest_layer() const {
    return ep_.lowest_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client_impl<Version, NextLayer>::lowest_layer_type&
client_impl<Version, NextLayer>::lowest_layer() {
    return ep_.lowest_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client_impl<Version, NextLayer>::endpoint_type const&
client_impl<Version, NextLayer>::get_endpoint() const {
    return ep_;
}

template <protocol_version Version, typename NextLayer>
inline
typename client_impl<Version, NextLayer>::endpoint_type&
client_impl<Version, NextLayer>::get_endpoint() {
    return ep_;
}

template <protocol_version Version, typename NextLayer>
inline
void
client_impl<Version, NextLayer>::set_auto_map_topic_alias_send(bool val) {
    ep_.set_auto_map_topic_alias_send(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client_impl<Version, NextLayer>::set_auto_replace_topic_alias_send(bool val) {
    ep_.set_auto_replace_topic_alias_send(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client_impl<Version, NextLayer>::set_pingresp_recv_timeout(
    std::chrono::milliseconds duration
) {
    ep_.set_pingresp_recv_timeout(duration);
}

template <protocol_version Version, typename NextLayer>
inline
void
client_impl<Version, NextLayer>::set_close_delay_after_disconnect_sent(
    std::chrono::milliseconds duration
) {
    ep_.set_close_delay_after_disconnect_sent(duration);
}

template <protocol_version Version, typename NextLayer>
inline
void
client_impl<Version, NextLayer>::set_bulk_write(bool val) {
    ep_.set_bulk_write(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client_impl<Version, NextLayer>::set_read_buffer_size(std::size_t val) {
    ep_.set_read_buffer_size(val);
}

} // namespace detail

// member functions

template <protocol_version Version, typename NextLayer>
template <typename... Args>
client<Version, NextLayer>::client(
    Args&&... args
): impl_{std::make_shared<impl_type>(std::forward<Args>(args)...)}
{
}

template <protocol_version Version, typename NextLayer>
template <typename Other>
client<Version, NextLayer>::client(
    client<Version, Other>&& other
): impl_{force_move(other.impl_)}
{
}

template <protocol_version Version, typename NextLayer>
inline
as::any_io_executor
client<Version, NextLayer>::get_executor() {
    BOOST_ASSERT(impl_);
    return impl_->get_executor();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::next_layer_type const&
client<Version, NextLayer>::next_layer() const {
    BOOST_ASSERT(impl_);
    return impl_->next_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::next_layer_type&
client<Version, NextLayer>::next_layer() {
    BOOST_ASSERT(impl_);
    return impl_->next_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::lowest_layer_type const&
client<Version, NextLayer>::lowest_layer() const {
    BOOST_ASSERT(impl_);
    return impl_->lowest_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::lowest_layer_type&
client<Version, NextLayer>::lowest_layer() {
    BOOST_ASSERT(impl_);
    return impl_->lowest_layer();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::endpoint_type const&
client<Version, NextLayer>::get_endpoint() const {
    BOOST_ASSERT(impl_);
    return impl_->get_endpoint();
}

template <protocol_version Version, typename NextLayer>
inline
typename client<Version, NextLayer>::endpoint_type&
client<Version, NextLayer>::get_endpoint() {
    BOOST_ASSERT(impl_);
    return impl_->get_endpoint();
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_auto_map_topic_alias_send(bool val) {
    BOOST_ASSERT(impl_);
    impl_->set_auto_map_topic_alias_send(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_auto_replace_topic_alias_send(bool val) {
    BOOST_ASSERT(impl_);
    impl_->set_auto_replace_topic_alias_send(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_pingresp_recv_timeout(
    std::chrono::milliseconds duration
) {
    BOOST_ASSERT(impl_);
    impl_->set_pingresp_recv_timeout(duration);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_close_delay_after_disconnect_sent(
    std::chrono::milliseconds duration
) {
    BOOST_ASSERT(impl_);
    impl_->set_close_delay_after_disconnect_sent(duration);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_bulk_write(bool val) {
    BOOST_ASSERT(impl_);
    impl_->set_bulk_write(val);
}

template <protocol_version Version, typename NextLayer>
inline
void
client<Version, NextLayer>::set_read_buffer_size(std::size_t val) {
    BOOST_ASSERT(impl_);
    impl_->set_read_buffer_size(val);
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/impl/client_misc.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_CLIENT_MISC_HPP
