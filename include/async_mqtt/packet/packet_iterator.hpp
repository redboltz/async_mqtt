// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_ITERATOR_HPP)
#define ASYNC_MQTT_PACKET_PACKET_ITERATOR_HPP

#include <vector>
#include <tuple>

#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <template <typename> typename Container>
using packet_iterator = as::buffers_iterator<Container<as::const_buffer>>;

template <template <typename> typename Container>
std::pair<packet_iterator<Container>, packet_iterator<Container>>
make_packet_range(Container<as::const_buffer> const& cbs) {
    return {
        packet_iterator<Container>::begin(cbs),
        packet_iterator<Container>::end(cbs)
    };
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_ITERATOR_HPP
