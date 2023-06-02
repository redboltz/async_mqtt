// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PROPERTY_VARIANT_HPP)
#define ASYNC_MQTT_PACKET_PROPERTY_VARIANT_HPP

#include <async_mqtt/util/variant.hpp>
#include <async_mqtt/packet/property.hpp>
#include <async_mqtt/packet/validate_property.hpp>
#include <async_mqtt/exception.hpp>

namespace async_mqtt {

/**
 * @brief property variant
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
    property_variant(Property&& property):var_{std::forward<Property>(property)}
    {}

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) const& {
        return
            async_mqtt::visit(
                std::forward<Func>(func),
                var_
            );
    }

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) & {
        return
            async_mqtt::visit(
                std::forward<Func>(func),
                var_
            );
    }

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) && {
        return
            async_mqtt::visit(
                std::forward<Func>(func),
                force_move(var_)
            );
    }

    /**
     * @brief Get property_id
     * @return property_id
     */
    property::id id() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.id();
                },
                [] (system_error const&) {
                    BOOST_ASSERT(false);
                    return property::id(0);
                }
            }
        );
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    std::size_t num_of_const_buffer_sequence() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.num_of_const_buffer_sequence();
                },
                [] (system_error const&) {
                    BOOST_ASSERT(false);
                    return std::size_t(0);
                }
            }
        );
    }

    /**
     * @brief Create const buffer sequence.
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.const_buffer_sequence();
                },
                [] (system_error const&) {
                    BOOST_ASSERT(false);
                    return std::vector<as::const_buffer>{};
                }
            }
        );
    }

    /**
     * @brief Get packet size.
     * @return packet size
     */
    std::size_t size() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.size();
                },
                [] (system_error const&) {
                    BOOST_ASSERT(false);
                    return std::size_t(0);
                }
            }
        );
    }

    /**
     * @brief Get by type. If not match, then throw std::bad_variant_access exception.
     * @return actual packet
     */
    template <typename T>
    decltype(auto) get() {
        return std::get<T>(var_);
    }

    /**
     * @brief Get by type. If not match, then throw std::bad_variant_access exception.
     * @return actual packet
     */
    template <typename T>
    decltype(auto) get() const {
        return std::get<T>(var_);
    }

    /**
     * @brief Get by type pointer
     * @return actual packet pointer. If not match then return nullptr.
     */
    template <typename T>
    decltype(auto) get_if() {
        return std::get_if<T>(&var_);
    }

    /**
     * @brief Get by type pointer
     * @return actual packet pointer. If not match then return nullptr.
     */
    template <typename T>
    decltype(auto) get_if() const {
        return std::get_if<T>(&var_);
    }

    operator bool() {
        return var_.index() != 0;
    }

    friend bool operator==(property_variant const& lhs, property_variant const& rhs) {
        return lhs.var_ == rhs.var_;
    }
    friend bool operator<(property_variant const& lhs, property_variant const& rhs) {
        return lhs.var_ < rhs.var_;
    }
    friend std::ostream& operator<<(std::ostream& o, property_variant const& v) {
        v.visit(
            [&] (auto const& p) { o << p; }
        );
        return o;
    }

private:
    using variant_t = variant<
        system_error,
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

using properties = std::vector<property_variant>;

inline std::ostream& operator<<(std::ostream& o, properties const& props) {
    o << "[";
    auto it = props.cbegin();
    auto end = props.cend();

    if (it != end) {
        o << *it++;
    }
    for (; it != end; ++it) {
        o << "," << *it;
    }
    o << "]";
    return o;
}

