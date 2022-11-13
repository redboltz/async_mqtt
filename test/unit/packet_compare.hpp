// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_COMPARE_HPP)
#define ASYNC_MQTT_PACKET_COMPARE_HPP

#include <algorithm>

#include <async_mqtt/packet/packet_variant.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <typename T, typename U>
bool packet_compare(T const& t, U const& u) {

    auto t_cbs = t.const_buffer_sequence();
    auto [tb, te] = make_packet_range(t_cbs);

    auto u_cbs = u.const_buffer_sequence();
    auto [ub, ue] = make_packet_range(u_cbs);

    return std::equal(tb, te, ub, ue);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_COMPARE_HPP
