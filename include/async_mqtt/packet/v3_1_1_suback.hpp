// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/topic_subopts.hpp>
#include <async_mqtt/packet/reason_code.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
class basic_suback_packet {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;
    basic_suback_packet(
        std::vector<suback_return_code> params,
        packet_id_t packet_id
    )
        : fixed_header_{make_fixed_header(control_packet_type::suback, 0b0000)},
          entries_{force_move(params)},
          remaining_length_{PacketIdBytes + entries_.size()}
    {
        endian_store(packet_id, packet_id_.data());

        remaining_length_buf_ = endian_static_vector(remaining_length_);
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
            auto rc = static_cast<suback_return_code>(buf.back());
            entries_.emplace_back(rc);
            buf.remove_prefix(1);
        }
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
     * @brief Get whole size of sequence
     * @return whole size
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

    packet_id_t packet_id() const {
        return endian_load<packet_id_t>(packet_id_);
    }

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

using suback_packet = basic_suback_packet<2>;

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_V3_1_1_SUBACK_HPP
