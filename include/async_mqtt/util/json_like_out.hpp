// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_JSON_LIKE_OUT_HPP)
#define ASYNC_MQTT_UTIL_JSON_LIKE_OUT_HPP

#include <ostream>
#include <iomanip>

#include <async_mqtt/util/buffer.hpp>

namespace async_mqtt {

struct json_like_out_t {
    template <
        typename StringViewLike,
        std::enable_if_t<
            std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view>,
            std::nullptr_t
        > = nullptr
    >
    explicit json_like_out_t(StringViewLike const& sv): sv {sv} {}

    friend std::ostream& operator<<(std::ostream& o, json_like_out_t const& v) {
        for (char c : v.sv) {
            switch (c) {
            case '\\':
                o << "\\\\";
                break;
            case '"':
                o << "\\\"";
                break;
            case '/':
                o << "\\/";
                break;
            case '\b':
                o << "\\b";
                break;
            case '\f':
                o << "\\f";
                break;
            case '\n':
                o << "\\n";
                break;
            case '\r':
                o << "\\r";
                break;
            case '\t':
                o << "\\t";
                break;
            default: {
                unsigned int code = static_cast<unsigned int>(c);
                if (code < 0x20 || code >= 0x7f) {
                    std::ios::fmtflags flags(o.flags());
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (code & 0xff);
                    o.flags(flags);
                }
                else {
                    o << c;
                }
            } break;
            }
        }
        return o;
    }

private:
    std::string_view sv;
};


template <
    typename StringViewLike,
    std::enable_if_t<
        std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view>,
        std::nullptr_t
    > = nullptr
>
inline json_like_out_t json_like_out(StringViewLike const& sv) {
    return json_like_out_t{sv};
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_JSON_LIKE_OUT_HPP
