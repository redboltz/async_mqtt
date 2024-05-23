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

// public

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
basic_endpoint<Role, PacketIdBytes, NextLayer>::~basic_endpoint() {
    ASYNC_MQTT_LOG("mqtt_impl", trace)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "destroy";
}


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
std::set<typename basic_packet_id_type<PacketIdBytes>::type>
basic_endpoint<Role, PacketIdBytes, NextLayer>::get_qos2_publish_handled_pids() const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_qos2_publish_handled_pids";
    return qos2_publish_handled_;
}


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::restore_qos2_publish_handled_pids(
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "restore_qos2_publish_handled_pids";
    qos2_publish_handled_ = force_move(pids);
}



template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
protocol_version
basic_endpoint<Role, PacketIdBytes, NextLayer>::get_protocol_version() const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "get_protocol_version:" << protocol_version_;
    return protocol_version_;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
bool
basic_endpoint<Role, PacketIdBytes, NextLayer>::is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "is_publish_processing:" << pid;
    return qos2_publish_processing_.find(pid) != qos2_publish_processing_.end();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::set_pingreq_send_interval_ms(std::size_t ms) {
    pingreq_send_interval_ms_ = ms;
}

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
inline
std::optional<topic_alias_type>
basic_endpoint<Role, PacketIdBytes, NextLayer>::get_topic_alias(properties const& props) {
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


template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
bool
basic_endpoint<Role, PacketIdBytes, NextLayer>::enqueue_publish(
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
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::send_stored() {
    store_.for_each(
        [&](basic_store_packet_variant<PacketIdBytes> const& pv) {
            if (pv.size() > maximum_packet_size_send_) {
                release_pid(pv.packet_id());
                return false;
            }
            pv.visit(
                // copy packet because the stored packets need to be preserved
                // until receiving puback/pubrec/pubcomp
                overload {
                    [&](v3_1_1::basic_publish_packet<PacketIdBytes> p) {
                        async_send(
                            p,
                            [](system_error const&){}
                        );
                    },
                    [&](v5::basic_publish_packet<PacketIdBytes> p) {
                        if (enqueue_publish(p)) return;
                        async_send(
                            p,
                            [](system_error const&){}
                        );
                    },
                    [&](v3_1_1::basic_pubrel_packet<PacketIdBytes> p) {
                        async_send(
                            p,
                            [](system_error const&){}
                        );
                    },
                    [&](v5::basic_pubrel_packet<PacketIdBytes> p) {
                        async_send(
                            p,
                            [](system_error const&){}
                        );
                    }
                }
            );
            return true;
        }
    );
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::initialize() {
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
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::reset_pingreq_send_timer() {
    if (pingreq_send_interval_ms_) {
        tim_pingreq_send_->cancel();
        if (status_ == connection_status::disconnecting ||
            status_ == connection_status::closing ||
            status_ == connection_status::closed) return;
        tim_pingreq_send_->expires_after(
            std::chrono::milliseconds{*pingreq_send_interval_ms_}
        );
        tim_pingreq_send_->async_wait(
            [this, wp = std::weak_ptr{tim_pingreq_send_}](error_code const& ec) {
                if (!ec) {
                    if (auto sp = wp.lock()) {
                        switch (protocol_version_) {
                        case protocol_version::v3_1_1:
                            async_send(
                                v3_1_1::pingreq_packet(),
                                [](system_error const&){}
                            );
                            break;
                        case protocol_version::v5:
                            async_send(
                                v5::pingreq_packet(),
                                [](system_error const&){}
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
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::reset_pingreq_recv_timer() {
    if (pingreq_recv_timeout_ms_) {
        tim_pingreq_recv_->cancel();
        if (status_ == connection_status::disconnecting ||
            status_ == connection_status::closing ||
            status_ == connection_status::closed) return;
        tim_pingreq_recv_->expires_after(
            std::chrono::milliseconds{*pingreq_recv_timeout_ms_}
        );
        tim_pingreq_recv_->async_wait(
            [this, wp = std::weak_ptr{tim_pingreq_recv_}](error_code const& ec) {
                if (!ec) {
                    if (auto sp = wp.lock()) {
                        switch (protocol_version_) {
                        case protocol_version::v3_1_1:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "pingreq recv timeout. close.";
                            async_close(
                                []{}
                            );
                            break;
                        case protocol_version::v5:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "pingreq recv timeout. close.";
                            async_send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::keep_alive_timeout,
                                    properties{}
                                },
                                [this](system_error const&){
                                    async_close(
                                        []{}
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
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::reset_pingresp_recv_timer() {
    if (pingresp_recv_timeout_ms_) {
        tim_pingresp_recv_->cancel();
        if (status_ == connection_status::disconnecting ||
            status_ == connection_status::closing ||
            status_ == connection_status::closed) return;
        tim_pingresp_recv_->expires_after(
            std::chrono::milliseconds{*pingresp_recv_timeout_ms_}
        );
        tim_pingresp_recv_->async_wait(
            [this, wp = std::weak_ptr{tim_pingresp_recv_}](error_code const& ec) {
                if (!ec) {
                    if (auto sp = wp.lock()) {
                        switch (protocol_version_) {
                        case protocol_version::v3_1_1:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "pingresp recv timeout. close.";
                            async_close(
                                []{}
                            );
                            break;
                        case protocol_version::v5:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "pingresp recv timeout. close.";
                            if (status_ == connection_status::connected) {
                                async_send(
                                    v5::disconnect_packet{
                                        disconnect_reason_code::keep_alive_timeout,
                                        properties{}
                                    },
                                    [this](system_error const&){
                                        async_close(
                                            []{}
                                        );
                                    }
                                );
                            }
                            else {
                                async_close(
                                    []{}
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
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::notify_retry_one() {
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
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::complete_retry_one() {
    if (!tim_retry_acq_pid_queue_.empty()) {
        tim_retry_acq_pid_queue_.pop_front();
    }
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::notify_retry_all() {
    tim_retry_acq_pid_queue_.clear();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
bool
basic_endpoint<Role, PacketIdBytes, NextLayer>::has_retry() const {
    return !tim_retry_acq_pid_queue_.empty();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::clear_pid_man() {
    pid_man_.clear();
    notify_retry_all();
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
inline
void
basic_endpoint<Role, PacketIdBytes, NextLayer>::release_pid(typename basic_packet_id_type<PacketIdBytes>::type pid) {
    pid_man_.release_id(pid);
    notify_retry_one();
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_ENDPOINT_IMPL_HPP
