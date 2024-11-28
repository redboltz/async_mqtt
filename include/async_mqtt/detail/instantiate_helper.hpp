// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_DETAIL_INSTANTIATE_HELPER_HPP)
#define ASYNC_MQTT_DETAIL_INSTANTIATE_HELPER_HPP

#include <boost/preprocessor/seq/for_each_product.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>
#include <boost/preprocessor/seq/push_back.hpp>
#include <boost/preprocessor/seq/replace.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/replace.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/cat.hpp>

#include <async_mqtt/protocol/role.hpp>

#if !defined(ASYNC_MQTT_PP_ROLE)
#define ASYNC_MQTT_PP_ROLE (role::client)(role::server)(role::any)
#endif // !defined(ASYNC_MQTT_PP_ROLE)

#if !defined(ASYNC_MQTT_PP_SIZE)
#define ASYNC_MQTT_PP_SIZE (2)(4)
#endif // !defined(ASYNC_MQTT_PP_SIZE)

#if !defined(ASYNC_MQTT_PP_PROTOCOL)

#if defined(ASYNC_MQTT_USE_TLS)
#  if defined(ASYNC_MQTT_USE_WS)

#include <async_mqtt/predefined_layer/wss.hpp>
#define ASYNC_MQTT_PP_PROTOCOL (protocol::mqtt)(protocol::mqtts)(protocol::ws)(protocol::wss)

#  else  // defined(ASYNC_MQTT_USE_WS)

#include <async_mqtt/predefined_layer/mqtts.hpp>
#define ASYNC_MQTT_PP_PROTOCOL (protocol::mqtt)(protocol::mqtts)

#  endif // defined(ASYNC_MQTT_USE_WS)

#else  // defined(ASYNC_MQTT_USE_TLS)

#  if defined(ASYNC_MQTT_USE_WS)

#include <async_mqtt/predefined_layer/ws.hpp>
#define ASYNC_MQTT_PP_PROTOCOL (protocol::mqtt)(protocol::ws)

#  else  // defined(ASYNC_MQTT_USE_WS)

#include <async_mqtt/predefined_layer/mqtt.hpp>
#define ASYNC_MQTT_PP_PROTOCOL (protocol::mqtt)

#  endif // defined(ASYNC_MQTT_USE_WS)
#endif // defined(ASYNC_MQTT_USE_TLS)

#endif // !defined(ASYNC_MQTT_PP_PROTOCOL)

#define ASYNC_MQTT_PP_PACKET \
    (v3_1_1::connect_packet) \
    (v3_1_1::connack_packet) \
    (v3_1_1::pingreq_packet) \
    (v3_1_1::pingresp_packet) \
    (v3_1_1::disconnect_packet) \
    (v5::connect_packet) \
    (v5::connack_packet) \
    (v5::pingreq_packet) \
    (v5::pingresp_packet) \
    (v5::disconnect_packet) \
    (v5::auth_packet) \

#define ASYNC_MQTT_PP_BASIC_PACKET \
    (v3_1_1::basic_publish_packet) \
    (v3_1_1::basic_puback_packet) \
    (v3_1_1::basic_pubrec_packet) \
    (v3_1_1::basic_pubrel_packet) \
    (v3_1_1::basic_pubcomp_packet) \
    (v3_1_1::basic_subscribe_packet) \
    (v3_1_1::basic_suback_packet) \
    (v3_1_1::basic_unsubscribe_packet) \
    (v3_1_1::basic_unsuback_packet) \
    (v5::basic_publish_packet) \
    (v5::basic_puback_packet) \
    (v5::basic_pubrec_packet) \
    (v5::basic_pubrel_packet) \
    (v5::basic_pubcomp_packet) \
    (v5::basic_subscribe_packet) \
    (v5::basic_suback_packet) \
    (v5::basic_unsubscribe_packet) \
    (v5::basic_unsuback_packet) \
    (basic_packet_variant)

#define ASYNC_MQTT_PP_BASIC_PACKET_INSTANTIATE(name, n) name<n>

#if !defined(ASYNC_MQTT_PP_VERSION)
#define ASYNC_MQTT_PP_VERSION \
    (protocol_version::v3_1_1) \
    (protocol_version::v5)
#endif // !defined(ASYNC_MQTT_PP_VERSION)

#endif // ASYNC_MQTT_DETAIL_INSTANTIATE_HELPER_HPP
