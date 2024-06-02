// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_ENDPOINT_SEND_IPP)
#define ASYNC_MQTT_IMPL_ENDPOINT_SEND_IPP

#include <async_mqtt/impl/endpoint_send.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint<Role, PacketIdBytes, NextLayer>::
send_op<Packet>::
validate_topic_alias_range(topic_alias_type ta) {
    if (!ep.topic_alias_send_) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << ASYNC_MQTT_ADD_VALUE(address, &ep)
            << "topic_alias is set but topic_alias_maximum is 0";
        return false;
    }
    if (ta == 0 || ta > ep.topic_alias_send_->max()) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << ASYNC_MQTT_ADD_VALUE(address, &ep)
            << "topic_alias is set but out of range";
        return false;
    }
    return true;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<std::string>
basic_endpoint<Role, PacketIdBytes, NextLayer>::
send_op<Packet>::
validate_topic_alias(std::optional<topic_alias_type> ta_opt) {
    if (!ta_opt) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << ASYNC_MQTT_ADD_VALUE(address, &ep)
            << "topic is empty but topic_alias isn't set";
        return std::nullopt;
    }

    if (!validate_topic_alias_range(*ta_opt)) {
        return std::nullopt;
    }

    auto topic = ep.topic_alias_send_->find(*ta_opt);
    if (topic.empty()) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << ASYNC_MQTT_ADD_VALUE(address, &ep)
            << "topic is empty but topic_alias is not registered";
        return std::nullopt;
    }
    return topic;
}

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
template <typename Packet>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool
basic_endpoint<Role, PacketIdBytes, NextLayer>::
send_op<Packet>::
validate_maximum_packet_size(std::size_t size) {
    if (size > ep.maximum_packet_size_send_) {
        ASYNC_MQTT_LOG("mqtt_impl", error)
            << ASYNC_MQTT_ADD_VALUE(address, &ep)
            << "packet size over maximum_packet_size for sending";
        return false;
    }
    return true;
}

} // namespace async_mqtt

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#include <async_mqtt/detail/instantiate_helper.hpp>

#define ASYNC_MQTT_INSTANTIATE_EACH_PACKET(a_role, a_size, a_protocol, a_packet) \
namespace async_mqtt { \
template \
bool \
basic_endpoint<a_role, a_size, a_protocol>::send_op<a_packet>:: \
validate_topic_alias_range(topic_alias_type); \
\
template \
std::optional<std::string> \
basic_endpoint<a_role, a_size, a_protocol>::send_op<a_packet>:: \
validate_topic_alias(std::optional<topic_alias_type>); \
\
template \
bool \
basic_endpoint<a_role, a_size, a_protocol>::send_op<a_packet>:: \
validate_maximum_packet_size(std::size_t); \
} // namespace async_mqtt

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH_PACKET \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(
    ASYNC_MQTT_PP_GENERATE,
    (ASYNC_MQTT_PP_ROLE)(ASYNC_MQTT_PP_SIZE)(ASYNC_MQTT_PP_PROTOCOL)(ASYNC_MQTT_PP_PACKET)
)

#define ASYNC_MQTT_PP_GENERATE_BASIC(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH_PACKET \
        BOOST_PP_SEQ_TO_TUPLE( \
            BOOST_PP_SEQ_REPLACE( \
                product, \
                BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(product)), \
                ASYNC_MQTT_PP_BASIC_PACKET_INSTANTIATE( \
                    BOOST_PP_SEQ_ELEM(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(product)), product), \
                    BOOST_PP_SEQ_ELEM(1, product) \
                ) \
            ) \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE_BASIC, (ASYNC_MQTT_PP_ROLE)(ASYNC_MQTT_PP_SIZE)(ASYNC_MQTT_PP_PROTOCOL)(ASYNC_MQTT_PP_BASIC_PACKET))


#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_PP_GENERATE_BASIC
#undef ASYNC_MQTT_INSTANTIATE_EACH_PACKET

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_ENDPOINT_SEND_IPP
