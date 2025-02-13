// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_IMPL_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_IMPL_IPP

#include <async_mqtt/asio_bind/endpoint.hpp>
#include <async_mqtt/asio_bind/impl/endpoint_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

namespace detail {

// member functions

// public

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::underlying_accepted() {
    status_ = close_status::open;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_offline_publish(bool val) {
    con_.set_offline_publish(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_auto_pub_response(bool val) {
    con_.set_auto_pub_response(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_auto_ping_response(bool val) {
    con_.set_auto_ping_response(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_auto_map_topic_alias_send(bool val) {
    con_.set_auto_map_topic_alias_send(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_auto_replace_topic_alias_send(bool val) {
    con_.set_auto_replace_topic_alias_send(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_pingresp_recv_timeout(
    std::chrono::milliseconds duration
) {
    con_.set_pingresp_recv_timeout(duration);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_close_delay_after_disconnect_sent(
    std::chrono::milliseconds duration
) {
    duration_close_by_disconnect_ = duration;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_bulk_write(bool val) {
    stream_.set_bulk_write(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_read_buffer_size(std::size_t val) {
    read_buffer_size_ = val;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::set<typename basic_packet_id_type<PacketIdBytes>::type>
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::get_qos2_publish_handled_pids() const {
    return con_.get_qos2_publish_handled_pids();
}


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::restore_qos2_publish_handled_pids(
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids
) {
    con_.restore_qos2_publish_handled_pids(force_move(pids));
}


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
protocol_version
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::get_protocol_version() const {
    return con_.get_protocol_version();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const {
    return con_.is_publish_processing(pid);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_pingreq_send_interval(
    this_type_sp ep,
    std::chrono::milliseconds duration
) {
    auto events = ep->con_.set_pingreq_send_interval(duration);
    for (auto& event : events) {
        std::visit(
            overload {
                [&](async_mqtt::event::timer&& ev) {
                    if (ev.get_kind() == timer_kind::pingreq_send) {
                        switch (ev.get_op()) {
                        case timer_op::reset:
                            reset_pingreq_send_timer(ep, ev.get_ms());
                            break;
                        case timer_op::cancel:
                            cancel_pingreq_send_timer(ep);
                            break;
                        }
                    }
                    else {
                        BOOST_ASSERT(false);
                    }
                },
                [&](auto const&) {
                    BOOST_ASSERT(false);
                }
            },
            force_move(event)
        );
    }
}

// private

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::enqueue_publish(
    v5::basic_publish_packet<PacketIdBytes>& packet
) {
    if (packet.opts().get_qos() == qos::at_least_once ||
        packet.opts().get_qos() == qos::exactly_once
    ) {
        auto vacancy_opt = con_.get_receive_maximum_vacancy_for_send();
        if (vacancy_opt && *vacancy_opt == 0) {
            publish_queue_.push_back(force_move(packet));
            return true;
        }
        else {
            if (!publish_queue_.empty()) {
                publish_queue_.push_back(force_move(packet));
                return true;
            }
        }
    }
    return false;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::initialize() {
    // TBD where call from?
    publish_queue_.clear();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::reset_pingreq_send_timer(
    this_type_sp ep,
    std::optional<std::chrono::milliseconds> ms
) {
    cancel_pingreq_send_timer(ep);
    if constexpr (Role == role::client || Role == role::any) {
        if (ms) {
            ep->tim_pingreq_send_.expires_after(
                *ms
            );
            ep->tim_pingreq_send_.async_wait(
                [wp = std::weak_ptr{ep}](error_code const& ec) {
                    if (!ec) {
                        if (auto ep = wp.lock()) {
                            auto events = ep->con_.notify_timer_fired(timer_kind::pingreq_send);
                            for (auto& event : events) {
                                namespace event_ns = async_mqtt::event;
                                std::visit(
                                    overload {
                                        [&](event_ns::timer&& ev) {
                                            switch (ev.get_kind()) {
                                            case timer_kind::pingreq_send:
                                                switch (ev.get_op()) {
                                                case timer_op::reset:
                                                    reset_pingreq_send_timer(ep, ev.get_ms());
                                                    break;
                                                case timer_op::cancel:
                                                    ep->tim_pingreq_send_.cancel();
                                                    break;
                                                }
                                                break;
                                            case timer_kind::pingresp_recv:
                                                switch (ev.get_op()) {
                                                case timer_op::reset:
                                                    reset_pingresp_recv_timer(ep, ev.get_ms());
                                                    break;
                                                default:
                                                    BOOST_ASSERT(false);
                                                }
                                                break;
                                            default:
                                                BOOST_ASSERT(false);
                                                break;
                                            }
                                        },
                                        [&](event_ns::basic_send<PacketIdBytes>&& ev) {
                                            // must be pingreq packet here
                                            BOOST_ASSERT(!ev.get_release_packet_id_if_send_error());
                                            ep->stream_.async_write_packet(
                                                force_move(ev.get()),
                                                as::detached
                                            );
                                        },
                                        [&](auto const&) {
                                            BOOST_ASSERT(false);
                                        }
                                    },
                                    force_move(event)
                                );
                            }
                        }
                    }
                }
            );
        }
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::cancel_pingreq_send_timer(
    this_type_sp ep
) {
    ep->tim_pingreq_send_.cancel();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::reset_pingreq_recv_timer(
    this_type_sp ep,
    std::optional<std::chrono::milliseconds> ms
) {
    cancel_pingreq_recv_timer(ep);
    if constexpr (Role == role::server || Role == role::any) {
        if (ms) {
            ep->tim_pingreq_recv_.expires_after(
                *ms
            );

            ep->tim_pingreq_recv_.async_wait(
                [wp = std::weak_ptr{ep}](error_code const& ec) {
                    if (!ec) {
                        if (auto ep = wp.lock()) {
                            auto events = ep->con_.notify_timer_fired(timer_kind::pingreq_recv);
                            for (auto it = events.begin(); it != events.end();) {
                                auto& event = *it++;
                                std::visit(
                                    overload {
                                        [&](async_mqtt::event::timer&& ev) {
                                            if (ev.get_kind() == timer_kind::pingreq_recv) {
                                                switch (ev.get_op()) {
                                                case timer_op::reset:
                                                    reset_pingreq_recv_timer(ep, ev.get_ms());
                                                    break;
                                                case timer_op::cancel:
                                                    ep->tim_pingreq_recv_.cancel();
                                                    break;
                                                }
                                            }
                                            else {
                                                BOOST_ASSERT(false);
                                            }
                                        },
                                        [&](async_mqtt::event::basic_send<PacketIdBytes>&& ev) {
                                            auto pv{force_move(ev.get())};
                                            BOOST_ASSERT(pv.template get_if<v5::disconnect_packet>());
                                            BOOST_ASSERT(it != events.end());
                                            auto& ev_close = *it++;
                                            BOOST_ASSERT(it == events.end());
                                            BOOST_ASSERT(std::get_if<async_mqtt::event::close>(&ev_close));
                                            ep->stream_.async_write_packet(
                                                force_move(pv),
                                                [ep]
                                                (
                                                    error_code const& /*ec*/,
                                                    std::size_t /*bytes_transferred*/
                                                ) {
                                                    async_close(ep, as::detached);
                                                }
                                            );
                                        },
                                        [&](async_mqtt::event::close const&) {
                                            async_close(ep, as::detached);
                                        },
                                        [&](auto const&) {
                                            BOOST_ASSERT(false);
                                        }
                                    },
                                    force_move(event)
                                );
                            }
                        }
                    }
                }
            );
        }
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::cancel_pingreq_recv_timer(
    this_type_sp ep
) {
    ep->tim_pingreq_recv_.cancel();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::reset_pingresp_recv_timer(
    this_type_sp ep,
    std::optional<std::chrono::milliseconds> ms
) {
    if constexpr (Role == role::client || Role == role::any) {
        if (ms) {
            cancel_pingresp_recv_timer(ep);
            ep->tim_pingresp_recv_.expires_after(
                *ms
            );

            ep->tim_pingresp_recv_.async_wait(
                [wp = std::weak_ptr{ep}](error_code const& ec) {
                    if (!ec) {
                        if (auto ep = wp.lock()) {
                            auto events = ep->con_.notify_timer_fired(timer_kind::pingresp_recv);
                            for (auto it = events.begin(); it != events.end();) {
                                auto& event = *it++;
                                std::visit(
                                    overload {
                                        [&](async_mqtt::event::timer&& ev) {
                                            switch (ev.get_kind()) {
                                            case timer_kind::pingresp_recv:
                                                switch (ev.get_op()) {
                                                case timer_op::reset:
                                                    reset_pingresp_recv_timer(ep, ev.get_ms());
                                                    break;
                                                case timer_op::cancel:
                                                    ep->tim_pingresp_recv_.cancel();
                                                    break;
                                                }
                                                break;
                                            case timer_kind::pingreq_send:
                                                switch (ev.get_op()) {
                                                case timer_op::cancel:
                                                    ep->tim_pingreq_send_.cancel();
                                                    break;
                                                default:
                                                    BOOST_ASSERT(false);
                                                    break;
                                                }
                                                break;
                                            default:
                                                BOOST_ASSERT(false);
                                                break;
                                            }
                                        },
                                        [&](async_mqtt::event::basic_send<PacketIdBytes>&& ev) {
                                            auto pv{force_move(ev.get())};
                                            BOOST_ASSERT(pv.template get_if<v5::disconnect_packet>());
                                            BOOST_ASSERT(it != events.end());
                                            auto& ev_close = *it++;
                                            BOOST_ASSERT(it == events.end());
                                            BOOST_ASSERT(std::get_if<async_mqtt::event::close>(&ev_close));
                                            ep->stream_.async_write_packet(
                                                force_move(pv),
                                                [ep]
                                                (
                                                    error_code const& /*ec*/,
                                                    std::size_t /*bytes_transferred*/
                                                ) {
                                                    async_close(ep, as::detached);
                                                }
                                            );
                                        },
                                        [&](async_mqtt::event::close&&) {
                                            async_close(ep, as::detached);
                                        },
                                        [&](auto const&) {
                                            BOOST_ASSERT(false);
                                        }
                                    },
                                    force_move(event)
                                );
                            }
                        }
                    }
                }
            );
        }
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::cancel_pingresp_recv_timer(
    this_type_sp ep
) {
    ep->tim_pingresp_recv_.cancel();
}


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::notify_retry_one() {
    for (auto it = tim_retry_acq_pid_queue_.begin();
         it != tim_retry_acq_pid_queue_.end();
         ++it
    ) {
        if (it->cancelled) continue;
        it->tim->cancel();
        it->cancelled = true;
        return;
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::complete_retry_one() {
    if (!tim_retry_acq_pid_queue_.empty()) {
        tim_retry_acq_pid_queue_.pop_front();
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::notify_retry_all() {
    tim_retry_acq_pid_queue_.clear();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::has_retry() const {
    return !tim_retry_acq_pid_queue_.empty();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::
notify_release_pid(typename basic_packet_id_type<PacketIdBytes>::type /*pid*/) {
    packet_id_released_ = true;
    notify_retry_one();
}

} // namespace detail

// member functions

// public

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_endpoint<Role, PacketIdBytes, NextLayer>::~basic_endpoint() {
    ASYNC_MQTT_LOG("mqtt_impl", trace)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "destroy";
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::underlying_accepted() {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "underlying_accepted";
    BOOST_ASSERT(impl_);
    impl_->underlying_accepted();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_offline_publish(bool val) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "set_offline_publish val:" << val;
    BOOST_ASSERT(impl_);
    impl_->set_offline_publish(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_auto_pub_response(bool val) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "set_auto_pub_response val:" << val;
    BOOST_ASSERT(impl_);
    impl_->set_auto_pub_response(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_auto_ping_response(bool val) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "set_auto_ping_response val:" << val;
    BOOST_ASSERT(impl_);
    impl_->set_auto_ping_response(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_auto_map_topic_alias_send(bool val) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "set_auto_map_topic_alias_send val:" << val;
    BOOST_ASSERT(impl_);
    impl_->set_auto_map_topic_alias_send(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_auto_replace_topic_alias_send(bool val) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "set_auto_replace_topic_alias_send val:" << val;
    BOOST_ASSERT(impl_);
    impl_->set_auto_replace_topic_alias_send(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_pingresp_recv_timeout(
    std::chrono::milliseconds duration
) {
    BOOST_ASSERT(impl_);
    impl_->set_pingresp_recv_timeout(duration);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_close_delay_after_disconnect_sent(
    std::chrono::milliseconds duration
) {
    BOOST_ASSERT(impl_);
    impl_->set_close_delay_after_disconnect_sent(duration);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_bulk_write(bool val) {
    BOOST_ASSERT(impl_);
    impl_->set_bulk_write(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_read_buffer_size(std::size_t val) {
    BOOST_ASSERT(impl_);
    impl_->set_read_buffer_size(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::set<typename basic_packet_id_type<PacketIdBytes>::type>
basic_endpoint<Role, PacketIdBytes, NextLayer>::get_qos2_publish_handled_pids() const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_qos2_publish_handled_pids";
    BOOST_ASSERT(impl_);
    return impl_->get_qos2_publish_handled_pids();
}


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::restore_qos2_publish_handled_pids(
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "restore_qos2_publish_handled_pids";
    BOOST_ASSERT(impl_);
    impl_->restore_qos2_publish_handled_pids(force_move(pids));
}



template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
protocol_version
basic_endpoint<Role, PacketIdBytes, NextLayer>::get_protocol_version() const {
    auto protocol_version = impl_->get_protocol_version();
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_protocol_version:" << protocol_version;
    BOOST_ASSERT(impl_);
    return protocol_version;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint<Role, PacketIdBytes, NextLayer>::is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "is_publish_processing:" << pid;
    BOOST_ASSERT(impl_);
    return impl_->is_publish_processing(pid);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_pingreq_send_interval(
    std::chrono::milliseconds duration
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "set_pingreq_send_interval";
    BOOST_ASSERT(impl_);
    impl_type::set_pingreq_send_interval(
        impl_,
        duration
    );
}

} // namespace async_mqtt

#include <async_mqtt/asio_bind/impl/endpoint_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_ENDPOINT_IMPL_IPP
