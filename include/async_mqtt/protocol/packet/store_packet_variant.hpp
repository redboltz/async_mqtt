// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_STORE_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_STORE_PACKET_VARIANT_HPP

#include <variant>

#include <async_mqtt/util/overload.hpp>
#include <async_mqtt/protocol/packet/store_packet_variant_fwd.hpp>
#include <async_mqtt/protocol/packet/packet_variant.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>
#include <async_mqtt/protocol/packet/v3_1_1_publish.hpp>
#include <async_mqtt/protocol/packet/v3_1_1_pubrel.hpp>
#include <async_mqtt/protocol/packet/v5_publish.hpp>
#include <async_mqtt/protocol/packet/v5_pubrel.hpp>

namespace async_mqtt {

/**
 * @ingroup store_packet_variant
 * @brief corresponding response packet
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/store_packet_variant.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
enum class response_packet {
    v3_1_1_puback,  ///< stored packet is v3_1_1_basic_publish_packet QoS1
    v3_1_1_pubrec,  ///< stored packet is v3_1_1_basic_publish_packet QoS2
    v3_1_1_pubcomp, ///< stored packet is v3_1_1_basic_pubrel_packet
    v5_puback,      ///< stored packet is v5_basic_publish_packet QoS1
    v5_pubrec,      ///< stored packet is v5_basic_publish_packet QoS2
    v5_pubcomp,     ///< stored packet is v5_basic_rel_packet
};

template <std::size_t PacketIdBytes>
class basic_store_packet_variant {
public:

    /**
     * @brief constructor
     * @param packet PUBLISH packet QoS1 or 2
     */
    basic_store_packet_variant(v3_1_1::basic_publish_packet<PacketIdBytes> packet)
        :res_{
             [&] {
                 switch (packet.opts().get_qos()) {
                 case qos::at_least_once:
                     return response_packet::v3_1_1_puback;
                 case qos::exactly_once:
                     return response_packet::v3_1_1_pubrec;
                 default:
                     throw system_error{
                         make_error_code(
                             mqtt_error::packet_not_allowed_to_store
                         )
                     };
                 }
             }()
         },
         var_{force_move(packet)}
    {}

    /**
     * @brief constructor
     * @param packet PUBREL packet
     */
    basic_store_packet_variant(v3_1_1::basic_pubrel_packet<PacketIdBytes> packet)
        :res_{response_packet::v3_1_1_pubcomp},
         var_{force_move(packet)}
    {}

    /**
     * @brief constructor
     * @param packet PUBLISH packet QoS1 or 2
     */
    basic_store_packet_variant(v5::basic_publish_packet<PacketIdBytes> packet)
        :res_{
             [&] {
                 switch (packet.opts().get_qos()) {
                 case qos::at_least_once:
                     return response_packet::v5_puback;
                 case qos::exactly_once:
                     return response_packet::v5_pubrec;
                 default:
                     throw system_error{
                         make_error_code(
                             mqtt_error::packet_not_allowed_to_store
                         )
                     };
                 }
             }()
         },
         var_{force_move(packet)}
    {}

    /**
     * @brief constructor
     * @param packet PUBREL packet
     */
    basic_store_packet_variant(v5::basic_pubrel_packet<PacketIdBytes> packet)
        :res_{response_packet::v5_pubcomp},
         var_{force_move(packet)}
    {}

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) const& {
        return
            std::visit(
                std::forward<Func>(func),
                var_
            );
    }

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) & {
        return
            std::visit(
                std::forward<Func>(func),
                var_
            );
    }

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) && {
        return
            std::visit(
                std::forward<Func>(func),
                force_move(var_)
            );
    }

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.const_buffer_sequence();
                },
                [] (error_code const&) {
                    BOOST_ASSERT(false);
                    return std::vector<as::const_buffer>{};
                }
            }
        );
    }

    /**
     * @brief Get packet id
     * @return packet_id
     */
    typename basic_packet_id_type<PacketIdBytes>::type packet_id() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.packet_id();
                }
            }
        );
    }

    /**
     * @brief Get packet size.
     * @return packet size
     */
    std::size_t size() const {
        return visit(
            overload {
                [] (auto const& p) -> std::size_t {
                    return p.size();
                }
            }
        );
    }

    /**
     * @brief Get MessageExpiryInterval property value
     * @return  message_expiry_interval
     */
    std::uint32_t get_message_expiry_interval() const {
        return visit(
            overload {
                [] (v5::basic_publish_packet<PacketIdBytes> const& p) {
                    std::uint32_t ret = 0;
                    bool finish = false;
                    for (auto const& prop : p.props()) {
                        prop.visit(
                            overload {
                                [&](property::message_expiry_interval const& p) {
                                    ret = p.val();
                                    finish = true;
                                },
                                [](auto const&) {
                                }
                            }
                        );
                        if (finish) break;
                    }
                    return ret;
                },
                [] (auto const&) {
                    return std::uint32_t(0);
                }
            }
        );
    }

    /**
     * @brief Update MessageExpiryInterval property
     * @param val message_expiry_interval
     */
    void update_message_expiry_interval(std::uint32_t val) const {
        visit(
            overload {
                [&] (v5::basic_publish_packet<PacketIdBytes>& p) {
                    bool finish = false;
                    for (auto& prop : p.props()) {
                        prop.visit(
                            overload {
                                [&](property::message_expiry_interval& p) {
                                    p = property::message_expiry_interval{val};
                                    finish = true;
                                },
                                [](auto&) {
                                }
                            }
                        );
                        if (finish) break;
                    }
                },
                [] (auto&) {
                }
            }
        );
    }

    /**
     * @brief Get response packet type corresponding to this packet.
     * @return response_packet
     */
    response_packet response_packet_type() const {
        return res_;
    }

    operator basic_packet_variant<PacketIdBytes>() const {
        return visit(
            [](auto const& p) {
                return basic_packet_variant<PacketIdBytes>(p);
            }
        );
    }

private:
    using variant_t = std::variant<
        v3_1_1::basic_publish_packet<PacketIdBytes>,
        v3_1_1::basic_pubrel_packet<PacketIdBytes>,
        v5::basic_publish_packet<PacketIdBytes>,
        v5::basic_pubrel_packet<PacketIdBytes>
    >;

    response_packet res_;
    variant_t var_;
};

template <std::size_t PacketIdBytes>
inline std::ostream& operator<<(std::ostream& o, basic_store_packet_variant<PacketIdBytes> const& v) {
    v.visit(
        overload {
            [&] (auto const& p) {
                o << p;
            }
        }
    );
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_PACKET_STORE_PACKET_VARIANT_HPP
