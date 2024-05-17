// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_ITERATOR_HPP)
#define ASYNC_MQTT_PACKET_PACKET_ITERATOR_HPP

#include <vector>
#include <tuple>
#include <string>

#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>

#include <async_mqtt/util/buffer.hpp>

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @ingroup packet
 * @brief iterator type of buffer sequence
 */
template <template <typename...> typename Container, typename Buffer>
using packet_iterator = as::buffers_iterator<Container<Buffer>>;

/**
 * @ingroup packet
 * @brief create packet iterator range
 * @param cbs collection of th buffer
 * @return the pair of packet_iterator
 */
template <template <typename...> typename Container, typename Buffer>
std::pair<packet_iterator<Container, Buffer>, packet_iterator<Container, Buffer>>
make_packet_range(Container<Buffer> const& cbs) {
    return {
        packet_iterator<Container, Buffer>::begin(cbs),
        packet_iterator<Container, Buffer>::end(cbs)
    };
}

/**
 * @ingroup packet
 * @brief convert buffer sequence to the string
 * @param cbs collection of th buffer
 * @return string that all buffers are concatenated
 */
template <template <typename...> typename Container, typename Buffer>
std::string
to_string(Container<Buffer> const& cbs) {
    auto [b, e] = make_packet_range(cbs);
    return std::string(b, e);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_ITERATOR_HPP
