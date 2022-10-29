// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_EXAMPLE_HEX_DUMP_HPP)
#define ASYNC_MQTT_EXAMPLE_HEX_DUMP_HPP

#include <ostream>
#include <iomanip>

template <typename Iter, typename End>
inline std::ostream& hex_dump(std::ostream& o, Iter it, End end) {
    std::ios::fmtflags f(o.flags());
    o << std::hex;
    for (; it != end; ++it) {
        o << "0x" << std::setw(2) << std::setfill('0') << (static_cast<int>(*it) & 0xff) << ' ';
    }
    o.flags(f);
    return o;
}

#endif // ASYNC_MQTT_EXAMPLE_HEX_DUMP_HPP)
