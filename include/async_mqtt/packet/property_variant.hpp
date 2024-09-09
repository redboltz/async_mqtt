// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PROPERTY_VARIANT_HPP)
#define ASYNC_MQTT_PACKET_PROPERTY_VARIANT_HPP

#include <variant>

#include <async_mqtt/util/overload.hpp>
#include <async_mqtt/packet/property.hpp>

namespace async_mqtt {

/**
 * @defgroup property_variant variant class for all properties
 * @ingroup property
 */

/**
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
bool operator==(property_variant const& lhs, property_variant const& rhs);

/**
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
bool operator<(property_variant const& lhs, property_variant const& rhs);

/*
 * @brief output to the stream
 * @param os output stream
 * @param v  target
 * @return output stream
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, property_variant const& v);


/**
 * @ingroup property_variant
 * @brief property variant
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
class property_variant {
public:

    /**
     * @brief constructor
     * @param property property
     */
    template <
        typename Property,
        std::enable_if_t<
            !std::is_same_v<std::decay_t<Property>, property_variant>,
            std::nullptr_t
        >* = nullptr
    >
    property_variant(Property&& property);

    /**
     * @brief constructor
     *        property variant value is std::monostate
     */
    explicit property_variant() = default;

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) const&;

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) &;

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) &&;

    /**
     * @brief Get property_id
     * @return property_id
     */
    property::id id() const;

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    std::size_t num_of_const_buffer_sequence() const;

    /**
     * @brief Create const buffer sequence.
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const;

    /**
     * @brief Get packet size.
     * @return packet size
     */
    std::size_t size() const;

    /**
     * @brief Get by type. If not match, then throw std::bad_variant_access exception.
     * @return actual packet
     */
    template <typename T>
    decltype(auto) get();

    /**
     * @brief Get by type. If not match, then throw std::bad_variant_access exception.
     * @return actual packet
     */
    template <typename T>
    decltype(auto) get() const;

    /**
     * @brief Get by type pointer
     * @return actual packet pointer. If not match then return nullptr.
     */
    template <typename T>
    decltype(auto) get_if();

    /**
     * @brief Get by type pointer
     * @return actual packet pointer. If not match then return nullptr.
     */
    template <typename T>
    decltype(auto) get_if() const;

    /**
     * @brief Check the property is valid
     * @return true if the property is valid, other
     */
    operator bool();

    friend bool operator==(property_variant const& lhs, property_variant const& rhs);
    friend bool operator<(property_variant const& lhs, property_variant const& rhs);
    friend std::ostream& operator<<(std::ostream& o, property_variant const& v);

private:
    using variant_t = std::variant<
        std::monostate,
        property::payload_format_indicator,
        property::message_expiry_interval,
        property::content_type,
        property::response_topic,
        property::correlation_data,
        property::subscription_identifier,
        property::session_expiry_interval,
        property::assigned_client_identifier,
        property::server_keep_alive,
        property::authentication_method,
        property::authentication_data,
        property::request_problem_information,
        property::will_delay_interval,
        property::request_response_information,
        property::response_information,
        property::server_reference,
        property::reason_string,
        property::receive_maximum,
        property::topic_alias_maximum,
        property::topic_alias,
        property::maximum_qos,
        property::retain_available,
        property::user_property,
        property::maximum_packet_size,
        property::wildcard_subscription_available,
        property::subscription_identifier_available,
        property::shared_subscription_available
    >;

    variant_t var_;
};

/**
 * @ingroup property
 * @brief property variant collection type
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
using properties = std::vector<property_variant>;

/*
 * @ingroup property
 * @brief output to the stream
 * @param os output stream
 * @param v  target
 * @return output stream
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, properties const& props);

/**
 * @related property_variant
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
bool operator<(property_variant const& lhs, property_variant const& rhs);

/**
 * @related property_variant
 * @brief equal to operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
bool operator==(property_variant const& lhs, property_variant const& rhs);

/**
 * @related property_variant
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * - Header: async_mqtt/packet/property_variant.hpp
 * - Convenience header: async_mqtt/all.hpp
 *
 */
std::ostream& operator<<(std::ostream& o, property_variant const& v);

} // namespace async_mqtt

#include <async_mqtt/packet/impl/property_variant.hpp>

#endif // ASYNC_MQTT_PACKET_PROPERTY_VARIANT_HPP
