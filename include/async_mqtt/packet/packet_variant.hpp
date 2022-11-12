// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP

#include <async_mqtt/util/variant.hpp>
#include <async_mqtt/packet/v3_1_1_connect.hpp>
#include <async_mqtt/packet/v3_1_1_connack.hpp>
#include <async_mqtt/packet/v3_1_1_publish.hpp>
#include <async_mqtt/packet/v3_1_1_puback.hpp>
#include <async_mqtt/packet/v3_1_1_pubrec.hpp>
#include <async_mqtt/packet/v3_1_1_pubrel.hpp>
#include <async_mqtt/packet/v3_1_1_pubcomp.hpp>
#include <async_mqtt/packet/v3_1_1_subscribe.hpp>
#include <async_mqtt/packet/v3_1_1_suback.hpp>
#include <async_mqtt/packet/v3_1_1_unsubscribe.hpp>
#include <async_mqtt/packet/v3_1_1_unsuback.hpp>
#include <async_mqtt/packet/v3_1_1_pingreq.hpp>
#include <async_mqtt/packet/v3_1_1_pingresp.hpp>
#include <async_mqtt/packet/v3_1_1_disconnect.hpp>
#include <async_mqtt/packet/v5_connect.hpp>
#include <async_mqtt/packet/v5_connack.hpp>
#include <async_mqtt/packet/v5_publish.hpp>
#include <async_mqtt/packet/v5_puback.hpp>
#include <async_mqtt/packet/v5_pubrec.hpp>
#include <async_mqtt/packet/v5_pubrel.hpp>
#include <async_mqtt/packet/v5_pubcomp.hpp>
#include <async_mqtt/packet/v5_subscribe.hpp>
#include <async_mqtt/packet/v5_suback.hpp>
#include <async_mqtt/packet/v5_unsubscribe.hpp>
#include <async_mqtt/packet/v5_unsuback.hpp>
#include <async_mqtt/packet/v5_pingreq.hpp>
#include <async_mqtt/packet/v5_pingresp.hpp>
#include <async_mqtt/packet/v5_disconnect.hpp>
#include <async_mqtt/packet/v5_auth.hpp>
#include <async_mqtt/exception.hpp>

namespace async_mqtt {

template <std::size_t PacketIdBytes>
class basic_packet_variant {
public:
    basic_packet_variant() = default;
    template <
        typename Packet,
        std::enable_if_t<
            !std::is_same_v<
                std::decay_t<Packet>,
                basic_packet_variant<PacketIdBytes>
            >
        >* = nullptr
    >
    basic_packet_variant(Packet&& packet):var_{std::forward<Packet>(packet)}
    {}

    template <typename Func>
    auto visit(Func&& func) const {
        return
            async_mqtt::visit(
                std::forward<Func>(func),
                var_
            );
    }

    template <typename Func>
    auto visit(Func&& func) {
        return
            async_mqtt::visit(
                std::forward<Func>(func),
                var_
            );
    }

    std::vector<as::const_buffer> const_buffer_sequence() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.const_buffer_sequence();
                },
                [] (system_error const&) {
                    BOOST_ASSERT(false);
                    return std::vector<as::const_buffer>{};
                }
            }
        );
    }

    operator bool() {
        return var_.index() != 0;
    }

private:
    using variant_t = variant<
        system_error,
        v3_1_1::connect_packet,
        v3_1_1::connack_packet,
        v3_1_1::basic_publish_packet<PacketIdBytes>,
        v3_1_1::basic_puback_packet<PacketIdBytes>,
        v3_1_1::basic_pubrec_packet<PacketIdBytes>,
        v3_1_1::basic_pubrel_packet<PacketIdBytes>,
        v3_1_1::basic_pubcomp_packet<PacketIdBytes>,
        v3_1_1::basic_subscribe_packet<PacketIdBytes>,
        v3_1_1::basic_suback_packet<PacketIdBytes>,
        v3_1_1::basic_unsubscribe_packet<PacketIdBytes>,
        v3_1_1::basic_unsuback_packet<PacketIdBytes>,
        v3_1_1::pingreq_packet,
        v3_1_1::pingresp_packet,
        v3_1_1::disconnect_packet,
        v5::connect_packet,
        v5::connack_packet,
        v5::basic_publish_packet<PacketIdBytes>,
        v5::basic_puback_packet<PacketIdBytes>,
        v5::basic_pubrec_packet<PacketIdBytes>,
        v5::basic_pubrel_packet<PacketIdBytes>,
        v5::basic_pubcomp_packet<PacketIdBytes>,
        v5::basic_subscribe_packet<PacketIdBytes>,
        v5::basic_suback_packet<PacketIdBytes>,
        v5::basic_unsubscribe_packet<PacketIdBytes>,
        v5::basic_unsuback_packet<PacketIdBytes>,
        v5::pingreq_packet,
        v5::pingresp_packet,
        v5::disconnect_packet,
        v5::auth_packet
>;

    variant_t var_;
};

using packet_variant = basic_packet_variant<2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP
