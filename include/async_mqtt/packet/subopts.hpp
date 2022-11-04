// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_SUBOPTS_HPP)
#define ASYNC_MQTT_PACKET_SUBOPTS_HPP

#include <cstdint>
#include <ostream>

#include <async_mqtt/packet/qos.hpp>

namespace async_mqtt {

namespace sub {

enum class retain_handling : std::uint8_t
{
    send = 0b00000000,
    send_only_new_subscription = 0b00010000,
    not_send = 0b00100000,
};
enum class rap : std::uint8_t
{
    dont = 0b00000000,
    retain = 0b00001000,
};
enum class nl : std::uint8_t
{
    no = 0b00000000,
    yes = 0b00000100,
};

struct opts final {
    constexpr opts() = delete;
    ~opts() = default;
    constexpr opts(opts &&) = default;
    constexpr opts(opts const&) = default;
    constexpr opts& operator=(opts &&) = default;
    constexpr opts& operator=(opts const&) = default;

    explicit constexpr opts(std::uint8_t value) : data_(value) { }

    constexpr opts(retain_handling value) : data_(static_cast<std::uint8_t>(value)) { }
    constexpr opts(rap value)             : data_(static_cast<std::uint8_t>(value)) { }
    constexpr opts(nl value)              : data_(static_cast<std::uint8_t>(value)) { }
    constexpr opts(qos value)             : data_(static_cast<std::uint8_t>(value)) { }

    constexpr opts operator|(opts rhs) const { return opts(data_ | rhs.data_); }
    constexpr opts operator|(retain_handling rhs) const   { return *this | opts(rhs); }
    constexpr opts operator|(rap rhs) const               { return *this | opts(rhs); }
    constexpr opts operator|(nl rhs) const                { return *this | opts(rhs); }
    constexpr opts operator|(qos rhs) const               { return *this | opts(rhs); }

    constexpr opts& operator|=(opts rhs) { return (*this = (*this | rhs)); }
    constexpr opts& operator|=(retain_handling rhs)   { return (*this = (*this | rhs)); }
    constexpr opts& operator|=(rap rhs)               { return (*this = (*this | rhs)); }
    constexpr opts& operator|=(nl rhs)                { return (*this = (*this | rhs)); }
    constexpr opts& operator|=(qos rhs)               { return (*this = (*this | rhs)); }

    constexpr retain_handling retain_handling() const
    { return static_cast<enum retain_handling>(data_ & 0b00110000); }
    constexpr rap rap() const
    { return static_cast<enum rap>(data_ & 0b00001000); }
    constexpr nl nl() const
    { return static_cast<enum nl>(data_ & 0b00000100); }
    constexpr qos qos() const
    { return static_cast<enum qos>(data_ & 0b00000011); }

    explicit constexpr operator std::uint8_t() const { return data_; }

    constexpr bool operator==(opts rhs) const { return data_ == rhs.data_; }
    constexpr bool operator<(opts rhs) const { return data_ < rhs.data_; }

private:
    std::uint8_t data_;
};

constexpr opts operator|(retain_handling lhs, rap rhs) { return opts(lhs) | rhs; }
constexpr opts operator|(retain_handling lhs, nl rhs)  { return opts(lhs) | rhs; }
constexpr opts operator|(retain_handling lhs, qos rhs) { return opts(lhs) | rhs; }

constexpr opts operator|(rap lhs, retain_handling rhs) { return opts(lhs) | rhs; }
constexpr opts operator|(rap lhs, nl rhs)              { return opts(lhs) | rhs; }
constexpr opts operator|(rap lhs, qos rhs)             { return opts(lhs) | rhs; }

constexpr opts operator|(nl lhs, retain_handling rhs)  { return opts(lhs) | rhs; }
constexpr opts operator|(nl lhs, rap rhs)              { return opts(lhs) | rhs; }
constexpr opts operator|(nl lhs, qos rhs)              { return opts(lhs) | rhs; }

constexpr opts operator|(qos lhs, retain_handling rhs) { return opts(lhs) | rhs; }
constexpr opts operator|(qos lhs, rap rhs)             { return opts(lhs) | rhs; }
constexpr opts operator|(qos lhs, nl rhs)              { return opts(lhs) | rhs; }

constexpr char const* retain_handling_to_str(retain_handling v) {
    switch(v) {
    case retain_handling::send:                       return "send";
    case retain_handling::send_only_new_subscription: return "send_only_new_subscription";
    case retain_handling::not_send:                   return "not_send";
    default:                                          return "invalid_retain_handling";
    }
}

inline
std::ostream& operator<<(std::ostream& os, retain_handling val)
{
    os << retain_handling_to_str(val);
    return os;
}

constexpr char const* rap_to_str(rap v) {
    switch(v) {
    case rap::dont:   return "dont";
    case rap::retain: return "retain";
    default:          return "invalid_rap";
    }
}

inline
std::ostream& operator<<(std::ostream& os, rap val)
{
    os << rap_to_str(val);
    return os;
}

constexpr char const* nl_to_str(nl v) {
    switch(v) {
    case nl::no:   return "no";
    case nl::yes:  return "yes";
    default:       return "invalid_nl";
    }
}

inline
std::ostream& operator<<(std::ostream& os, nl val)
{
    os << nl_to_str(val);
    return os;
}

} // namespace sub

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_SUBOPTS_HPP
