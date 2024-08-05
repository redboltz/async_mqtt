// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_SRC_HPP)
#define ASYNC_MQTT_SRC_HPP

#ifndef ASYNC_MQTT_SEPARATE_COMPILATION
#error You need to define ASYNC_MQTT_SEPARATE_COMPILATION in all translation units that use the compiled version of async_mqtt, \
    as well as the one where this file is included.
#endif

#include <async_mqtt/packet/impl/v3_1_1_connect.ipp>
#include <async_mqtt/packet/impl/v3_1_1_connack.ipp>
#include <async_mqtt/packet/impl/v3_1_1_subscribe.ipp>
#include <async_mqtt/packet/impl/v3_1_1_suback.ipp>
#include <async_mqtt/packet/impl/v3_1_1_unsubscribe.ipp>
#include <async_mqtt/packet/impl/v3_1_1_unsuback.ipp>
#include <async_mqtt/packet/impl/v3_1_1_publish.ipp>
#include <async_mqtt/packet/impl/v3_1_1_puback.ipp>
#include <async_mqtt/packet/impl/v3_1_1_pubrec.ipp>
#include <async_mqtt/packet/impl/v3_1_1_pubrel.ipp>
#include <async_mqtt/packet/impl/v3_1_1_pubcomp.ipp>
#include <async_mqtt/packet/impl/v3_1_1_pingreq.ipp>
#include <async_mqtt/packet/impl/v3_1_1_pingresp.ipp>
#include <async_mqtt/packet/impl/v3_1_1_disconnect.ipp>

#include <async_mqtt/packet/impl/v5_connect.ipp>
#include <async_mqtt/packet/impl/v5_connack.ipp>
#include <async_mqtt/packet/impl/v5_subscribe.ipp>
#include <async_mqtt/packet/impl/v5_suback.ipp>
#include <async_mqtt/packet/impl/v5_unsubscribe.ipp>
#include <async_mqtt/packet/impl/v5_unsuback.ipp>
#include <async_mqtt/packet/impl/v5_publish.ipp>
#include <async_mqtt/packet/impl/v5_puback.ipp>
#include <async_mqtt/packet/impl/v5_pubrec.ipp>
#include <async_mqtt/packet/impl/v5_pubrel.ipp>
#include <async_mqtt/packet/impl/v5_pubcomp.ipp>
#include <async_mqtt/packet/impl/v5_pingreq.ipp>
#include <async_mqtt/packet/impl/v5_pingresp.ipp>
#include <async_mqtt/packet/impl/v5_disconnect.ipp>
#include <async_mqtt/packet/impl/v5_auth.ipp>

#include <async_mqtt/packet/impl/packet_variant.ipp>
#include <async_mqtt/packet/impl/property_variant.ipp>

#include <async_mqtt/impl/buffer_to_packet_variant.ipp>
#include <async_mqtt/impl/client_impl.ipp>
#include <async_mqtt/impl/endpoint_impl.ipp>
#include <async_mqtt/impl/endpoint_recv.ipp>
#include <async_mqtt/impl/endpoint_send.ipp>

#endif // ASYNC_MQTT_SRC_HPP
