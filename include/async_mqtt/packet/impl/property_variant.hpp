// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_PROPERTY_VARIANT_HPP)
#define ASYNC_MQTT_PACKET_IMPL_PROPERTY_VARIANT_HPP

#include <variant>

#include <async_mqtt/util/overload.hpp>
#include <async_mqtt/packet/property.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/impl/validate_property.hpp>
#include <async_mqtt/exception.hpp>

namespace async_mqtt {

template <
    typename Property,
    std::enable_if_t<
        !std::is_same_v<std::decay_t<Property>, property_variant>,
        std::nullptr_t
    >*
>
inline
property_variant::property_variant(Property&& property):var_{std::forward<Property>(property)}
{}

template <typename Func>
inline
auto property_variant::visit(Func&& func) const& {
    return
        std::visit(
            std::forward<Func>(func),
            var_
        );
}

template <typename Func>
inline
auto property_variant::visit(Func&& func) & {
    return
        std::visit(
            std::forward<Func>(func),
            var_
        );
}

template <typename Func>
inline
auto property_variant::visit(Func&& func) && {
    return
        std::visit(
            std::forward<Func>(func),
            force_move(var_)
        );
}

inline
property::id property_variant::id() const {
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


inline
std::size_t property_variant::num_of_const_buffer_sequence() const {
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

inline
std::vector<as::const_buffer> property_variant::const_buffer_sequence() const {
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

inline
std::size_t property_variant::size() const {
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

template <typename T>
inline
decltype(auto) property_variant::get() {
    return std::get<T>(var_);
}

template <typename T>
inline
decltype(auto) property_variant::get() const {
    return std::get<T>(var_);
}

template <typename T>
inline
decltype(auto) property_variant::get_if() {
    return std::get_if<T>(&var_);
}

template <typename T>
inline
decltype(auto) property_variant::get_if() const {
    return std::get_if<T>(&var_);
}

inline
property_variant::operator bool() {
    return var_.index() != 0;
}

inline
std::ostream& operator<<(std::ostream& o, properties const& props) {
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
bool operator==(property_variant const& lhs, property_variant const& rhs) {
    return lhs.var_ == rhs.var_;
}

inline
bool operator<(property_variant const& lhs, property_variant const& rhs) {
    return lhs.var_ < rhs.var_;
}

inline
std::ostream& operator<<(std::ostream& o, property_variant const& v) {
    v.visit(
        [&] (auto const& p) { o << p; }
    );
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

#endif // ASYNC_MQTT_PACKET_IMPL_PROPERTY_VARIANT_HPP
