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
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_auto_pub_response(bool val) {
    auto_pub_response_ = val;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_auto_ping_response(bool val) {
    auto_ping_response_ = val;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_auto_map_topic_alias_send(bool val) {
    auto_map_topic_alias_send_ = val;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_auto_replace_topic_alias_send(bool val) {
    auto_replace_topic_alias_send_ = val;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_pingresp_recv_timeout(
    std::chrono::milliseconds duration
) {
    if (duration == std::chrono::milliseconds::zero()) {
        pingresp_recv_timeout_ms_ = std::nullopt;
    }
    else {
        pingresp_recv_timeout_ms_.emplace(duration);
    }
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
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_bulk_read_buffer_size(std::size_t val) {
    stream_.set_bulk_read_buffer_size(val);
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::set<typename basic_packet_id_type<PacketIdBytes>::type>
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::get_qos2_publish_handled_pids() const {
    return qos2_publish_handled_;
}


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::restore_qos2_publish_handled_pids(
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids
) {
    qos2_publish_handled_ = force_move(pids);
}



template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
protocol_version
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::get_protocol_version() const {
    return protocol_version_;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const {
    return qos2_publish_processing_.find(pid) != qos2_publish_processing_.end();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::set_pingreq_send_interval(
    this_type_sp ep,
    std::chrono::milliseconds duration
) {
    if (duration == std::chrono::milliseconds::zero()) {
        ep->pingreq_send_interval_ms_.reset();
        ep->tim_pingreq_send_.cancel();
    }
    else {
        ep->pingreq_send_interval_ms_.emplace(duration);
        reset_pingreq_send_timer(force_move(ep));
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
        if (publish_send_count_ == publish_send_max_) {
            publish_queue_.push_back(force_move(packet));
            return true;
        }
        else {
            ++publish_send_count_;
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
    publish_send_count_ = 0;
    publish_queue_.clear();
    topic_alias_send_ = std::nullopt;
    topic_alias_recv_ = std::nullopt;
    publish_recv_.clear();
    qos2_publish_processing_.clear();
    need_store_ = false;
    pid_suback_.clear();
    pid_unsuback_.clear();
    pid_puback_.clear();
    pid_pubrec_.clear();
    pid_pubcomp_.clear();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::reset_pingreq_send_timer(
    this_type_sp ep
) {
    if constexpr (Role == role::client || Role == role::any) {
        if (ep->pingreq_send_interval_ms_) {
            ep->tim_pingreq_send_.cancel();
            if (ep->status_ == connection_status::disconnecting ||
                ep->status_ == connection_status::closing ||
                ep->status_ == connection_status::closed) return;
            ep->tim_pingreq_send_.expires_after(
                *ep->pingreq_send_interval_ms_
            );
            ep->tim_pingreq_send_.async_wait(
                [wp = std::weak_ptr{ep}](error_code const& ec) {
                    if (!ec) {
                        if (auto ep = wp.lock()) {
                            switch (ep->protocol_version_) {
                            case protocol_version::v3_1_1:
                                async_send(
                                    ep,
                                    v3_1_1::pingreq_packet(),
                                    as::detached
                                );
                                break;
                            case protocol_version::v5:
                                async_send(
                                    ep,
                                    v5::pingreq_packet(),
                                    as::detached
                                );
                                break;
                            default:
                                BOOST_ASSERT(false);
                                break;
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
    this_type_sp ep
) {
    if (ep->pingreq_recv_timeout_ms_) {
        ep->tim_pingreq_recv_.cancel();
        if (ep->status_ == connection_status::disconnecting ||
            ep->status_ == connection_status::closing ||
            ep->status_ == connection_status::closed) return;
        ep->tim_pingreq_recv_.expires_after(
            *ep->pingreq_recv_timeout_ms_
        );
        ep->tim_pingreq_recv_.async_wait(
            [wp = std::weak_ptr{ep}](error_code const& ec) {
                if (!ec) {
                    if (auto ep = wp.lock()) {
                        switch (ep->protocol_version_) {
                        case protocol_version::v3_1_1:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, ep.get())
                                << "pingreq recv timeout. close.";
                            async_close(
                                ep,
                                as::detached
                            );
                            break;
                        case protocol_version::v5:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, ep.get())
                                << "pingreq recv timeout. close.";
                            async_send(
                                ep,
                                v5::disconnect_packet{
                                    disconnect_reason_code::keep_alive_timeout,
                                    properties{}
                                },
                                [ep](error_code const&){
                                    async_close(
                                        ep,
                                        as::detached
                                    );
                                }
                            );
                            break;
                        default:
                            BOOST_ASSERT(false);
                            break;
                        }
                    }
                }
            }
        );
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::reset_pingresp_recv_timer(
    this_type_sp ep
) {
    if (ep->pingresp_recv_timeout_ms_) {
        ep->tim_pingresp_recv_.cancel();
        if (ep->status_ == connection_status::disconnecting ||
            ep->status_ == connection_status::closing ||
            ep->status_ == connection_status::closed) return;
        ep->tim_pingresp_recv_.expires_after(
            *ep->pingresp_recv_timeout_ms_
        );
        ep->tim_pingresp_recv_.async_wait(
            [wp = std::weak_ptr{ep}](error_code const& ec) {
                if (!ec) {
                    if (auto ep = wp.lock()) {
                        switch (ep->protocol_version_) {
                        case protocol_version::v3_1_1:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, ep.get())
                                << "pingresp recv timeout. close.";
                            async_close(
                                ep,
                                as::detached
                            );
                            break;
                        case protocol_version::v5:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, ep.get())
                                << "pingresp recv timeout. close.";
                            if (ep->status_ == connection_status::connected) {
                                async_send(
                                    ep,
                                    v5::disconnect_packet{
                                        disconnect_reason_code::keep_alive_timeout,
                                        properties{}
                                    },
                                    [ep](error_code const&){
                                        async_close(
                                            ep,
                                            as::detached
                                        );
                                    }
                                );
                            }
                            else {
                                async_close(
                                    ep,
                                    as::detached
                                );
                            }
                            break;
                        default:
                            BOOST_ASSERT(false);
                            break;
                        }
                    }
                }
            }
        );
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

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::clear_pid_man() {
    pid_man_.clear();
    notify_retry_all();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_endpoint_impl<Role, PacketIdBytes, NextLayer>::release_pid(typename basic_packet_id_type<PacketIdBytes>::type pid) {
    pid_man_.release_id(pid);
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
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_bulk_read_buffer_size(std::size_t val) {
    BOOST_ASSERT(impl_);
    impl_->set_bulk_read_buffer_size(val);
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
