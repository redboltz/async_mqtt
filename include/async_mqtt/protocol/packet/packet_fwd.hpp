// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_PACKET_FWD_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_PACKET_FWD_HPP

#include <cstddef>

namespace async_mqtt {

namespace v3_1_1 {

class connack_packet;
class connect_packet;
template <std::size_t PacketIdBytes> class basic_publish_packet;
template <std::size_t PacketIdBytes> class basic_puback_packet;
template <std::size_t PacketIdBytes> class basic_pubrec_packet;
template <std::size_t PacketIdBytes> class basic_pubrel_packet;
template <std::size_t PacketIdBytes> class basic_pubcomp_packet;
template <std::size_t PacketIdBytes> class basic_subscribe_packet;
template <std::size_t PacketIdBytes> class basic_suback_packet;
template <std::size_t PacketIdBytes> class basic_unsubscribe_packet;
template <std::size_t PacketIdBytes> class basic_unsuback_packet;
class pingreq_packet;
class pingresp_packet;
class disconnect_packet;
using publish_packet = basic_publish_packet<2>;
using puback_packet = basic_puback_packet<2>;
using pubrec_packet = basic_pubrec_packet<2>;
using pubrel_packet = basic_pubrel_packet<2>;
using pubcomp_packet = basic_pubcomp_packet<2>;
using subscribe_packet = basic_subscribe_packet<2>;
using suback_packet = basic_suback_packet<2>;
using unsubscribe_packet = basic_unsubscribe_packet<2>;
using unsuback_packet = basic_unsuback_packet<2>;

} // namespace v3_1_1

namespace v5 {

class connack_packet;
class connect_packet;
template <std::size_t PacketIdBytes> class basic_publish_packet;
template <std::size_t PacketIdBytes> class basic_puback_packet;
template <std::size_t PacketIdBytes> class basic_pubrec_packet;
template <std::size_t PacketIdBytes> class basic_pubrel_packet;
template <std::size_t PacketIdBytes> class basic_pubcomp_packet;
template <std::size_t PacketIdBytes> class basic_subscribe_packet;
template <std::size_t PacketIdBytes> class basic_suback_packet;
template <std::size_t PacketIdBytes> class basic_unsubscribe_packet;
template <std::size_t PacketIdBytes> class basic_unsuback_packet;
class pingreq_packet;
class pingresp_packet;
class disconnect_packet;
class auth_packet;
using publish_packet = basic_publish_packet<2>;
using puback_packet = basic_puback_packet<2>;
using pubrec_packet = basic_pubrec_packet<2>;
using pubrel_packet = basic_pubrel_packet<2>;
using pubcomp_packet = basic_pubcomp_packet<2>;
using subscribe_packet = basic_subscribe_packet<2>;
using suback_packet = basic_suback_packet<2>;
using unsubscribe_packet = basic_unsubscribe_packet<2>;
using unsuback_packet = basic_unsuback_packet<2>;

} // namespace v5

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_PACKET_PACKET_FWD_HPP
