// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_DETAIL_CLIENT_PACKET_TYPE_GETTER_HPP)
#define ASYNC_MQTT_DETAIL_CLIENT_PACKET_TYPE_GETTER_HPP

#include <boost/preprocessor/cat.hpp>

#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/protocol/packet/packet_fwd.hpp>

namespace async_mqtt::detail {

#define ASYNC_MQTT_PACKET_TYPE_GETTER(packet)                     \
    template <protocol_version Ver>                               \
    struct BOOST_PP_CAT(meta_,packet) {                           \
        using type = v5::BOOST_PP_CAT(packet, _packet);           \
    };                                                            \
    template <>                                                   \
    struct BOOST_PP_CAT(meta_,packet)<protocol_version::v3_1_1> { \
        using type = v3_1_1::BOOST_PP_CAT(packet, _packet);       \
    };

ASYNC_MQTT_PACKET_TYPE_GETTER(connect)
ASYNC_MQTT_PACKET_TYPE_GETTER(connack)
ASYNC_MQTT_PACKET_TYPE_GETTER(subscribe)
ASYNC_MQTT_PACKET_TYPE_GETTER(suback)
ASYNC_MQTT_PACKET_TYPE_GETTER(unsubscribe)
ASYNC_MQTT_PACKET_TYPE_GETTER(unsuback)
ASYNC_MQTT_PACKET_TYPE_GETTER(publish)
ASYNC_MQTT_PACKET_TYPE_GETTER(puback)
ASYNC_MQTT_PACKET_TYPE_GETTER(pubrec)
ASYNC_MQTT_PACKET_TYPE_GETTER(pubrel)
ASYNC_MQTT_PACKET_TYPE_GETTER(pubcomp)
ASYNC_MQTT_PACKET_TYPE_GETTER(pingreq)
ASYNC_MQTT_PACKET_TYPE_GETTER(pingresp)
ASYNC_MQTT_PACKET_TYPE_GETTER(disconnect)

#undef ASYNC_MQTT_PACKET_TYPE_GETTER

#define ASYNC_MQTT_PACKET_TYPE(version, packet)                   \
    using BOOST_PP_CAT(packet, _packet) = typename BOOST_PP_CAT(detail::meta_, packet<version>::type);

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_DETAIL_CLIENT_PACKET_TYPE_GETTER_HPP
