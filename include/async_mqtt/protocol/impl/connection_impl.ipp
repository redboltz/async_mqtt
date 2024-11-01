// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_IMPL_IPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_IMPL_IPP

#include <async_mqtt/protocol/connection.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

namespace detail {

// public
template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_connection_impl<Role, PacketIdBytes>::
basic_connection_impl(protocol_version ver)
    : protocol_version_{ver}
{
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_connection_impl<Role, PacketIdBytes>::
notify_timer_fired(timer kind) {
    switch (kind) {
    case timer::pingreq_send:
        switch (protocol_version_) {
        case protocol_version::v3_1_1:
            return send(v3_1_1::pingreq_packet());
            break;
        case protocol_version::v5:
            return send(v5::pingreq_packet());
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    default:
        BOOST_ASSERT(false);
        break;
    }
    return std::vector<basic_event_variant<PacketIdBytes>>{};
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
set_pingreq_send_interval(
    std::chrono::milliseconds duration,
    std::vector<basic_event_variant<PacketIdBytes>>& events
) {
    if (duration == std::chrono::milliseconds::zero()) {
        pingreq_send_interval_ms_.reset();
        events.emplace_back(
            event_timer{
                event_timer::op_type::cancel,
                timer::pingreq_send
            }
        );
    }
    else {
        pingreq_send_interval_ms_.emplace(duration);
        events.emplace_back(
            event_timer{
                event_timer::op_type::reset,
                timer::pingreq_send
            }
        );
    }
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
basic_connection_impl<Role, PacketIdBytes>::
acquire_unique_packet_id() {
    return pid_man_.acquire_unique_id();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection_impl<Role, PacketIdBytes>::
register_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    return pid_man_.register_id(packet_id);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
release_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    pid_man_.relase_id(packet_id);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::set<typename basic_packet_id_type<PacketIdBytes>::type>
basic_connection_impl<Role, PacketIdBytes>::
get_qos2_publish_handled_pids() const {
    return qos2_publish_handled_;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
restore_qos2_publish_handled_pids(
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids
) {
    qos2_publish_handled_ = force_move(pids);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
) {
    for (auto& pv : pvs) {
        pv.visit(
            [&](auto& p) {
                if (pid_man_.register_id(p.packet_id())) {
                    store_.add(force_move(p));
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_impl", error)
                        << "packet_id:" << p.packet_id()
                        << " has already been used. Skip it";
                }
            }
        );
    }
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_store_packet_variant<PacketIdBytes>>
basic_connection_impl<Role, PacketIdBytes>::
get_stored_packets() const {
    return store_.get_stored();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
protocol_version
basic_connection_impl<Role, PacketIdBytes>::
get_protocol_version() const {
    return protocol_version_;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
error_code
basic_connection_impl<Role, PacketIdBytes>::
regulate_for_store(
    v5::basic_publish_packet<PacketIdBytes>& packet
) const {
    if (packet.topic().empty()) {
        if (auto ta_opt = get_topic_alias(packet.props())) {
            auto topic = topic_alias_send_->find_without_touch(*ta_opt);
            if (topic.empty()) {
                return make_error_code(
                    mqtt_error::packet_not_regulated
                );
            }
            packet.remove_topic_alias_add_topic(force_move(topic));
        }
        else {
            return make_error_code(
                mqtt_error::packet_not_regulated
            );
        }
    }
    else {
        packet.remove_topic_alias();
    }
    return error_code{};
}

// private

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
initialize() {
    publish_send_count_ = 0;
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

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<std::string>
basic_connection_impl<Role, PacketIdBytes>::
validate_topic_alias(std::optional<topic_alias_type> ta_opt) {
    if (!ta_opt) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "topic is empty but topic_alias isn't set";
        return std::nullopt;
    }

    if (!validate_topic_alias_range(*ta_opt)) {
        return std::nullopt;
    }

    auto topic = topic_alias_send_->find(*ta_opt);
    if (topic.empty()) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "topic is empty but topic_alias is not registered";
        return std::nullopt;
    }
    return topic;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection_impl<Role, PacketIdBytes>::
validate_topic_alias_range(topic_alias_type ta) {
    if (!topic_alias_send_) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "topic_alias is set but topic_alias_maximum is 0";
        return false;
    }
    if (ta == 0 || ta > topic_alias_send_->max()) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "topic_alias is set but out of range";
        return false;
    }
    return true;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection_impl<Role, PacketIdBytes>::
validate_maximum_packet_size(std::size_t size) {
    if (size > maximum_packet_size_send_) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << "packet size over maximum_packet_size for sending";
        return false;
    }
    return true;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<topic_alias_type>
basic_connection_impl<Role, PacketIdBytes>::
get_topic_alias(properties const& props) {
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


} // namespace detail

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_connection<Role, PacketIdBytes>::
basic_connection(protocol_version ver)
    :
    impl_{
        std::make_shared<impl_type>(
            ver
        )
    }
{
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_connection<Role, PacketIdBytes>::
notify_timer_fired(timer kind) {
    BOOST_ASSERT(impl_);
    return impl_->notify_timer_fired(kind);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_connection<Role, PacketIdBytes>::
set_pingreq_send_interval(
    std::chrono::milliseconds duration
) {
    BOOST_ASSERT(impl_);
    std::vector<basic_event_variant<PacketIdBytes>> events;
    impl_->set_pingreq_send_interval(duration, events);
    return events;
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
basic_connection<Role, PacketIdBytes>::
acquire_unique_packet_id() {
    BOOST_ASSERT(impl_);
    return impl_->acquire_unique_packet_id();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_connection<Role, PacketIdBytes>::
register_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    BOOST_ASSERT(impl_);
    return impl_->register_packet_id(packet_id);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
release_packet_id(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    BOOST_ASSERT(impl_);
    return impl_->release_packet_id(packet_id);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::set<typename basic_packet_id_type<PacketIdBytes>::type>
basic_connection<Role, PacketIdBytes>::
get_qos2_publish_handled_pids() const {
    BOOST_ASSERT(impl_);
    return impl_->get_qos2_publish_handled_pids();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
restore_qos2_publish_handled_pids(
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids
) {
    BOOST_ASSERT(impl_);
    impl_->restore_qos2_publish_handled_pids(force_move(pids));
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection<Role, PacketIdBytes>::
restore_packets(
    std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
) {
    BOOST_ASSERT(impl_);
    impl_->restore_packets(force_move(pvs));
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_store_packet_variant<PacketIdBytes>>
basic_connection<Role, PacketIdBytes>::
get_stored_packets() const {
    BOOST_ASSERT(impl_);
    return impl_->get_stored_packets();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
protocol_version
basic_connection<Role, PacketIdBytes>::
get_protocol_version() const {
    BOOST_ASSERT(impl_);
    return impl_->get_protocol_version();
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
error_code
basic_connection<Role, PacketIdBytes>::
regulate_for_store(
    v5::basic_publish_packet<PacketIdBytes>& packet
) const {
    BOOST_ASSERT(impl_);
    return impl_->regulate_for_store(packet);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_IMPL_IPP
