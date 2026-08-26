// Copyright Takatoshi Kondo 2026
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TEST_EXACT_BUFFER_HPP)
#define ASYNC_MQTT_TEST_EXACT_BUFFER_HPP

#include <algorithm>
#include <memory>
#include <string_view>

#include <async_mqtt/util/buffer.hpp>

/**
 * @brief Copy sv into an exactly-sized heap allocation and return a buffer
 *        viewing it.
 *
 * Packet parser tests must not construct the input buffer directly from a
 * string literal. A view of a literal is exactly sv.size() bytes long, but its
 * backing storage is the literal itself, whose terminating NUL lies just past
 * the view's end. A parser that reads one byte past the end of the buffer
 * would then read that NUL (a valid value for most fields) and the bug would
 * be invisible to AddressSanitizer. See GHSA-fwh8-wcr5-q65c.
 *
 * A bare new[] is used rather than std::make_shared<char[]> because the latter
 * co-allocates the control block, whose alignment padding would likewise
 * absorb a one byte overread.
 */
inline async_mqtt::buffer make_exact_buffer(std::string_view sv) {
    std::shared_ptr<char[]> sp{new char[sv.size()]};
    std::copy(sv.begin(), sv.end(), sp.get());
    return async_mqtt::buffer{sp.get(), sv.size(), sp};
}

#endif // ASYNC_MQTT_TEST_EXACT_BUFFER_HPP
