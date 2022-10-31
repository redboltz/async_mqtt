// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_SUBSCRIBE_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_SUBSCRIBE_HPP

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/topic_subopts.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
class basic_subscribe_packet {
public:
    struct entry {
        entry(buffer topic, sub::opts opts)
            : topic_{force_move(topic)},
              topic_length_buf_{endian_static_vector(boost::numeric_cast<std::uint16_t>(topic_.size()))},
              opts_{opts}
        {}

        buffer topic() const {
            return topic_;
        }

        qos qos() const {
            return opts_.qos();
        }

        static_vector<char, 2> topic_length_buf_;
        buffer topic_;
        sub::opts opts_;
    };

    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;
    basic_subscribe_packet(
        std::vector<topic_subopts> params,
        packet_id_t packet_id
    )
        : fixed_header_{make_fixed_header(control_packet_type::subscribe, 0b0010)},
          remaining_length_{PacketIdBytes}
    {

        endian_store(packet_id, packet_id_.data());

        // Check for errors before allocating.
        for (auto const& e : params) {
#if 0 // TBD
            utf8string_check(e.topic);
#endif
        }

        entries_.reserve(params.size());
        for (auto&& e : params) {
            size_t size = e.topic.size();

            entries_.emplace_back(force_move(e.topic), e.opts);
            remaining_length_ +=
                2 +                     // topic name length
                size +                  // topic name
                1;                      // opts
        }
        remaining_length_buf_ = endian_static_vector(remaining_length_);
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

        for (auto const& e : entries_) {
            ret.emplace_back(as::buffer(e.topic_length_buf_.data(), e.topic_length_buf_.size()));
            ret.emplace_back(as::buffer(e.topic_));
            ret.emplace_back(as::buffer(&e.opts_, 1));
        }

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
            entries_.size() * 3;  // topic name length, topic name, qos
    }

    packet_id_t packet_id() const {
        return endian_load<packet_id_t>(packet_id_);
    }

    std::vector<entry> const& entries() const {
        return entries_;
    }

private:
    std::uint8_t fixed_header_;
    std::vector<entry> entries_;
    static_vector<char, PacketIdBytes> packet_id_ = static_vector<char, PacketIdBytes>(PacketIdBytes);
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
};

using subscribe_packet = basic_subscribe_packet<2>;

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_V3_1_1_SUBSCRIBE_HPP
