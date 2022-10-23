// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PUBOPTS_HPP)
#define ASYNC_MQTT_PACKET_PUBOPTS_HPP

#include <async_mqtt/packet/qos.hpp>

namespace async_mqtt {

namespace pub {

constexpr bool is_dup(std::uint8_t v) {
    return (v & 0b00001000) != 0;
}

constexpr qos get_qos(std::uint8_t v) {
    return static_cast<qos>((v & 0b00000110) >> 1);
}

constexpr bool is_retain(std::uint8_t v) {
    return (v & 0b00000001) != 0;
}

constexpr void set_dup(std::uint8_t& fixed_header, bool dup) {
    if (dup) fixed_header |=  0b00001000;
    else     fixed_header &= static_cast<std::uint8_t>(~0b00001000);
}

enum class retain : std::uint8_t {
    yes = 0b00000001,
    no = 0b00000000,
};

enum class dup : std::uint8_t {
    yes = 0b00001000,
    no = 0b00000000,
};

struct opts final {
    constexpr opts() = default;
    ~opts() = default;
    constexpr opts(opts &&) = default;
    constexpr opts(opts const&) = default;
    constexpr opts& operator=(opts &&) = default;
    constexpr opts& operator=(opts const&) = default;

    explicit constexpr opts(std::uint8_t value) : data_(value) { }

    constexpr opts(retain value) : data_(static_cast<std::uint8_t>(value)) { }
    constexpr opts(dup value)    : data_(static_cast<std::uint8_t>(value)) { }
    constexpr opts(qos value)    : data_(static_cast<std::uint8_t>(static_cast<std::uint8_t>(value) << 1))
    {
        BOOST_ASSERT(value == qos::at_most_once || value == qos::at_least_once || value == qos::exactly_once);
    }

    constexpr opts operator|(opts rhs) const { return opts(data_ | rhs.data_); }
    constexpr opts operator|(retain rhs) const          { return *this | opts(rhs); }
    constexpr opts operator|(dup rhs) const             { return *this | opts(rhs); }
    constexpr opts operator|(qos rhs) const             { return *this | opts(rhs); }

    constexpr opts& operator|=(opts rhs) { return (*this = (*this | rhs)); }
    constexpr opts& operator|=(retain rhs)          { return (*this = (*this | rhs)); }
    constexpr opts& operator|=(dup rhs)             { return (*this = (*this | rhs)); }
    constexpr opts& operator|=(qos rhs)             { return (*this = (*this | rhs)); }

    constexpr retain get_retain() const
    { return static_cast<retain>(data_ & 0b00000001); }
    constexpr dup get_dup() const
    { return static_cast<dup>(data_ & 0b00001000); }
    constexpr qos get_qos() const
    { return static_cast<qos>((data_ & 0b00000110) >> 1); }

    explicit constexpr operator std::uint8_t() const { return data_; }

private:
    std::uint8_t data_ = 0; // defaults to retain::no, dup::no, qos::at_most_once
};

constexpr opts operator|(retain lhs, dup rhs) { return opts(lhs) | rhs; }
constexpr opts operator|(retain lhs, qos rhs) { return opts(lhs) | rhs; }

constexpr opts operator|(dup lhs, retain rhs) { return opts(lhs) | rhs; }
constexpr opts operator|(dup lhs, qos rhs)    { return opts(lhs) | rhs; }

constexpr opts operator|(qos lhs, retain rhs) { return opts(lhs) | rhs; }
constexpr opts operator|(qos lhs, dup rhs)    { return opts(lhs) | rhs; }


constexpr char const* retain_to_str(retain v) {
    switch(v) {
        case retain::yes: return "yes";
        case retain::no:  return "no";
        default:          return "invalid_retain";
    }
}

inline
std::ostream& operator<<(std::ostream& os, retain val)
{
    os << retain_to_str(val);
    return os;
}

constexpr char const* dup_to_str(dup v) {
    switch(v) {
        case dup::yes: return "yes";
        case dup::no:  return "no";
        default:       return "invalid_dup";
    }
}


inline
std::ostream& operator<<(std::ostream& os, dup val)
{
    os << dup_to_str(val);
    return os;
}

} // namespace pub

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PUBOPTS_HPP
