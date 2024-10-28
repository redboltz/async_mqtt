// Copyright Takatoshi Kondo 2025
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_SEPARATE_SRC_CONNECTION_HPP)
#define ASYNC_MQTT_SEPARATE_SRC_CONNECTION_HPP

#include <async_mqtt/protocol/impl/buffer_to_packet_variant.ipp>
#include <async_mqtt/protocol/impl/connection_impl.ipp>
#include <async_mqtt/protocol/impl/connection_send.ipp>
#include <async_mqtt/protocol/impl/timer_impl.ipp>
#include <async_mqtt/protocol/packet/impl/packet_variant.ipp>
#include <async_mqtt/protocol/packet/impl/property.ipp>
#include <async_mqtt/protocol/packet/impl/property_variant.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_connack.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_connect.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_disconnect.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_pingreq.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_pingresp.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_puback.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_pubcomp.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_publish.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_pubrec.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_pubrel.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_suback.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_subscribe.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_unsuback.ipp>
#include <async_mqtt/protocol/packet/impl/v3_1_1_unsubscribe.ipp>
#include <async_mqtt/protocol/packet/impl/v5_auth.ipp>
#include <async_mqtt/protocol/packet/impl/v5_connack.ipp>
#include <async_mqtt/protocol/packet/impl/v5_connect.ipp>
#include <async_mqtt/protocol/packet/impl/v5_disconnect.ipp>
#include <async_mqtt/protocol/packet/impl/v5_pingreq.ipp>
#include <async_mqtt/protocol/packet/impl/v5_pingresp.ipp>
#include <async_mqtt/protocol/packet/impl/v5_puback.ipp>
#include <async_mqtt/protocol/packet/impl/v5_pubcomp.ipp>
#include <async_mqtt/protocol/packet/impl/v5_publish.ipp>
#include <async_mqtt/protocol/packet/impl/v5_pubrec.ipp>
#include <async_mqtt/protocol/packet/impl/v5_pubrel.ipp>
#include <async_mqtt/protocol/packet/impl/v5_suback.ipp>
#include <async_mqtt/protocol/packet/impl/v5_subscribe.ipp>
#include <async_mqtt/protocol/packet/impl/v5_unsuback.ipp>
#include <async_mqtt/protocol/packet/impl/v5_unsubscribe.ipp>

#include <async_mqtt/protocol/impl/connection_instantiate_direct.hpp>

#endif // ASYNC_MQTT_SEPARATE_SRC_CONNECTION_HPP
