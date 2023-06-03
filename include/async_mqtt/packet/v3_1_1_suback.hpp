// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/topic_subopts.hpp>
#include <async_mqtt/packet/suback_return_code.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @brief MQTT SUBACK packet (v3.1.1)
 * @tparam PacketIdBytes size of packet_id
 *
 * MQTT SUBACK packet.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718068
 */
template <std::size_t PacketIdBytes>
class basic_suback_packet {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    /**
     * @brief constructor
     * @param packet_id MQTT PacketIdentifier that is corresponding to the SUBSCRIBE packet
     * @param params    suback entries.
     */
    basic_suback_packet(
        packet_id_t packet_id,
        std::vector<suback_return_code> params
    )
        : fixed_header_{make_fixed_header(control_packet_type::suback, 0b0000)},
          entries_{force_move(params)},
          remaining_length_{PacketIdBytes + entries_.size()}
    {
        endian_store(packet_id, packet_id_.data());

        remaining_length_buf_ = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
    }

    basic_suback_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::suback_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);
        auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(fixed_header_));
        if (!cpt_opt || *cpt_opt != control_packet_type::suback) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::suback_packet fixed_header is invalid"
            );
        }

        // remaining_length
        if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
            remaining_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v3_1_1::suback_packet remaining length is invalid");
        }
        if (remaining_length_ != buf.size()) {
            throw make_error(errc::bad_message, "v3_1_1::suback_packet remaining length doesn't match buf.size()");
        }

        // packet_id
        if (!copy_advance(buf, packet_id_)) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::suback_packet packet_id doesn't exist"
            );
        }

        if (remaining_length_ == 0) {
            throw make_error(errc::bad_message, "v3_1_1::suback_packet doesn't have entries");
        }

        while (!buf.empty()) {
            // suback_return_code
            if (buf.empty()) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::suback_packet suback_return_code  doesn't exist"
                );
            }
            auto rc = static_cast<suback_return_code>(buf.front());
            entries_.emplace_back(rc);
            buf.remove_prefix(1);
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::suback;
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
            entries_.size();      // suback_return_code
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
    std::vector<suback_return_code> const& entries() const {
        return entries_;
    }

private:
    std::uint8_t fixed_header_;
    std::vector<suback_return_code> entries_;
    static_vector<char, PacketIdBytes> packet_id_ = static_vector<char, PacketIdBytes>(PacketIdBytes);
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
};

template <std::size_t PacketIdBytes>
inline std::ostream& operator<<(std::ostream& o, basic_suback_packet<PacketIdBytes> const& v) {
    o <<
        "v3_1_1::suback{" <<
        "pid:" << v.packet_id() << ",[";
    auto b = v.entries().cbegin();
    auto e = v.entries().cend();
    if (b != e) {
        o << *b++;
    }
    for (; b != e; ++b) {
        o << "," << *b;
    }
    o << "]}";
    return o;
}

/**
 * @related basic_suback_packet
 * @brief Type alias of basic_suback_packet (PacketIdBytes=2).
 */
using suback_packet = basic_suback_packet<2>;

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP
