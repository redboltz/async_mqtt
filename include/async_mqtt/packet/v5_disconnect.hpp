// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_DISCONNECT_HPP)
#define ASYNC_MQTT_PACKET_V5_DISCONNECT_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/scope_guard.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @brief MQTT DISCONNECT packet (v5)
 *
 * When the endpoint sends DISCONNECT packet, then the endpoint become disconnecting status.
 * The endpoint can't send packets any more.
 * The underlying layer is not automatically closed from the client side.
 * If you want to close the underlying layer from the client side, you need to call basic_endpoint::close()
 * after sending DISCONNECT packet.
 * When the broker receives DISCONNECT packet, then close underlying layer from the broker.
 * In this case, Will is not published by the broker except reason_code is Disconnect with Will Message.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901205
 */
class disconnect_packet {
public:
    /**
     * @brief constructor
     *
     * @param reason_code DisonnectReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901208
     * @param props       properties.
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901209
     */
    disconnect_packet(
        disconnect_reason_code reason_code,
        properties props
    ) : disconnect_packet{
            optional<disconnect_reason_code>(reason_code),
            force_move(props)
        }
    {}

    /**
     * @brief constructor
     */
    disconnect_packet(
    ) : disconnect_packet{
            nullopt,
            properties{}
        }
    {}

    /**
     * @brief constructor
     *
     * @param reason_code DisonnectReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901208
     */
    disconnect_packet(
        disconnect_reason_code reason_code
    ) : disconnect_packet{
            optional<disconnect_reason_code>(reason_code),
            properties{}
        }
    {}

    disconnect_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::disconnect_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);

        // remaining_length
        if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
            remaining_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v5::disconnect_packet remaining length is invalid");
        }

        if (remaining_length_ == 0) {
            if (!buf.empty()) {
                throw make_error(errc::bad_message, "v5::disconnect_packet remaining length is invalid");
            }
            return;
        }

        // connect_reason_code
        reason_code_.emplace(static_cast<disconnect_reason_code>(buf.front()));
        buf.remove_prefix(1);
        switch (*reason_code_) {
        case disconnect_reason_code::normal_disconnection:
        case disconnect_reason_code::disconnect_with_will_message:
        case disconnect_reason_code::unspecified_error:
        case disconnect_reason_code::malformed_packet:
        case disconnect_reason_code::protocol_error:
        case disconnect_reason_code::implementation_specific_error:
        case disconnect_reason_code::not_authorized:
        case disconnect_reason_code::server_busy:
        case disconnect_reason_code::server_shutting_down:
        case disconnect_reason_code::keep_alive_timeout:
        case disconnect_reason_code::session_taken_over:
        case disconnect_reason_code::topic_filter_invalid:
        case disconnect_reason_code::topic_name_invalid:
        case disconnect_reason_code::receive_maximum_exceeded:
        case disconnect_reason_code::topic_alias_invalid:
        case disconnect_reason_code::packet_too_large:
        case disconnect_reason_code::message_rate_too_high:
        case disconnect_reason_code::quota_exceeded:
        case disconnect_reason_code::administrative_action:
        case disconnect_reason_code::payload_format_invalid:
        case disconnect_reason_code::retain_not_supported:
        case disconnect_reason_code::qos_not_supported:
        case disconnect_reason_code::use_another_server:
        case disconnect_reason_code::server_moved:
        case disconnect_reason_code::shared_subscriptions_not_supported:
        case disconnect_reason_code::connection_rate_exceeded:
        case disconnect_reason_code::maximum_connect_time:
        case disconnect_reason_code::subscription_identifiers_not_supported:
        case disconnect_reason_code::wildcard_subscriptions_not_supported:
            break;
        default:
            throw make_error(
                errc::bad_message,
                "v5::disconnect_packet connect reason_code is invalid"
            );
            break;
        }

        if (remaining_length_ == 1) {
            if (!buf.empty()) {
                throw make_error(errc::bad_message, "v5::disconnect_packet remaining length is invalid");
            }
            return;
        }

        // property
        auto it = buf.begin();
        if (auto pl_opt = variable_bytes_to_val(it, buf.end())) {
            property_length_ = *pl_opt;
            std::copy(buf.begin(), it, std::back_inserter(property_length_buf_));
            buf.remove_prefix(std::size_t(std::distance(buf.begin(), it)));
            if (buf.size() < property_length_) {
                throw make_error(
                    errc::bad_message,
                    "v5::disconnect_packet properties_don't match its length"
                );
            }
            auto prop_buf = buf.substr(0, property_length_);
            props_ = make_properties(prop_buf, property_location::disconnect);
            buf.remove_prefix(property_length_);
        }
        else {
            throw make_error(
                errc::bad_message,
                "v5::disconnect_packet property_length is invalid"
            );
        }

        if (!buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::disconnect_packet properties don't match its length"
            );
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::disconnect;
    }

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
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

    /**
     * @brief Get packet size.
     * @return packet size
     */
    std::size_t size() const {
        return
            1 +                            // fixed header
            remaining_length_buf_.size() +
            remaining_length_;
    }

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    std::size_t num_of_const_buffer_sequence() const {
        return
            1 +                   // fixed header
            1 +                   // remaining length
            1 +                   // reason_code
            [&] () -> std::size_t {
                if (property_length_buf_.size() == 0) return 0;
                return
                    1 +                   // property length
                    async_mqtt::num_of_const_buffer_sequence(props_);
            }();
    }

    /**
     * @breif Get reason code
     * @return reason_code
     */
    disconnect_reason_code code() const {
        if (reason_code_) return *reason_code_;
        return disconnect_reason_code::normal_disconnection;
    }

    /**
     * @breif Get properties
     * @return properties
     */
    properties const& props() const {
        return props_;
    }

    friend
    inline std::ostream& operator<<(std::ostream& o, disconnect_packet const& v) {
        o <<
            "v5::disconnect{";
        if (v.reason_code_) {
            o << "rc:" << *v.reason_code_;
        }
        if (!v.props().empty()) {
            o << ",ps:" << v.props();
        };
        o << "}";
        return o;
    }

private:
    disconnect_packet(
        optional<disconnect_reason_code> reason_code,
        properties props
    )
        : fixed_header_{
              make_fixed_header(control_packet_type::disconnect, 0b0000)
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
            if (!validate_property(property_location::disconnect, id)) {
                throw make_error(
                    errc::bad_message,
                    "v5::disconnect_packet property "s + id_to_str(id) + " is not allowed"
                );
            }
        }

        remaining_length_ += property_length_buf_.size() + property_length_;
    }

private:
    std::uint8_t fixed_header_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;

    optional<disconnect_reason_code> reason_code_;

    std::size_t property_length_ = 0;
    static_vector<char, 4> property_length_buf_;
    properties props_;
};

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_V5_DISCONNECT_HPP
