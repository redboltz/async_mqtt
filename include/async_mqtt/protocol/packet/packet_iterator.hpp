// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_PACKET_ITERATOR_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_PACKET_ITERATOR_HPP

#include <vector>
#include <tuple>
#include <string>

#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>

#include <async_mqtt/util/buffer.hpp>

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @brief A type alias of Boost.Asio buffers_iterator.
 * @tparam The type of buffer sequence.
 *
 */
template <typename BufferSequence>
using packet_iterator = as::buffers_iterator<BufferSequence>;

/**
 * @brief Create a pair of the const buffer sequence that points begin and end.
 * @param cbs The sequence of const buffer.
 * @return The pair of the iterator.
 * @tparam The type of const buffer sequence.
 *
 */
template <typename  ConstBufferSequence>
std::pair<packet_iterator<ConstBufferSequence>, packet_iterator<ConstBufferSequence>>
make_packet_range(ConstBufferSequence const& cbs) {
    return {
        packet_iterator<ConstBufferSequence>::begin(cbs),
        packet_iterator<ConstBufferSequence>::end(cbs)
    };
}

/**
 * @brief Convert const buffer sequence to the string.
 * @param cbs The sequence of const buffer.
 * @return The string that all buffers are concatenated.
 * @tparam The type of const buffer sequence.
 *
 */
template <typename ConstBufferSequence>
std::string
to_string(ConstBufferSequence const& cbs) {
    auto [b, e] = make_packet_range(cbs);
    return std::string(b, e);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_PACKET_PACKET_ITERATOR_HPP
