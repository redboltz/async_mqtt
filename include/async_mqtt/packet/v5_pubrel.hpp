// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PUBREL_HPP)
#define ASYNC_MQTT_PACKET_V5_PUBREL_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/scope_guard.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @brief MQTT PUBREL packet (v5)
 * @tparam PacketIdBytes size of packet_id
 *
 * If basic_endpoint::set_auto_pub_response() is called with true, then this packet is
 * automatically sent when PUBREC v5::basic_pubrec_packet is received.
 * If both the client and the broker keeping the session, this packet is
 * stored in the endpoint for resending if disconnect/reconnect happens.
 * If the session doesn' exist or lost, then the stored packets are erased.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901141
 */
template <std::size_t PacketIdBytes>
class basic_pubrel_packet {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBREC packet
     * @param reason_code PubrelReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901144
     * @param props       properties.
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901145
     */
    basic_pubrel_packet(
        packet_id_t packet_id,
        pubrel_reason_code reason_code,
        properties props
    ) : basic_pubrel_packet{
            packet_id,
            optional<pubrel_reason_code>(reason_code),
            force_move(props)
        }
    {}

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBREC packet
     */
    basic_pubrel_packet(
        packet_id_t packet_id
    ) : basic_pubrel_packet{
            packet_id,
            nullopt,
            properties{}
        }
    {}

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the PUBREC packet
     * @param reason_code PubrelReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901144
     */
    basic_pubrel_packet(
        packet_id_t packet_id,
        pubrel_reason_code reason_code
    ) : basic_pubrel_packet{
            packet_id,
            optional<pubrel_reason_code>(reason_code),
            properties{}
        }
    {}

    basic_pubrel_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::pubrel_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);

        // remaining_length
        if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
            remaining_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v5::pubrel_packet remaining length is invalid");
        }

        // packet_id
        if (!insert_advance(buf, packet_id_)) {
            throw make_error(
                errc::bad_message,
                "v5::pubrel_packet packet_id doesn't exist"
            );
        }

        if (remaining_length_ == 2) {
            if (!buf.empty()) {
                throw make_error(errc::bad_message, "v5::pubrel_packet remaining length is invalid");
            }
            return;
        }

        // connect_reason_code
        reason_code_.emplace(static_cast<pubrel_reason_code>(buf.front()));
        buf.remove_prefix(1);
        switch (*reason_code_) {
        case pubrel_reason_code::success:
        case pubrel_reason_code::packet_identifier_not_found:
            break;
        default:
            throw make_error(
                errc::bad_message,
                "v5::pubrel_packet connect reason_code is invalid"
            );
            break;
        }

        if (remaining_length_ == 3) {
            if (!buf.empty()) {
                throw make_error(errc::bad_message, "v5::pubrel_packet remaining length is invalid");
            }
            return;
        }

        // property
        auto it = buf.begin();
        if (auto pl_opt = variable_bytes_to_val(it, buf.end())) {
            property_length_ = *pl_opt;
            std::copy(buf.begin(), it, std::back_inserter(property_length_buf_));
            buf.remove_prefix(std::size_t(std::distance(buf.begin(), it)));
            if (buf.size() < property_length_) {
                throw make_error(
                    errc::bad_message,
                    "v5::pubrel_packet properties_don't match its length"
                );
            }
            auto prop_buf = buf.substr(0, property_length_);
            props_ = make_properties(prop_buf, property_location::pubrel);
            buf.remove_prefix(property_length_);
        }
        else {
            throw make_error(
                errc::bad_message,
                "v5::pubrel_packet property_length is invalid"
            );
        }

        if (!buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::pubrel_packet properties don't match its length"
            );
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::pubrel;
    }

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        std::vector<as::const_buffer> ret;
        ret.reserve(num_of_const_buffer_sequence());
        ret.emplace_back(as::buffer(&fixed_header_, 1));
        ret.emplace_back(as::buffer(remaining_length_buf_.data(), remaining_length_buf_.size()));

        ret.emplace_back(as::buffer(packet_id_.data(), packet_id_.size()));

        if (reason_code_) {
            ret.emplace_back(as::buffer(&*reason_code_, 1));

            if (property_length_buf_.size() != 0) {
                ret.emplace_back(as::buffer(property_length_buf_.data(), property_length_buf_.size()));
                auto props_cbs = async_mqtt::const_buffer_sequence(props_);
                std::move(props_cbs.begin(), props_cbs.end(), std::back_inserter(ret));
            }
        }

        return ret;
    }

    /**
     * @brief Get packet size.
     * @return packet size
     */
    std::size_t size() const {
        return
            1 +                            // fixed header
            remaining_length_buf_.size() +
            remaining_length_;
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    constexpr std::size_t num_of_const_buffer_sequence() const {
        return
            1 +                   // fixed header
            1 +                   // remaining length
            1 +                   // packet_id
            1 +                   // reason_code
            [&] () -> std::size_t {
                if (property_length_buf_.size() == 0) return 0;
                return
                    1 +                   // property length
                    async_mqtt::num_of_const_buffer_sequence(props_);
            }();
    }

    /**
     * @brief Get packet_id.
     * @return packet_id
     */
    packet_id_t packet_id() const {
        return endian_load<packet_id_t>(packet_id_.data());
    }

    /**
     * @breif Get reason code
     * @return reason_code
     */
    pubrel_reason_code code() const {
        if (reason_code_) return *reason_code_;
        return pubrel_reason_code::success;
    }

    /**
     * @breif Get properties
     * @return properties
     */
    properties const& props() const {
        return props_;
    }

    friend
    inline std::ostream& operator<<(std::ostream& o, basic_pubrel_packet<PacketIdBytes> const& v) {
        o <<
            "v5::pubrel{" <<
            "pid:" << v.packet_id();
        if (v.reason_code_) {
            o << ",rc:" << *v.reason_code_;
        }
        if (!v.props().empty()) {
            o << ",ps:" << v.props();
        };
        o << "}";
        return o;
    }

private:
    basic_pubrel_packet(
        packet_id_t packet_id,
        optional<pubrel_reason_code> reason_code,
        properties props
    )
        : fixed_header_{
              make_fixed_header(control_packet_type::pubrel, 0b0010)
          },
          remaining_length_{
              PacketIdBytes
          },
          packet_id_(packet_id_.capacity()),
          reason_code_{reason_code},
          property_length_(async_mqtt::size(props)),
          props_(force_move(props))
    {
        using namespace std::literals;
        endian_store(packet_id, packet_id_.data());

        auto guard = unique_scope_guard(
            [&] {
                auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
                for (auto e : rb) {
                    remaining_length_buf_.push_back(e);
                }
            }
        );

        if (!reason_code_) return;
        remaining_length_ += 1;

        if (property_length_ == 0) return;

        auto pb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(property_length_));
        for (auto e : pb) {
            property_length_buf_.push_back(e);
        }

        for (auto const& prop : props_) {
            auto id = prop.id();
            if (!validate_property(property_location::pubrel, id)) {
                throw make_error(
                    errc::bad_message,
                    "v5::pubrel_packet property "s + id_to_str(id) + " is not allowed"
                );
            }
        }

        remaining_length_ += property_length_buf_.size() + property_length_;
    }

private:
    std::uint8_t fixed_header_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
    static_vector<char, PacketIdBytes> packet_id_;

    optional<pubrel_reason_code> reason_code_;

    std::size_t property_length_ = 0;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

using pubrel_packet = basic_pubrel_packet<2>;

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_V5_PUBREL_HPP
