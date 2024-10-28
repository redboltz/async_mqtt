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

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_connection_impl<Role, PacketIdBytes>::
basic_connection_impl(protocol_version ver)
    : protocol_version_{ver}
{
}

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

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_IMPL_IPP
