// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_STORE_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_PACKET_STORE_PACKET_VARIANT_HPP

#include <async_mqtt/util/variant.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/v3_1_1_publish.hpp>
#include <async_mqtt/packet/v3_1_1_pubrel.hpp>
#include <async_mqtt/packet/v5_publish.hpp>
#include <async_mqtt/packet/v5_pubrel.hpp>
#include <async_mqtt/exception.hpp>

namespace async_mqtt {

enum class response_packet {
    v3_1_1_puback,
    v3_1_1_pubrec,
    v3_1_1_pubcomp,
    v5_puback,
    v5_pubrec,
    v5_pubcomp,
};

template <std::size_t PacketIdBytes>
class basic_store_packet_variant {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    basic_store_packet_variant(v3_1_1::basic_publish_packet<PacketIdBytes> packet)
        :res_{
             [&] {
                 switch (packet.opts().qos()) {
                 case qos::at_least_once:
                     return response_packet::v3_1_1_puback;
                 case qos::exactly_once:
                     return response_packet::v3_1_1_pubrec;
                 default:
                     BOOST_ASSERT(false);
                     break;
                 }
             }()
         },
         var_{force_move(packet)}
    {}

    basic_store_packet_variant(v3_1_1::basic_pubrel_packet<PacketIdBytes> packet)
        :res_{response_packet::v3_1_1_pubcomp},
         var_{force_move(packet)}
    {}

    basic_store_packet_variant(v5::basic_publish_packet<PacketIdBytes> packet)
        :res_{
             [&] {
                 switch (packet.opts().qos()) {
                 case qos::at_least_once:
                     return response_packet::v5_puback;
                 case qos::exactly_once:
                     return response_packet::v5_pubrec;
                 default:
                     BOOST_ASSERT(false);
                     break;
                 }
             }()
         },
         var_{force_move(packet)}
    {}

    basic_store_packet_variant(v5::basic_pubrel_packet<PacketIdBytes> packet)
        :res_{response_packet::v5_pubcomp},
         var_{force_move(packet)}
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

    packet_id_t packet_id() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.packet_id();
                }
            }
        );
    }

    response_packet response_packet_type() const {
        return res_;
    }

private:
    using variant_t = variant<
        v3_1_1::basic_publish_packet<PacketIdBytes>,
        v3_1_1::basic_pubrel_packet<PacketIdBytes>,
        v5::basic_publish_packet<PacketIdBytes>,
        v5::basic_pubrel_packet<PacketIdBytes>
    >;

    response_packet res_;
    variant_t var_;
};

using store_packet_variant = basic_store_packet_variant<2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_STORE_PACKET_VARIANT_HPP
