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

/**
 * @defgroup subscribe_options SUBSCRIBE packet flags
 * @ingroup subscribe_v5
 */

namespace async_mqtt {

namespace sub {

/**
 * @ingroup subscribe_options
 * @brief MQTT RetainHandling
 *
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169"></a>
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
enum class retain_handling : std::uint8_t
{
    send = 0b00000000,                       ///< Always send. Same as MQTT v.3.1.1
    send_only_new_subscription = 0b00010000, ///< Only new subscription. Not overwrite.
    not_send = 0b00100000,                   ///< Always not send.
};

/**
 * @ingroup subscribe_options
 * @brief MQTT RetainAsPublished
 *
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169"></a>
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
enum class rap : std::uint8_t
{
    dont = 0b00000000,   ///< Retain is set to 0 delivery on publish by the broker. Same as MQTT v.3.1.1
    retain = 0b00001000, ///< Preserve published Retain delivery on publish by the broker.
};

/**
 * @ingroup subscribe_options
 * @brief MQTT NoLocal
 *
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169"></a>
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
enum class nl : std::uint8_t
{
    no = 0b00000000,  ///< Subscriber's publish would be delivered to the subscriber itself. Same as MQTT v.3.1.1
    yes = 0b00000100, ///< Subscriber's publish would not be delivered to the subscriber itself.
};

/**
 * @ingroup subscribe_options
 * @brief MQTT SubscribeOptions
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169"></a>
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
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
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts operator|(opts rhs) const { return opts(data_ | rhs.data_); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts operator|(retain_handling rhs) const   { return *this | opts(rhs); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts operator|(rap rhs) const               { return *this | opts(rhs); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts operator|(nl rhs) const                { return *this | opts(rhs); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts operator|(qos rhs) const               { return *this | opts(rhs); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts& operator|=(opts rhs) { return (*this = (*this | rhs)); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts& operator|=(retain_handling rhs)   { return (*this = (*this | rhs)); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts& operator|=(rap rhs)               { return (*this = (*this | rhs)); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
     */
    constexpr opts& operator|=(nl rhs)                { return (*this = (*this | rhs)); }

    /**
     * @brief Combine opts operator
     * @param rhs combined target
     * @return conbined opts
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
     * @param rhs compare target
     * @return true if this opts equal to the rhs, otherwise false.
     */
    constexpr bool operator==(opts rhs) const { return data_ == rhs.data_; }

    /**
     * @brief less than operator
     * @param rhs compare target
     * @return true if this opts less than the rhs, otherwise false.
     */
    constexpr bool operator<(opts rhs) const { return data_ < rhs.data_; }

private:
    std::uint8_t data_;
};

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(retain_handling lhs, rap rhs) { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(retain_handling lhs, nl rhs)  { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(retain_handling lhs, qos rhs) { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(rap lhs, retain_handling rhs) { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(rap lhs, nl rhs)              { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(rap lhs, qos rhs)             { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(nl lhs, retain_handling rhs)  { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(nl lhs, rap rhs)              { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(nl lhs, qos rhs)              { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(qos lhs, retain_handling rhs) { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(qos lhs, rap rhs)             { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 * @param lhs combined target
 * @param rhs combined target
 * @return conbined opts
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr opts operator|(qos lhs, nl rhs)              { return opts(lhs) | rhs; }

/**
 * @ingroup subscribe_options
 * @related opts
 * @brief stringize retain_handling
 * @param v target
 * @return retain_handling string
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
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
 * @ingroup subscribe_options
 * @related opts
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
inline
std::ostream& operator<<(std::ostream& o, retain_handling v)
{
    o << retain_handling_to_str(v);
    return o;
}

/**
 * @ingroup subscribe_options
 * @related opts
 * @brief stringize rap(retain as published)
 * @param v target
 * @return rap(retain as published) string
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr char const* rap_to_str(rap v) {
    switch(v) {
    case rap::dont:   return "dont";
    case rap::retain: return "retain";
    default:          return "invalid_rap";
    }
}

/**
 * @ingroup subscribe_options
 * @related opts
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
inline
std::ostream& operator<<(std::ostream& o, rap v)
{
    o << rap_to_str(v);
    return o;
}

/**
 * @ingroup subscribe_options
 * @related opts
 * @brief stringize nl(no local)
 * @param v target
 * @return nl(no local) string
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
constexpr char const* nl_to_str(nl v) {
    switch(v) {
    case nl::no:   return "no";
    case nl::yes:  return "yes";
    default:       return "invalid_nl";
    }
}

/**
 * @ingroup subscribe_options
 * @related opts
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/subopts.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
inline
std::ostream& operator<<(std::ostream& o, nl v)
{
    o << nl_to_str(v);
    return o;
}

} // namespace sub

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_SUBOPTS_HPP
