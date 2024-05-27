// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V5_CONNACK_HPP)
#define ASYNC_MQTT_PACKET_IMPL_V5_CONNACK_HPP

#include <utility>
#include <numeric>

#include <async_mqtt/packet/v5_connack.hpp>
#include <async_mqtt/packet/impl/packet_helper.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/detail/fixed_header.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/impl/copy_to_static_vector.hpp>
#include <async_mqtt/packet/impl/session_present.hpp>
#include <async_mqtt/packet/impl/validate_property.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

inline
connack_packet::connack_packet(
    bool session_present,
    connect_reason_code reason_code,
    properties props
)
    : fixed_header_{
          detail::make_fixed_header(control_packet_type::connack, 0b0000)
      },
      remaining_length_(
          1 + // connect acknowledge flags
          1   // reason code
      ),
      connect_acknowledge_flags_(
          session_present ? 1 : 0
      ),
      reason_code_{reason_code},
      property_length_{async_mqtt::size(props)},
      props_(force_move(props))
{
    using namespace std::literals;
    auto pb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(property_length_));
    for (auto e : pb) {
        property_length_buf_.push_back(e);
    }

    for (auto const& prop : props_) {
        auto id = prop.id();
        if (!validate_property(property_location::connack, id)) {
            throw system_error{
                make_error_code(
                    connect_reason_code::protocol_error
                )
            };
        }
    }

    remaining_length_ += property_length_buf_.size() + property_length_;
    auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
    for (auto e : rb) {
        remaining_length_buf_.push_back(e);
    }
}

inline
constexpr control_packet_type connack_packet::type() {
    return control_packet_type::connack;
}

inline
std::vector<as::const_buffer> connack_packet::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;
    ret.reserve(num_of_const_buffer_sequence());

    ret.emplace_back(as::buffer(&fixed_header_, 1));
    ret.emplace_back(as::buffer(remaining_length_buf_.data(), remaining_length_buf_.size()));
    ret.emplace_back(as::buffer(&connect_acknowledge_flags_, 1));
    ret.emplace_back(as::buffer(&reason_code_, 1));

    ret.emplace_back(as::buffer(property_length_buf_.data(), property_length_buf_.size()));
    auto props_cbs = async_mqtt::const_buffer_sequence(props_);
    std::move(props_cbs.begin(), props_cbs.end(), std::back_inserter(ret));

    return ret;
}

inline
std::size_t connack_packet::size() const {
    return
        1 +                            // fixed header
        remaining_length_buf_.size() +
        remaining_length_;
}

inline
std::size_t connack_packet::num_of_const_buffer_sequence() const {
    return
        1 +                   // fixed header
        1 +                   // remaining length
        1 +                   // connect_acknowledge_flags
        1 +                   // reason_code
        1 +                   // property length
        async_mqtt::num_of_const_buffer_sequence(props_);
}

inline
bool connack_packet::session_present() const {
    return is_session_present(static_cast<char>(connect_acknowledge_flags_));
}

inline
connect_reason_code connack_packet::code() const {
    return reason_code_;
}

inline
properties const& connack_packet::props() const {
    return props_;
}

inline
connack_packet::connack_packet(buffer buf, error_code& ec) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    fixed_header_ = static_cast<std::uint8_t>(buf.front());
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(fixed_header_);
    if (!cpt_opt || *cpt_opt != control_packet_type::connack) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // remaining_length
    if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
        remaining_length_ = *vl_opt;
    }
    else {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    if (remaining_length_ != buf.size()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // connect_acknowledge_flags
    if (buf.size() < 1) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    connect_acknowledge_flags_ = static_cast<std::uint8_t>(buf.front());
    buf.remove_prefix(1);
    if ((connect_acknowledge_flags_ & 0b11111110)!= 0) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    // reason_code
    reason_code_ = static_cast<connect_reason_code>(buf.front());
    buf.remove_prefix(1);
    switch (reason_code_) {
    case connect_reason_code::success:
    case connect_reason_code::unspecified_error:
    case connect_reason_code::malformed_packet:
    case connect_reason_code::protocol_error:
    case connect_reason_code::implementation_specific_error:
    case connect_reason_code::unsupported_protocol_version:
    case connect_reason_code::client_identifier_not_valid:
    case connect_reason_code::bad_user_name_or_password:
    case connect_reason_code::not_authorized:
    case connect_reason_code::server_unavailable:
    case connect_reason_code::server_busy:
    case connect_reason_code::banned:
    case connect_reason_code::bad_authentication_method:
    case connect_reason_code::topic_name_invalid:
    case connect_reason_code::packet_too_large:
    case connect_reason_code::quota_exceeded:
    case connect_reason_code::payload_format_invalid:
    case connect_reason_code::retain_not_supported:
    case connect_reason_code::qos_not_supported:
    case connect_reason_code::use_another_server:
    case connect_reason_code::server_moved:
    case connect_reason_code::connection_rate_exceeded:
        break;
    default:
        ec = make_error_code(
            disconnect_reason_code::protocol_error
        );
        return;
    }

    // property
    auto it = buf.begin();
    if (auto pl_opt = variable_bytes_to_val(it, buf.end())) {
        property_length_ = *pl_opt;
        std::copy(buf.begin(), it, std::back_inserter(property_length_buf_));
        buf.remove_prefix(std::size_t(std::distance(buf.begin(), it)));
        if (buf.size() < property_length_) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        auto prop_buf = buf.substr(0, property_length_);
        props_ = make_properties(prop_buf, property_location::connack, ec);
        if (ec) return;
        buf.remove_prefix(property_length_);
    }
    else {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    if (!buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
}

inline
bool operator==(connack_packet const& lhs, connack_packet const& rhs) {
    return detail::equal(lhs, rhs);
}

inline
bool operator<(connack_packet const& lhs, connack_packet const& rhs) {
    return detail::less_than(lhs, rhs);
}

inline
std::ostream& operator<<(std::ostream& o, connack_packet const& v) {
    o <<
        "v5::connack{" <<
        "rc:" << v.code() << "," <<
        "sp:" << v.session_present();
    if (!v.props().empty()) {
        o << ",ps:" << v.props();
    };
    o << "}";
    return o;
}

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_IMPL_V5_CONNACK_HPP
