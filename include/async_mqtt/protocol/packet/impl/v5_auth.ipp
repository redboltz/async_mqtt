// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V5_AUTH_IPP)
#define ASYNC_MQTT_PACKET_IMPL_V5_AUTH_IPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/v5_auth.hpp>
#include <async_mqtt/packet/impl/packet_helper.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/inline.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/scope_guard.hpp>

#include <async_mqtt/packet/detail/fixed_header.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/impl/copy_to_static_vector.hpp>
#include <async_mqtt/packet/impl/validate_property.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

ASYNC_MQTT_HEADER_ONLY_INLINE
auth_packet::auth_packet(
    auth_reason_code reason_code,
    properties props
) : auth_packet{
        std::optional<auth_reason_code>(reason_code),
        force_move(props)
    }
{}

ASYNC_MQTT_HEADER_ONLY_INLINE
auth_packet::auth_packet(
) : auth_packet{
        std::nullopt,
        properties{}
    }
{}

ASYNC_MQTT_HEADER_ONLY_INLINE
auth_packet::auth_packet(
    auth_reason_code reason_code
) : auth_packet{
        std::optional<auth_reason_code>(reason_code),
        properties{}
    }
{}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> auth_packet::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;
    ret.reserve(num_of_const_buffer_sequence());
    ret.emplace_back(as::buffer(&fixed_header_, 1));
    ret.emplace_back(as::buffer(remaining_length_buf_.data(), remaining_length_buf_.size()));

    if (reason_code_) {
        ret.emplace_back(as::buffer(&*reason_code_, 1));

        if (property_length_buf_.size() != 0) {
            ret.emplace_back(as::buffer(property_length_buf_.data(), property_length_buf_.size()));
            auto props_cbs = async_mqtt::const_buffer_sequence(props_);
            std::move(props_cbs.begin(), props_cbs.end(), std::back_inserter(ret));
        }
    }

    return ret;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t auth_packet::size() const {
    return
        1 +                            // fixed header
        remaining_length_buf_.size() +
        remaining_length_;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t auth_packet::num_of_const_buffer_sequence() const {
    return
        1 +                   // fixed header
        1 +                   // remaining length
        [&] () -> std::size_t {
            if (reason_code_) {
                return
                    1 +       // reason_code
                    [&] () -> std::size_t {
                        if (property_length_buf_.size() == 0) return 0;
                        return
                            1 +                   // property length
                            async_mqtt::num_of_const_buffer_sequence(props_);
                    }();
            }
            return 0;
        }();
}

ASYNC_MQTT_HEADER_ONLY_INLINE
auth_reason_code auth_packet::code() const {
    if (reason_code_) return *reason_code_;
    return auth_reason_code::success;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
properties const& auth_packet::props() const {
    return props_;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
auth_packet::auth_packet(
    std::optional<auth_reason_code> reason_code,
    properties props
)
    : fixed_header_{
          detail::make_fixed_header(control_packet_type::auth, 0b0000)
      },
      remaining_length_{
          0
      },
      reason_code_{reason_code},
      property_length_(async_mqtt::size(props)),
      props_(force_move(props))
{
    using namespace std::literals;

    auto guard = unique_scope_guard(
        [&] {
            auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
            for (auto e : rb) {
                remaining_length_buf_.push_back(e);
            }
        }
    );

    if (!reason_code_) return;
    remaining_length_ += 1;

    if (property_length_ == 0) return;

    auto pb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(property_length_));
    for (auto e : pb) {
        property_length_buf_.push_back(e);
    }

    for (auto const& prop : props_) {
        auto id = prop.id();
        if (!validate_property(property_location::auth, id)) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            );
        }
    }

    remaining_length_ += property_length_buf_.size() + property_length_;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
auth_packet::auth_packet(buffer buf, error_code& ec) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    fixed_header_ = static_cast<std::uint8_t>(buf.front());
    buf.remove_prefix(1);

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

    if (remaining_length_ == 0) {
        if (!buf.empty()) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        ec = error_code{};
        return;
    }

    // connect_reason_code
    reason_code_.emplace(static_cast<auth_reason_code>(buf.front()));
    buf.remove_prefix(1);
    switch (*reason_code_) {
    case auth_reason_code::success:
    case auth_reason_code::continue_authentication:
    case auth_reason_code::re_authenticate:
        break;
    default:
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    if (remaining_length_ == 1) {
        if (!buf.empty()) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        ec = error_code{};
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
        props_ = make_properties(prop_buf, property_location::auth, ec);
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
    ec = error_code{};
}

ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator==(auth_packet const& lhs, auth_packet const& rhs) {
    return detail::equal(lhs, rhs);
}

ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator<(auth_packet const& lhs, auth_packet const& rhs) {
    return detail::less_than(lhs, rhs);
}

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_IMPL_V5_AUTH_IPP