inline
property_variant make_property_variant(buffer& buf, property_location loc) {
    if (buf.empty()) {
        return make_error(
            errc::bad_message,
            "property doesn't exist"
        );
    }

    try {
        using namespace std::literals;
        auto id = static_cast<property::id>(buf.front());
        if (!validate_property(loc, id)) {
            return make_error(
                errc::bad_message,
                "property "s + property::id_to_str(id) + " is not allowed in " + property_location_to_str(loc)
            );
        }
        buf.remove_prefix(1);
        switch (id) {
        case property::id::payload_format_indicator: {
            if (buf.size() < 1) {
                return make_error(
                    errc::bad_message,
                    "property::payload_format_indicator is invalid"
                );
            }
            auto p = property::payload_format_indicator(buf.begin(), std::next(buf.begin(), 1));
            buf.remove_prefix(1);
            return property_variant(p);
        } break;
        case property::id::message_expiry_interval: {
            if (buf.size() < 4) {
                return make_error(
                    errc::bad_message,
                    "property::message_expiry_interval is invalid"
                );
            }
            auto p = property::message_expiry_interval(buf.begin(), std::next(buf.begin(), 4));
            buf.remove_prefix(4);
            return property_variant(p);
        } break;
        case property::id::content_type: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::content_type length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::content_type is invalid"
                );
            }
            auto p = property::content_type(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::response_topic: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::response_topic length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::response_topic is invalid"
                );
            }
            auto p = property::response_topic(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::correlation_data: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::correlation_data length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::correlation_data is invalid"
                );
            }
            auto p = property::correlation_data(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::subscription_identifier: {
            auto it = buf.begin();
            if (auto val_opt = variable_bytes_to_val(it, buf.end())) {
                auto p = property::subscription_identifier(*val_opt);
                buf.remove_prefix(std::size_t(std::distance(buf.begin(), it)));
                return property_variant(p);
            }
            return make_error(
                errc::bad_message,
                "property::subscription_identifier is invalid"
            );
        } break;
        case property::id::session_expiry_interval: {
            if (buf.size() < 4) {
                return make_error(
                    errc::bad_message,
                    "property::session_expiry_interval is invalid"
                );
            }
            auto p = property::session_expiry_interval(buf.begin(), std::next(buf.begin(), 4));
            buf.remove_prefix(4);
            return property_variant(p);
        } break;
        case property::id::assigned_client_identifier: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::assigned_client_identifier length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::assigned_client_identifier is invalid"
                );
            }
            auto p = property::assigned_client_identifier(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::server_keep_alive: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::server_keep_alive is invalid"
                );
            }
            auto p = property::server_keep_alive(buf.begin(), std::next(buf.begin(), 2));
            buf.remove_prefix(2);
            return property_variant(p);
        } break;
        case property::id::authentication_method: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::authentication_method length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::authentication_method is invalid"
                );
            }
            auto p = property::authentication_method(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::authentication_data: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::authentication_data length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::authentication_data is invalid"
                );
            }
            auto p = property::authentication_data(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::request_problem_information: {
            if (buf.size() < 1) {
                return make_error(
                    errc::bad_message,
                    "property::request_problem_information is invalid"
                );
            }
            auto p = property::request_problem_information(buf.begin(), std::next(buf.begin(), 1));
            buf.remove_prefix(1);
            return property_variant(p);
        } break;
        case property::id::will_delay_interval: {
            if (buf.size() < 4) {
                return make_error(
                    errc::bad_message,
                    "property::will_delay_interval is invalid"
                );
            }
            auto p = property::will_delay_interval(buf.begin(), std::next(buf.begin(), 4));
            buf.remove_prefix(4);
            return property_variant(p);
        } break;
        case property::id::request_response_information: {
            if (buf.size() < 1) {
                return make_error(
                    errc::bad_message,
                    "property::request_response_information is invalid"
                );
            }
            auto p = property::request_response_information(buf.begin(), std::next(buf.begin(), 1));
            buf.remove_prefix(1);
            return property_variant(p);
        } break;
        case property::id::response_information: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::response_information length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::response_information is invalid"
                );
            }
            auto p = property::response_information(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::server_reference: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::server_reference length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::server_reference is invalid"
                );
            }
            auto p = property::server_reference(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::reason_string: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::reason_string length is invalid"
                );
            }
            auto len = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + len) {
                return make_error(
                    errc::bad_message,
                    "property::reason_string is invalid"
                );
            }
            auto p = property::reason_string(buf.substr(2, len));
            buf.remove_prefix(2 + len);
            return property_variant(p);
        } break;
        case property::id::receive_maximum: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::receive_maximum is invalid"
                );
            }
            auto p = property::receive_maximum(buf.begin(), std::next(buf.begin(), 2));
            buf.remove_prefix(2);
            return property_variant(p);
        } break;
        case property::id::topic_alias_maximum: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::topic_alias_maximum is invalid"
                );
            }
            auto p = property::topic_alias_maximum(buf.begin(), std::next(buf.begin(), 2));
            buf.remove_prefix(2);
            return property_variant(p);
        } break;
        case property::id::topic_alias: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::topic_alias is invalid"
                );
            }
            auto p = property::topic_alias(buf.begin(), std::next(buf.begin(), 2));
            buf.remove_prefix(2);
            return property_variant(p);
        } break;
        case property::id::maximum_qos: {
            if (buf.size() < 1) {
                return make_error(
                    errc::bad_message,
                    "property::maximum_qos is invalid"
                );
            }
            auto p = property::maximum_qos(buf.begin(), std::next(buf.begin(), 1));
            buf.remove_prefix(1);
            return property_variant(p);
        } break;
        case property::id::retain_available: {
            if (buf.size() < 1) {
                return make_error(
                    errc::bad_message,
                    "property::reason_string length is invalid"
                );
            }
            auto p = property::retain_available(buf.begin(), std::next(buf.begin(), 1));
            buf.remove_prefix(1);
            return property_variant(p);
        } break;
        case property::id::user_property: {
            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::user_property key length is invalid"
                );
            }
            auto keylen = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + keylen) {
                return make_error(
                    errc::bad_message,
                    "property::user_property key is invalid"
                );
            }
            auto key = buf.substr(2, keylen);
            buf.remove_prefix(2 + keylen);

            if (buf.size() < 2) {
                return make_error(
                    errc::bad_message,
                    "property::user_property val length is invalid"
                );
            }
            auto vallen = endian_load<std::uint16_t>(buf.data());
            if (buf.size() < 2U + vallen) {
                return make_error(
                    errc::bad_message,
                    "property::user_property val is invalid"
                );
            }
            auto val = buf.substr(2, vallen);
            auto p = property::user_property(force_move(key), force_move(val));
            buf.remove_prefix(2 + vallen);

            return property_variant(p);
        } break;
        case property::id::maximum_packet_size: {
            if (buf.size() < 4) {
                return make_error(
                    errc::bad_message,
                    "property::maximum_packet_size is invalid"
                );
            }
            auto p = property::maximum_packet_size(buf.begin(), std::next(buf.begin(), 4));
            buf.remove_prefix(4);
            return property_variant(p);
        } break;
        case property::id::wildcard_subscription_available: {
            if (buf.size() < 1) {
                return make_error(
                    errc::bad_message,
                    "property::wildcard_subscription_available is invalid"
                );
            }
            auto p = property::wildcard_subscription_available(buf.begin(), std::next(buf.begin(), 1));
            buf.remove_prefix(1);
            return property_variant(p);
        } break;
        case property::id::subscription_identifier_available: {
            if (buf.size() < 1) {
                return make_error(
                    errc::bad_message,
                    "property::subscription_identifier_available is invalid"
                );
            }
            auto p = property::subscription_identifier_available(buf.begin(), std::next(buf.begin(), 1));
            buf.remove_prefix(1);
            return property_variant(p);
        } break;
        case property::id::shared_subscription_available: {
            if (buf.size() < 1) {
                return make_error(
                    errc::bad_message,
                    "property::shared_subscription_available is invalid"
                );
            }
            auto p = property::shared_subscription_available(buf.begin(), std::next(buf.begin(), 1));
            buf.remove_prefix(1);
            return property_variant(p);
        } break;
        }
    }
    catch (system_error const& e) {
        return e;
    }
    return make_error(
        errc::bad_message,
        "unknown error"
    );
}

inline
properties make_properties(buffer buf, property_location loc) {
    properties props;
    while (!buf.empty()) {
        if (auto pv = make_property_variant(buf, loc)) {
            props.push_back(force_move(pv));
        }
        else {
            throw pv;
        }
    }

    return props;
}

inline
std::vector<as::const_buffer> const_buffer_sequence(properties const& props) {
    std::vector<as::const_buffer> v;
    for (auto const& p : props) {
        auto cbs = p.const_buffer_sequence();
        std::move(cbs.begin(), cbs.end(), std::back_inserter(v));
    }
    return v;
}

inline
std::size_t size(properties const& props) {
    return
        std::accumulate(
            props.begin(),
            props.end(),
            std::size_t(0U),
            [](std::size_t total, property_variant const& pv) {
                return total + pv.size();
            }
        );
}

inline
std::size_t num_of_const_buffer_sequence(properties const& props) {
    return
        std::accumulate(
            props.begin(),
            props.end(),
            std::size_t(0U),
            [](std::size_t total, property_variant const& pv) {
                return total + pv.num_of_const_buffer_sequence();
            }
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PROPERTY_VARIANT_HPP
