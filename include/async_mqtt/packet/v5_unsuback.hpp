// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_UNSUBACK_HPP)
#define ASYNC_MQTT_PACKET_V5_UNSUBACK_HPP

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @brief MQTT UNSUBACK packet (v5)
 * @tparam PacketIdBytes size of packet_id
 *
 * MQTT UNSUBACK packet.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901187
 */
template <std::size_t PacketIdBytes>
class basic_unsuback_packet {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the UNSUBSCRIBE packet
     * @param params    unsuback entries.
     * @param props     properties.
     *                  \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901187
     */
    basic_unsuback_packet(
        packet_id_t packet_id,
        std::vector<unsuback_reason_code> params,
        properties props = {}
    )
        : fixed_header_{make_fixed_header(control_packet_type::unsuback, 0b0000)},
          entries_{force_move(params)},
          remaining_length_{PacketIdBytes + entries_.size()},
          property_length_(async_mqtt::size(props)),
          props_(force_move(props))
    {
        using namespace std::literals;
        endian_store(packet_id, packet_id_.data());

        auto pb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(property_length_));
        for (auto e : pb) {
            property_length_buf_.push_back(e);
        }

        for (auto const& prop : props_) {
            auto id = prop.id();
            if (!validate_property(property_location::unsuback, id)) {
                throw make_error(
                    errc::bad_message,
                    "v5::unsuback_packet property "s + id_to_str(id) + " is not allowed"
                );
            }
        }

        remaining_length_ += property_length_buf_.size() + property_length_;
        remaining_length_buf_ = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
    }

    basic_unsuback_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::unsuback_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);
        auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(fixed_header_));
        if (!cpt_opt || *cpt_opt != control_packet_type::unsuback) {
            throw make_error(
                errc::bad_message,
                "v5::unsuback_packet fixed_header is invalid"
            );
        }

        // remaining_length
        if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
            remaining_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v5::unsuback_packet remaining length is invalid");
        }
        if (remaining_length_ != buf.size()) {
            throw make_error(errc::bad_message, "v5::unsuback_packet remaining length doesn't match buf.size()");
        }

        // packet_id
        if (!copy_advance(buf, packet_id_)) {
            throw make_error(
                errc::bad_message,
                "v5::unsuback_packet packet_id doesn't exist"
            );
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
                    "v5::unsuback_packet properties_don't match its length"
                );
            }
            auto prop_buf = buf.substr(0, property_length_);
            props_ = make_properties(prop_buf, property_location::unsuback);
            buf.remove_prefix(property_length_);
        }
        else {
            throw make_error(
                errc::bad_message,
                "v5::unsuback_packet property_length is invalid"
            );
        }

        if (remaining_length_ == 0) {
            throw make_error(errc::bad_message, "v5::unsuback_packet doesn't have entries");
        }

        while (!buf.empty()) {
            // unsuback_reason_code
            if (buf.empty()) {
                throw make_error(
                    errc::bad_message,
                    "v5::unsuback_packet unsuback_reason_code  doesn't exist"
                );
            }
            auto rc = static_cast<unsuback_reason_code>(buf.front());
            entries_.emplace_back(rc);
            buf.remove_prefix(1);
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::unsuback;
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

        ret.emplace_back(as::buffer(property_length_buf_.data(), property_length_buf_.size()));
        auto props_cbs = async_mqtt::const_buffer_sequence(props_);
        std::move(props_cbs.begin(), props_cbs.end(), std::back_inserter(ret));

        ret.emplace_back(as::buffer(entries_.data(), entries_.size()));

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
    std::size_t num_of_const_buffer_sequence() const {
        return
            1 +                   // fixed header
            1 +                   // remaining length
            1 +                   // packet id
            [&] () -> std::size_t {
                if (property_length_buf_.size() == 0) return 0;
                return
                    1 +                   // property length
                    async_mqtt::num_of_const_buffer_sequence(props_);
            }() +
            entries_.size();      // unsuback_reason_code
    }

    /**
     * @brief Get packet_id.
     * @return packet_id
     */
    packet_id_t packet_id() const {
        return endian_load<packet_id_t>(packet_id_.data());
    }

    /**
     * @brief Get entries
     * @return entries
     */
    std::vector<unsuback_reason_code> const& entries() const {
        return entries_;
    }

    /**
     * @breif Get properties
     * @return properties
     */
    properties const& props() const {
        return props_;
    }

private:
    std::uint8_t fixed_header_;
    std::vector<unsuback_reason_code> entries_;
    static_vector<char, PacketIdBytes> packet_id_ = static_vector<char, PacketIdBytes>(PacketIdBytes);
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;

    std::size_t property_length_ = 0;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

template <std::size_t PacketIdBytes>
inline std::ostream& operator<<(std::ostream& o, basic_unsuback_packet<PacketIdBytes> const& v) {
    o <<
        "v5::unsuback{" <<
        "pid:" << v.packet_id() << ",[";
    auto b = v.entries().cbegin();
    auto e = v.entries().cend();
    if (b != e) {
        o << *b++;
    }
    for (; b != e; ++b) {
        o << "," << *b;
    }
    o << "]";
    if (!v.props().empty()) {
        o << ",ps:" << v.props();
    };
    o << "}";
    return o;
}

using unsuback_packet = basic_unsuback_packet<2>;

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_V5_UNSUBACK_HPP
