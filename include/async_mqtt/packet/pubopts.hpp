// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PUBOPTS_HPP)
#define ASYNC_MQTT_PACKET_PUBOPTS_HPP

#include <boost/assert.hpp>

#include <async_mqtt/packet/qos.hpp>

/// @file

namespace async_mqtt {

namespace pub {

/**
 * @related opts
 * @brief Check fixed header is DUP
 * @param v fixed_header byte
 * @return If DUP return true, otherwise false.
 */
constexpr bool is_dup(std::uint8_t v) {
    return (v & 0b00001000) != 0;
}

/**
 * @related opts
 * @brief Get qos from the fixed header
 * @param v fixed_header byte
 * @return qos
 */
constexpr qos get_qos(std::uint8_t v) {
    return static_cast<qos>((v & 0b00000110) >> 1);
}

/**
 * @related opts
 * @brief Check fixed header is RETAIN
 * @param v fixed_header byte
 * @return If RETAIN return true, otherwise false.
 */
constexpr bool is_retain(std::uint8_t v) {
    return (v & 0b00000001) != 0;
}

/**
 * @related opts
 * @brief Set DUP to the fixed header
 * @param fixed_header fixed_header byte
 * @param dup DUP to set
 */
constexpr void set_dup(std::uint8_t& fixed_header, bool dup) {
    if (dup) fixed_header |=  0b00001000;
    else     fixed_header &= static_cast<std::uint8_t>(~0b00001000);
}

/**
 * @brief MQTT RETAIN
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901104
 */
enum class retain : std::uint8_t {
    yes = 0b00000001, ///< Retain
    no = 0b00000000,  ///< No Retain
};


/**
 * @brief MQTT DUP
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901102
 */
enum class dup : std::uint8_t {
    yes = 0b00001000, ///< Duplicated
    no = 0b00000000,  ///< No Duplicated
};

/**
 * @brief MQTT PublishOptions
 *
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901101
 */
struct opts final {
    constexpr opts() = default;
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
     * @param value retain
     */
    constexpr opts(retain value) : data_(static_cast<std::uint8_t>(value)) { }

    /**
     * @brief constructor
     * @param value dup
     */
    constexpr opts(dup value)    : data_(static_cast<std::uint8_t>(value)) { }

    /**
     * @brief constructor
     * @param value qos
     */
    constexpr opts(qos value)    : data_(static_cast<std::uint8_t>(static_cast<std::uint8_t>(value) << 1))
    {
        BOOST_ASSERT(value == qos::at_most_once || value == qos::at_least_once || value == qos::exactly_once);
    }

    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(opts rhs) const { return opts(data_ | rhs.data_); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(retain rhs) const          { return *this | opts(rhs); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(dup rhs) const             { return *this | opts(rhs); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts operator|(qos rhs) const             { return *this | opts(rhs); }

    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(opts rhs) { return (*this = (*this | rhs)); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(retain rhs)          { return (*this = (*this | rhs)); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(dup rhs)             { return (*this = (*this | rhs)); }
    /**
     * @brief Combine opts operator
     */
    constexpr opts& operator|=(qos rhs)             { return (*this = (*this | rhs)); }

    /**
     * @brief Get retain
     *
     * @return retain
     */
    constexpr retain get_retain() const
    { return static_cast<enum retain>(data_ & 0b00000001); }

    /**
     * @brief Get dup
     *
     * @return dup
     */
    constexpr dup get_dup() const
    { return static_cast<enum dup>(data_ & 0b00001000); }

    /**
     * @brief Get qos
     *
     * @return qos
     */
    constexpr qos get_qos() const
    { return static_cast<enum qos>((data_ & 0b00000110) >> 1); }

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
    std::uint8_t data_ = 0; // defaults to retain::no, dup::no, qos::at_most_once
};

/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(retain lhs, dup rhs) { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(retain lhs, qos rhs) { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(dup lhs, retain rhs) { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(dup lhs, qos rhs)    { return opts(lhs) | rhs; }

/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(qos lhs, retain rhs) { return opts(lhs) | rhs; }
/**
 * @related opts
 * @brief Combine opts operator
 */
constexpr opts operator|(qos lhs, dup rhs)    { return opts(lhs) | rhs; }


/**
 * @related opts
 * @brief stringize retain
 */
constexpr char const* retain_to_str(retain v) {
    switch(v) {
        case retain::yes: return "yes";
        case retain::no:  return "no";
        default:          return "invalid_retain";
    }
}

/**
 * @related opts
 * @brief output to the stream retain
 */
inline
std::ostream& operator<<(std::ostream& os, retain val)
{
    os << retain_to_str(val);
    return os;
}

/**
 * @related opts
 * @brief stringize dup
 */
constexpr char const* dup_to_str(dup v) {
    switch(v) {
        case dup::yes: return "yes";
        case dup::no:  return "no";
        default:       return "invalid_dup";
    }
}


/**
 * @related opts
 * @brief output to the stream dup
 */
inline
std::ostream& operator<<(std::ostream& os, dup val)
{
    os << dup_to_str(val);
    return os;
}

} // namespace pub

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PUBOPTS_HPP
