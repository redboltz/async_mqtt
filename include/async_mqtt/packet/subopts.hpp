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

/// @file

namespace async_mqtt {

namespace sub {

/**
 * @brief MQTT RetainHandling
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169
 */
enum class retain_handling : std::uint8_t
{
    send = 0b00000000,                       ///< Always send. Same as MQTT v.3.1.1
    send_only_new_subscription = 0b00010000, ///< Only new subscription. Not overwrite.
    not_send = 0b00100000,                   ///< Always not send.
};

/**
 * @brief MQTT RetainAsPublished
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169
 */
enum class rap : std::uint8_t
{
    dont = 0b00000000,   ///< Retain is set to 0 delivery on publish by the broker. Same as MQTT v.3.1.1
    retain = 0b00001000, ///< Preserve published Retain delivery on publish by the broker.
};

/**
 * @brief MQTT NoLocal
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169
 */
enum class nl : std::uint8_t
{
    no = 0b00000000,  ///< Subscriber's publish would be delivered to the subscriber itself. Same as MQTT v.3.1.1
    yes = 0b00000100, ///< Subscriber's publish would not be delivered to the subscriber itself.
};

/**
 * @brief MQTT SubscribeOptions
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169
 */
struct opts final {
    constexpr opts() = delete;
    ~opts() = default;
    constexpr opts(opts &&) = default;
    constexpr opts(opts const&) = default;
    constexpr opts& operator=(opts &&) = default;
    constexpr opts& operator=(opts const&) = default;

    /**
     * @brief constructor
     * @param value Byte image of the options
     */
    explicit constexpr opts(std::uint8_t value) : data_(value) { }

    /**
     * @brief constructor
     * @param value retain_handling
     */
    constexpr opts(retain_handling value) : data_(static_cast<std::uint8_t>(value)) { }

    /**
     * @brief constructor
     * @param value rap
     */
    constexpr opts(rap value)             : data_(static_cast<std::uint8_t>(value)) { }

    /**
     * @brief constructor
     * @param value nl
     */
    constexpr opts(nl value)              : data_(static_cast<std::uint8_t>(value)) { }

    /**
     * @brief constructor
     * @param value qos
     */
    constexpr opts(qos value)             : data_(static_cast<std::uint8_t>(value)) { }

    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(opts rhs) const { return opts(data_ | rhs.data_); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(retain_handling rhs) const   { return *this | opts(rhs); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(rap rhs) const               { return *this | opts(rhs); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(nl rhs) const                { return *this | opts(rhs); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(qos rhs) const               { return *this | opts(rhs); }

    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(opts rhs) { return (*this = (*this | rhs)); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(retain_handling rhs)   { return (*this = (*this | rhs)); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(rap rhs)               { return (*this = (*this | rhs)); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(nl rhs)                { return (*this = (*this | rhs)); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(qos rhs)               { return (*this = (*this | rhs)); }

    /**
     * @brief Get retain_handling
     *
     * @return retain_handling
     */
    constexpr retain_handling get_retain_handling() const
    { return static_cast<enum retain_handling>(data_ & 0b00110000); }

    /**
     * @brief Get rap
     *
     * @return rap
     */
    constexpr rap get_rap() const
    { return static_cast<enum rap>(data_ & 0b00001000); }

    /**
     * @brief Get nl
     *
     * @return no
     */
    constexpr nl get_nl() const
    { return static_cast<enum nl>(data_ & 0b00000100); }

    /**
     * @brief Get qos
     *
     * @return qos
     */
    constexpr qos get_qos() const
    { return static_cast<enum qos>(data_ & 0b00000011); }

    /**
     * @brief Get byte image
     *
     * @return byte image
     */
    explicit constexpr operator std::uint8_t() const { return data_; }

    /**
     * @brief equal operator
     */
    constexpr bool operator==(opts rhs) const { return data_ == rhs.data_; }

    /**
     * @brief less than operator
     */
    constexpr bool operator<(opts rhs) const { return data_ < rhs.data_; }

private:
    std::uint8_t data_;
};

/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(retain_handling lhs, rap rhs) { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(retain_handling lhs, nl rhs)  { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(retain_handling lhs, qos rhs) { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(rap lhs, retain_handling rhs) { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(rap lhs, nl rhs)              { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(rap lhs, qos rhs)             { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(nl lhs, retain_handling rhs)  { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(nl lhs, rap rhs)              { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(nl lhs, qos rhs)              { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(qos lhs, retain_handling rhs) { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(qos lhs, rap rhs)             { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(qos lhs, nl rhs)              { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief stringize retain_handling
 */
constexpr char const* retain_handling_to_str(retain_handling v) {
    switch(v) {
    case retain_handling::send:                       return "send";
    case retain_handling::send_only_new_subscription: return "send_only_new_subscription";
    case retain_handling::not_send:                   return "not_send";
    default:                                          return "invalid_retain_handling";
    }
}

/**
 * @related opts
 * @brief output to the stream retain_handling
 */
inline
std::ostream& operator<<(std::ostream& os, retain_handling val)
{
    os << retain_handling_to_str(val);
    return os;
}

/**
 * @related opts
 * @brief stringize rap
 */
constexpr char const* rap_to_str(rap v) {
    switch(v) {
    case rap::dont:   return "dont";
    case rap::retain: return "retain";
    default:          return "invalid_rap";
    }
}

/**
 * @related opts
 * @brief output to the stream rap
 */
inline
std::ostream& operator<<(std::ostream& os, rap val)
{
    os << rap_to_str(val);
    return os;
}

/**
 * @related opts
 * @brief stringize nl
 */
constexpr char const* nl_to_str(nl v) {
    switch(v) {
    case nl::no:   return "no";
    case nl::yes:  return "yes";
    default:       return "invalid_nl";
    }
}

/**
 * @related opts
 * @brief output to the stream nl
 */
inline
std::ostream& operator<<(std::ostream& os, nl val)
{
    os << nl_to_str(val);
    return os;
}

} // namespace sub

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_SUBOPTS_HPP
