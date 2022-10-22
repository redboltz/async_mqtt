// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_HPP)
#define ASYNC_MQTT_PACKET_HPP

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/packet/fixed_header.hpp>

namespace async_mqtt {

namespace detail {

class header_only_packet {
public:
    /**
     * @brief Create empty header_packet_id_packet.
     */
    header_only_packet(control_packet_type type, std::uint8_t flags)
        : packet_ { static_cast<char>(make_fixed_header(type, flags)), 0 }
    {}

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        return { as::buffer(packet_.data(), packet_.size()) };
    }

    /**
     * @brief Get whole size of sequence
     * @return whole size
     */
    std::size_t size() const {
        return packet_.size();
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return 1;
    }

    /**
     * @brief Create one continuours buffer.
     *        All sequence of buffers are concatinated.
     *        It is useful to store to file/database.
     * @return continuous buffer
     */
    std::string continuous_buffer() const {
        return std::string(packet_.data(), size());
    }
private:
    static_vector<char, 2> packet_;
};

} // namespace detail

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_HPP
