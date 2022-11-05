// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_v5_PUBREL_HPP)
#define ASYNC_MQTT_PACKET_v5_PUBREL_HPP

#include <utility>
#include <numeric>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/property_variant.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
class basic_pubrel_packet {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;
    basic_pubrel_packet(
        packet_id_t packet_id,
        properties props
    )
        : fixed_header_{
              static_cast<char>(make_fixed_header(control_packet_type::pubrel, 0b0010))
          },
          remaining_length_{
              PacketIdBytes
          },
          packet_id_(packet_id_.capacity()),
          property_length_(async_mqtt::size(props)),
          props_(force_move(props))
    {
        using namespace std::literals;
        endian_store(packet_id, packet_id_.data());

        auto pb = val_to_variable_bytes(property_length_);
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

        auto rb = val_to_variable_bytes(remaining_length_);
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }
    }

    basic_pubrel_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::pubrel_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());

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

        // property_length
        if (auto vl_opt = insert_advance_variable_length(buf, property_length_buf_)) {
            property_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v5::pubrel_packet property length is invalid");
        }

        // property
        if (buf.size() != property_length_) {
            throw make_error(errc::bad_message, "v5::pubrel_packet props don't match their length");
        }
        props_ = make_properties(buf, property_location::pubrel);
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
    constexpr std::size_t num_of_const_buffer_sequence() const {
        return
            1 +                   // fixed header
            1 +                   // remaining length
            1 +                   // packet_id
            1 +                   // property length
            async_mqtt::num_of_const_buffer_sequence(props_);
    }

    packet_id_t packet_id() const {
        return endian_load<packet_id_t>(packet_id_.data());
    }

private:
    std::uint8_t fixed_header_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
    static_vector<char, PacketIdBytes> packet_id_;
    std::uint32_t property_length_;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

using pubrel_packet = basic_pubrel_packet<2>;

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_v5_PUBREL_HPP
