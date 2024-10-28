// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PROPERTY_VARIANT_IPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PROPERTY_VARIANT_IPP

#include <variant>

#include <async_mqtt/util/overload.hpp>
#include <async_mqtt/util/inline.hpp>
#include <async_mqtt/protocol/packet/property.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>
#include <async_mqtt/protocol/packet/impl/property_variant.hpp>
#include <async_mqtt/protocol/packet/impl/validate_property.hpp>

namespace async_mqtt {

ASYNC_MQTT_HEADER_ONLY_INLINE
property::id property_variant::id() const {
    return visit(
        [] (auto const& p) {
            return p.id();
        }
    );
}


ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t property_variant::num_of_const_buffer_sequence() const {
    return visit(
        [] (auto const& p) {
            return p.num_of_const_buffer_sequence();
        }
    );
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> property_variant::const_buffer_sequence() const {
    return visit(
        [] (auto const& p) {
            return p.const_buffer_sequence();
        }
    );
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t property_variant::size() const {
    return visit(
        [] (auto const& p) {
            return p.size();
        }
    );
}

ASYNC_MQTT_HEADER_ONLY_INLINE
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

ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator==(property_variant const& lhs, property_variant const& rhs) {
    return lhs.var_ == rhs.var_;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator<(property_variant const& lhs, property_variant const& rhs) {
    return lhs.var_ < rhs.var_;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, property_variant const& v) {
    v.visit(
        [&] (auto const& p) { o << p; }
    );
    return o;
}


ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<property_variant>
make_property_variant(buffer& buf, property_location loc, error_code& ec) {
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return std::nullopt;
    }

    using namespace std::literals;
    auto id = static_cast<property::id>(buf.front());
    if (!validate_property(loc, id)) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return std::nullopt;
     }
    buf.remove_prefix(1);
    switch (id) {
    case property::id::payload_format_indicator: {
        if (buf.size() < 1) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::payload_format_indicator(buf.begin(), std::next(buf.begin(), 1), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(1);
        return property_variant(p);
    } break;
    case property::id::message_expiry_interval: {
        if (buf.size() < 4) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::message_expiry_interval(buf.begin(), std::next(buf.begin(), 4));
        buf.remove_prefix(4);
        return property_variant(p);
    } break;
    case property::id::content_type: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::content_type(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::response_topic: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::response_topic(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::correlation_data: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::correlation_data(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::subscription_identifier: {
        auto it = buf.begin();
        if (auto val_opt = variable_bytes_to_val(it, buf.end())) {
            auto p = property::subscription_identifier(*val_opt, ec);
            if (ec) return std::nullopt;
            buf.remove_prefix(std::size_t(std::distance(buf.begin(), it)));
            return property_variant(p);
        }
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return std::nullopt;
    } break;
    case property::id::session_expiry_interval: {
        if (buf.size() < 4) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::session_expiry_interval(buf.begin(), std::next(buf.begin(), 4));
        buf.remove_prefix(4);
        return property_variant(p);
    } break;
    case property::id::assigned_client_identifier: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::assigned_client_identifier(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::server_keep_alive: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::server_keep_alive(buf.begin(), std::next(buf.begin(), 2));
        buf.remove_prefix(2);
        return property_variant(p);
    } break;
    case property::id::authentication_method: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::authentication_method(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::authentication_data: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::authentication_data(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::request_problem_information: {
        if (buf.size() < 1) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::request_problem_information(buf.begin(), std::next(buf.begin(), 1));
        buf.remove_prefix(1);
        return property_variant(p);
    } break;
    case property::id::will_delay_interval: {
        if (buf.size() < 4) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::will_delay_interval(buf.begin(), std::next(buf.begin(), 4));
        buf.remove_prefix(4);
        return property_variant(p);
    } break;
    case property::id::request_response_information: {
        if (buf.size() < 1) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::request_response_information(buf.begin(), std::next(buf.begin(), 1));
        buf.remove_prefix(1);
        return property_variant(p);
    } break;
    case property::id::response_information: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::response_information(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::server_reference: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::server_reference(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::reason_string: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto len = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + len) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::reason_string(buf.substr(2, len), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + len);
        return property_variant(p);
    } break;
    case property::id::receive_maximum: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::receive_maximum(buf.begin(), std::next(buf.begin(), 2), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2);
        return property_variant(p);
    } break;
    case property::id::topic_alias_maximum: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::topic_alias_maximum(buf.begin(), std::next(buf.begin(), 2));
        buf.remove_prefix(2);
        return property_variant(p);
    } break;
    case property::id::topic_alias: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::topic_alias(buf.begin(), std::next(buf.begin(), 2));
        buf.remove_prefix(2);
        return property_variant(p);
    } break;
    case property::id::maximum_qos: {
        if (buf.size() < 1) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::maximum_qos(buf.begin(), std::next(buf.begin(), 1), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(1);
        return property_variant(p);
    } break;
    case property::id::retain_available: {
        if (buf.size() < 1) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::retain_available(buf.begin(), std::next(buf.begin(), 1));
        buf.remove_prefix(1);
        return property_variant(p);
    } break;
    case property::id::user_property: {
        if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto keylen = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + keylen) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto key = buf.substr(2, keylen);
        buf.remove_prefix(2 + keylen);
         if (buf.size() < 2) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto vallen = endian_load<std::uint16_t>(buf.data());
        if (buf.size() < 2U + vallen) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto val = buf.substr(2, vallen);
        auto p = property::user_property(force_move(key), force_move(val), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(2 + vallen);
         return property_variant(p);
    } break;
    case property::id::maximum_packet_size: {
        if (buf.size() < 4) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::maximum_packet_size(buf.begin(), std::next(buf.begin(), 4), ec);
        if (ec) return std::nullopt;
        buf.remove_prefix(4);
        return property_variant(p);
    } break;
    case property::id::wildcard_subscription_available: {
        if (buf.size() < 1) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::wildcard_subscription_available(buf.begin(), std::next(buf.begin(), 1));
        buf.remove_prefix(1);
        return property_variant(p);
    } break;
    case property::id::subscription_identifier_available: {
        if (buf.size() < 1) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::subscription_identifier_available(buf.begin(), std::next(buf.begin(), 1));
        buf.remove_prefix(1);
        return property_variant(p);
    } break;
    case property::id::shared_subscription_available: {
        if (buf.size() < 1) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return std::nullopt;
        }
        auto p = property::shared_subscription_available(buf.begin(), std::next(buf.begin(), 1));
        buf.remove_prefix(1);
        return property_variant(p);
    } break;
    }
    BOOST_ASSERT(false);
    return std::nullopt;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
properties make_properties(buffer buf, property_location loc, error_code& ec) {
    properties props;
    while (!buf.empty()) {
        auto pv_opt = make_property_variant(buf, loc, ec);
        if (ec) return properties{};
        BOOST_ASSERT(pv_opt);
        props.push_back(force_move(*pv_opt));
    }

    return props;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> const_buffer_sequence(properties const& props) {
    std::vector<as::const_buffer> v;
    for (auto const& p : props) {
        auto cbs = p.const_buffer_sequence();
        std::move(cbs.begin(), cbs.end(), std::back_inserter(v));
    }
    return v;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
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

ASYNC_MQTT_HEADER_ONLY_INLINE
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

#endif // ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PROPERTY_VARIANT_IPP
