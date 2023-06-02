// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_CONNACK_HPP)
#define ASYNC_MQTT_PACKET_V5_CONNACK_HPP

#include <utility>
#include <numeric>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/session_present.hpp>
#include <async_mqtt/packet/reason_code.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @bried MQTT CONNACK packet (v5)
 *
 * Only MQTT broker(sever) can send this packet.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901074
 */
class connack_packet {
public:

    /**
     * @bried constructor
     * @param session_present If the broker stores the session, then true, otherwise false.
     *                        When the endpoint receives CONNACK packet with session_present is false,
     *                        then stored packets are erased.
     * @param reason_code ConnectReasonCode
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901079
     * @param props       properties.
     *                    \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901080
     */
    connack_packet(
        bool session_present,
        connect_reason_code reason_code,
        properties props = {}
    )
        : fixed_header_{
              make_fixed_header(control_packet_type::connack, 0b0000)
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
                throw make_error(
                    errc::bad_message,
                    "v5::connack_packet property "s + id_to_str(id) + " is not allowed"
                );
            }
        }

        remaining_length_ += property_length_buf_.size() + property_length_;
        auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }
    }

    connack_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::connack_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);
        auto cpt_opt = get_control_packet_type_with_check(fixed_header_);
        if (!cpt_opt || *cpt_opt != control_packet_type::connack) {
            throw make_error(
                errc::bad_message,
                "v5::connack_packet fixed_header is invalid"
            );
        }

        // remaining_length
        if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
            remaining_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v5::connack_packet remaining length is invalid");
        }
        if (remaining_length_ != buf.size()) {
            throw make_error(errc::bad_message, "v5::connack_packet remaining length doesn't match buf.size()");
        }

        // connect_acknowledge_flags
        if (buf.size() < 1) {
            throw make_error(
                errc::bad_message,
                "v5::connack_packet connect acknowledge flags don't exist"
            );
        }
        connect_acknowledge_flags_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);
        if ((connect_acknowledge_flags_ & 0b11111110)!= 0) {
            throw make_error(
                errc::bad_message,
                "v5::connack_packet connect acknowledge flags is invalid"
            );
        }
        // connect_reason_code
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
        case connect_reason_code::server_shutting_down:
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
            throw make_error(
                errc::bad_message,
                "v5::connack_packet connect reason_code is invalid"
            );
            break;
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
                    "v5::connack_packet properties_don't match its length"
                );
            }
            auto prop_buf = buf.substr(0, property_length_);
            props_ = make_properties(prop_buf, property_location::connack);
            buf.remove_prefix(property_length_);
        }
        else {
            throw make_error(
                errc::bad_message,
                "v5::connack_packet property_length is invalid"
            );
        }

        if (!buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::connack_packet properties don't match its length"
            );
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::connack;
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
        ret.emplace_back(as::buffer(&connect_acknowledge_flags_, 1));
        ret.emplace_back(as::buffer(&reason_code_, 1));

        ret.emplace_back(as::buffer(property_length_buf_.data(), property_length_buf_.size()));
        auto props_cbs = async_mqtt::const_buffer_sequence(props_);
        std::move(props_cbs.begin(), props_cbs.end(), std::back_inserter(ret));

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
            1 +                   // connect_acknowledge_flags
            1 +                   // reason_code
            async_mqtt::num_of_const_buffer_sequence(props_);
    }

    bool session_present() const {
        return is_session_present(static_cast<char>(connect_acknowledge_flags_));
    }

    /**
     * @breif Get reason code
     * @return reason_code
     */
    connect_reason_code code() const {
        return reason_code_;
    }

    /**
     * @breif Get properties
     * @return properties
     */
    properties const& props() const {
        return props_;
    }

private:
    std::uint8_t fixed_header_;

    std::size_t remaining_length_;
    boost::container::static_vector<char, 4> remaining_length_buf_;

    std::uint8_t connect_acknowledge_flags_;

    connect_reason_code reason_code_;

    std::size_t property_length_;
    boost::container::static_vector<char, 4> property_length_buf_;
    properties props_;
};

inline std::ostream& operator<<(std::ostream& o, connack_packet const& v) {
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

#endif // ASYNC_MQTT_PACKET_V5_CONNACK_HPP
