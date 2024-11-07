// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_IMPL_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_IMPL_IPP

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

namespace detail {

// member functions

// public

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
                [&](event_timer const& ev) {
                    if (ev.get_timer_for() == timer::pingreq_send) {
                        switch (ev.get_op()) {
                        case event_timer::op_type::set:
                            set_pingreq_send_timer(ep, ev.get_ms());
                            break;
                        case event_timer::op_type::reset:
                            reset_pingreq_send_timer(ep, ev.get_ms());
                            break;
                        case event_timer::op_type::cancel:
                            cancel_pingreq_send_timer(ep);
                            break;
                        }
                    }
                    else {
                        BOOST_ASSERT(false);
                    }
                },
                [&](auto const&...) {
                    BOOST_ASSERT(false);
                }
            },
            event
        );
    }
}

// private

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<topic_alias_type>
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::get_topic_alias(properties const& props) {
    std::optional<topic_alias_type> ta_opt;
    for (auto const& prop : props) {
        prop.visit(
            overload {
                [&](property::topic_alias const& p) {
                    ta_opt.emplace(p.val());
                },
                [](auto const&) {
                }
            }
        );
        if (ta_opt) return ta_opt;
    }
    return ta_opt;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::enqueue_publish(
    v5::basic_publish_packet<PacketIdBytes>& packet
) {
    if (packet.opts().get_qos() == qos::at_least_once ||
        packet.opts().get_qos() == qos::exactly_once
    ) {
        if (con_.has_receive_maximum_vacancy_for_send()) {
            if (!publish_queue_.empty()) {
                publish_queue_.push_back(force_move(packet));
                return true;
            }
        }
        else {
            publish_queue_.push_back(force_move(packet));
            return true;
        }
    }
    return false;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::send_stored(this_type_sp ep) {
    ep->store_.for_each(
        [&](basic_store_packet_variant<PacketIdBytes> const& pv) {
            if (pv.size() > ep->maximum_packet_size_send_) {
                ep->release_pid(pv.packet_id());
                return false;
            }
            pv.visit(
                // copy packet because the stored packets need to be preserved
                // until receiving puback/pubrec/pubcomp
                overload {
                    [&](v3_1_1::basic_publish_packet<PacketIdBytes> p) {
                        async_send(
                            ep,
                            p,
                            as::detached
                        );
                    },
                    [&](v5::basic_publish_packet<PacketIdBytes> p) {
                        if (ep->enqueue_publish(p)) return;
                        async_send(
                            ep,
                            p,
                            as::detached
                        );
                    },
                    [&](v3_1_1::basic_pubrel_packet<PacketIdBytes> p) {
                        async_send(
                            ep,
                            p,
                            as::detached
                        );
                    },
                    [&](v5::basic_pubrel_packet<PacketIdBytes> p) {
                        async_send(
                            ep,
                            p,
                            as::detached
                        );
                    }
                }
            );
            return true;
        }
    );
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
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_pingreq_send_timer(
    this_type_sp ep,
    std::optional<std::chrono::milliseconds> ms
) {
    if constexpr (Role == role::client || Role == role::any) {
        if (ms) {
            ep->tim_pingreq_send_.expires_after(
                *ms
            );
            ep->tim_pingreq_send_.async_wait(
                [wp = std::weak_ptr{ep}](error_code const& ec) {
                    if (!ec) {
                        if (auto ep = wp.lock()) {
                            auto events = ep->con_.notify_timer_fired(timer::pingreq_send);
                            for (auto const& event : events) {
                                std::visit(
                                    overload {
                                        [&](event_timer const& ev) {
                                            if (ev.get_timer_for() == timer::pingreq_send) {
                                                switch (ev.get_op()) {
                                                case event_timer::op_type::set:
                                                    reset_pingreq_send_timer(ep, ev.get_ms());
                                                    break;
                                                case event_timer::op_type::reset:
                                                    reset_pingreq_send_timer(ep, ev.get_ms());
                                                    break;
                                                case event_timer::op_type::cancel:
                                                    ep->tim_pingreq_send_.cancel();
                                                    break;
                                                }
                                            }
                                            else {
                                                BOOST_ASSERT(false);
                                            }
                                        },
                                        [&](event_send& ev) {
                                            // must be pingreq packet here
                                            BOOST_ASSERT(!ev.get_release_packet_id_if_send_error());
                                            ep->stream_.async_write_packet(
                                                force_move(ev.get()),
                                                as::detached
                                            );
                                        },
                                        [&](auto const&...) {
                                            BOOST_ASSERT(false);
                                        }
                                    },
                                    event
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
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::reset_pingreq_send_timer(
    this_type_sp ep,
    std::optional<std::chrono::milliseconds> ms
) {
    cancel_pingreq_send_timer(ep);
    set_pingreq_send_timer(ep, ms);
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
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_pingreq_recv_timer(
    this_type_sp ep,
    std::optional<std::chrono::milliseconds> ms
) {
    if constexpr (Role == role::client || Role == role::any) {
        if (ms) {
            ep->tim_pingreq_recv_.expires_after(
                *ms
            );

            ep->tim_pingreq_recv_.async_wait(
                [wp = std::weak_ptr{ep}](error_code const& ec) {
                    if (!ec) {
                        if (auto ep = wp.lock()) {
                            auto events = ep->con_.notify_timer_fired(timer::pingreq_recv);
                            for (auto it = events.begin(); it != events.end();) {
                                auto& event = *it++;
                                std::visit(
                                    overload {
                                        [&](event_timer const& ev) {
                                            if (ev.get_timer_for() == timer::pingreq_recv) {
                                                switch (ev.get_op()) {
                                                case event_timer::op_type::set:
                                                    reset_pingreq_recv_timer(ep, ev.get_ms());
                                                    break;
                                                case event_timer::op_type::reset:
                                                    reset_pingreq_recv_timer(ep, ev.get_ms());
                                                    break;
                                                case event_timer::op_type::cancel:
                                                    ep->tim_pingreq_recv_.cancel();
                                                    break;
                                                }
                                            }
                                            else {
                                                BOOST_ASSERT(false);
                                            }
                                        },
                                        [&](event_send& ev) {
                                            auto pv{force_move(ev.get())};
                                            BOOST_ASSERT(pv.get_if<v5::disconnect_packet>());
                                            BOOST_ASSERT(it != events.end());
                                            auto& ev_close = *it++;
                                            BOOST_ASSERT(it == events.end());
                                            BOOST_ASSERT(std::get_if<event_close>(&ev_close));
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
                                        [&](event_close const&) {
                                            async_close(ep, as::detached);
                                        },
                                        [&](auto const&...) {
                                            BOOST_ASSERT(false);
                                        }
                                    },
                                    event
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
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::reset_pingreq_recv_timer(
    this_type_sp ep,
    std::optional<std::chrono::milliseconds> ms
) {
    cancel_pingreq_recv_timer(ep);
    set_pingreq_recv_timer(ep, ms);
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
    if constexpr (Role == role::server || Role == role::any) {
        if (ms) {
            ep->tim_pingresp_recv_.cancel();
            ep->tim_pingresp_recv_.expires_after(
                *ms
            );

            ep->tim_pingresp_recv_.async_wait(
                [wp = std::weak_ptr{ep}](error_code const& ec) {
                    if (!ec) {
                        if (auto ep = wp.lock()) {
                            auto events = ep->con_.notify_timer_fired(timer::pingresp_recv);
                            for (auto it = events.begin(); it != events.end();) {
                                auto& event = *it++;
                                std::visit(
                                    overload {
                                        [&](event_timer const& ev) {
                                            if (ev.get_timer_for() == timer::pingresp_recv) {
                                                switch (ev.get_op()) {
                                                case event_timer::op_type::set:
                                                    reset_pingresp_recv_timer(ep, ev.get_ms());
                                                    break;
                                                case event_timer::op_type::reset:
                                                    reset_pingresp_recv_timer(ep, ev.get_ms());
                                                    break;
                                                case event_timer::op_type::cancel:
                                                    ep->tim_pingresp_recv_.cancel();
                                                    break;
                                                }
                                            }
                                            else {
                                                BOOST_ASSERT(false);
                                            }
                                        },
                                        [&](event_send& ev) {
                                            auto pv{force_move(ev.get())};
                                            BOOST_ASSERT(pv.get_if<v5::disconnect_packet>());
                                            BOOST_ASSERT(it != events.end());
                                            auto& ev_close = *it++;
                                            BOOST_ASSERT(it == events.end());
                                            BOOST_ASSERT(std::get_if<event_close>(&ev_close));
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
                                        [&](event_close const&) {
                                            async_close(ep, as::detached);
                                        },
                                        [&](auto const&...) {
                                            BOOST_ASSERT(false);
                                        }
                                    },
                                    event
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

#if 0 // TBD
template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::clear_pid_man() {
    pid_man_.clear();
    notify_retry_all();
}
#endif

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

#endif // ASYNC_MQTT_IMPL_ENDPOINT_IMPL_IPP
