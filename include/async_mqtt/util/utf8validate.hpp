// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP)
#define ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP

#include <string_view>

namespace async_mqtt {

inline bool utf8string_check(std::string_view str) {
    // This code is based on https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
    auto result = true;
    auto it = str.begin();
    auto end = str.end();

    while (it != end) {
        if (static_cast<unsigned char>(*(it + 0)) < 0b1000'0000) {
            // 0xxxxxxxxx
            if (static_cast<unsigned char>(*(it + 0)) == 0x00) {
                result = false;
                break;
            }
            if ((static_cast<unsigned char>(*(it + 0)) >= 0x01 &&
                 static_cast<unsigned char>(*(it + 0)) <= 0x1f) ||
                static_cast<unsigned char>(*(it + 0)) == 0x7f) {
                result = false; // well_formed but contains non charactor
            }
            ++it;
        }
        else if ((static_cast<unsigned char>(*(it + 0)) & 0b1110'0000) == 0b1100'0000) {
            // 110XXXXx 10xxxxxx
            if (it + 1 >= end) {
                result = false;
                break;
            }
            if ((static_cast<unsigned char>(*(it + 1)) & 0b1100'0000) != 0b1000'0000 ||
                (static_cast<unsigned char>(*(it + 0)) & 0b1111'1110) == 0b1100'0000) { // overlong
                result = false;
                break;
            }
            if (static_cast<unsigned char>(*(it + 0)) == 0b1100'0010 &&
                static_cast<unsigned char>(*(it + 1)) >= 0b1000'0000 &&
                static_cast<unsigned char>(*(it + 1)) <= 0b1001'1111) {
                result = false; // well_formed but contains non charactor
            }
            it += 2;
        }
        else if ((static_cast<unsigned char>(*(it + 0)) & 0b1111'0000) == 0b1110'0000) {
            // 1110XXXX 10Xxxxxx 10xxxxxx
            if (it + 2 >= end) {
                result = false;
                break;
            }
            if ((static_cast<unsigned char>(*(it + 1)) & 0b1100'0000) != 0b1000'0000 ||
                (static_cast<unsigned char>(*(it + 2)) & 0b1100'0000) != 0b1000'0000 ||
                (static_cast<unsigned char>(*(it + 0)) == 0b1110'0000 &&
                 (static_cast<unsigned char>(*(it + 1)) & 0b1110'0000) == 0b1000'0000) || // overlong?
                (static_cast<unsigned char>(*(it + 0)) == 0b1110'1101 &&
                 (static_cast<unsigned char>(*(it + 1)) & 0b1110'0000) == 0b1010'0000)) { // surrogate?
                result = false;
                break;
            }
            if (static_cast<unsigned char>(*(it + 0)) == 0b1110'1111 &&
                static_cast<unsigned char>(*(it + 1)) == 0b1011'1111 &&
                (static_cast<unsigned char>(*(it + 2)) & 0b1111'1110) == 0b1011'1110) {
                // U+FFFE or U+FFFF?
                result = false; // well_formed but contains non charactor
            }
            it += 3;
        }
        else if ((static_cast<unsigned char>(*(it + 0)) & 0b1111'1000) == 0b1111'0000) {
            // 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx
            if (it + 3 >= end) {
                result = false;
                break;
            }
            if ((static_cast<unsigned char>(*(it + 1)) & 0b1100'0000) != 0b1000'0000 ||
                (static_cast<unsigned char>(*(it + 2)) & 0b1100'0000) != 0b1000'0000 ||
                (static_cast<unsigned char>(*(it + 3)) & 0b1100'0000) != 0b1000'0000 ||
                (static_cast<unsigned char>(*(it + 0)) == 0b1111'0000 &&
                 (static_cast<unsigned char>(*(it + 1)) & 0b1111'0000) == 0b1000'0000) ||    // overlong?
                (static_cast<unsigned char>(*(it + 0)) == 0b1111'0100 &&
                 static_cast<unsigned char>(*(it + 1)) > 0b1000'1111) ||
                static_cast<unsigned char>(*(it + 0)) > 0b1111'0100) { // > U+10FFFF?
                result = false;
                break;
            }
            if ((static_cast<unsigned char>(*(it + 1)) & 0b1100'1111) == 0b1000'1111 &&
                static_cast<unsigned char>(*(it + 2)) == 0b1011'1111 &&
                (static_cast<unsigned char>(*(it + 3)) & 0b1111'1110) == 0b1011'1110) {
                // U+nFFFE or U+nFFFF?
                result = false; // well_formed but contains non charactor
            }
            it += 4;
        }
        else {
            result = false;
            break;
        }
    }
    return result;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP
