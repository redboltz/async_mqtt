// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNECT_HPP)
#define ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNECT_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/v3_1_1_connect.hpp>
#include <async_mqtt/packet/impl/packet_helper.hpp>
#include <async_mqtt/util/variable_bytes.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/utf8validate.hpp>

#include <async_mqtt/packet/detail/fixed_header.hpp>
#include <async_mqtt/packet/impl/copy_to_static_vector.hpp>

#include <async_mqtt/packet/impl/connect_flags.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

inline
connect_packet::connect_packet(
    bool clean_session,
    std::uint16_t keep_alive_sec,
    std::string client_id,
    std::optional<std::string> user_name,
    std::optional<std::string> password
):connect_packet(
    clean_session,
    keep_alive_sec,
    force_move(client_id),
    std::nullopt,
    force_move(user_name),
    force_move(password)
)
{}

inline
connect_packet::connect_packet(
    bool clean_session,
    std::uint16_t keep_alive_sec,
    std::string client_id,
    std::optional<will> w,
    std::optional<std::string> user_name,
    std::optional<std::string> password
)
    : fixed_header_{
          detail::make_fixed_header(control_packet_type::connect, 0b0000)
      },
      connect_flags_{0},
      // protocol name length, protocol name, protocol level, connect flag, client id length, client id, keep alive
      remaining_length_(
          2 +                     // protocol name length
          4 +                     // protocol name
          1 +                     // protocol level
          1 +                     // connect flag
          2 +                     // keep alive
          2 +                     // client id length
          client_id.size()        // client id
      ),
      protocol_name_and_level_{0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04},
      client_id_{force_move(client_id)},
      client_id_length_buf_(2),
      keep_alive_buf_(2)
{
    endian_store(keep_alive_sec, keep_alive_buf_.data());
    endian_store(boost::numeric_cast<std::uint16_t>(client_id_.size()), client_id_length_buf_.data());

    if (!utf8string_check(client_id_)) {
        throw system_error(
            make_error_code(
                connect_reason_code::client_identifier_not_valid
            )
        );
    }

    if (clean_session) connect_flags_ |= connect_flags::mask_clean_session;
    if (user_name) {
        if (!utf8string_check(*user_name)) {
            throw system_error(
                make_error_code(
                    connect_reason_code::bad_user_name_or_password
                )
            );
        }
        connect_flags_ |= connect_flags::mask_user_name_flag;
        user_name_ = buffer{force_move(*user_name)};
        user_name_length_buf_ = endian_static_vector(boost::numeric_cast<std::uint16_t>(user_name_.size()));
        remaining_length_ += 2 + user_name_.size();
    }
    if (password) {
        connect_flags_ |= connect_flags::mask_password_flag;
        password_ = buffer{force_move(*password)};
        password_length_buf_ = endian_static_vector(boost::numeric_cast<std::uint16_t>(password_.size()));
        remaining_length_ += 2 + password_.size();
    }
    if (w) {
        connect_flags_ |= connect_flags::mask_will_flag;
        if (w->get_retain() == pub::retain::yes) connect_flags_ |= connect_flags::mask_will_retain;
        connect_flags::set_will_qos(connect_flags_, w->get_qos());
        if (!utf8string_check(w->topic())) {
            throw system_error(
                make_error_code(
                    connect_reason_code::topic_name_invalid
                )
            );
        }
        will_topic_ = force_move(w->topic_as_buffer());
        will_topic_length_buf_ = endian_static_vector(boost::numeric_cast<std::uint16_t>(will_topic_.size()));
        if (w->message().size() > 0xffffL) {
            throw system_error(
                make_error_code(
                    connect_reason_code::malformed_packet
                )
            );
        }
        will_message_ = force_move(w->message_as_buffer());
        will_message_length_buf_ = endian_static_vector(boost::numeric_cast<std::uint16_t>(will_message_.size()));

        remaining_length_ += 2 + will_topic_.size() + 2 + will_message_.size();
    }

    auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
    for (auto e : rb) {
        remaining_length_buf_.push_back(e);
    }
}

inline
constexpr control_packet_type connect_packet::type() {
    return control_packet_type::connect;
}

inline
std::vector<as::const_buffer> connect_packet::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;
    ret.reserve(num_of_const_buffer_sequence());

    ret.emplace_back(as::buffer(&fixed_header_, 1));
    ret.emplace_back(as::buffer(remaining_length_buf_.data(), remaining_length_buf_.size()));
    ret.emplace_back(as::buffer(protocol_name_and_level_.data(), protocol_name_and_level_.size()));
    ret.emplace_back(as::buffer(&connect_flags_, 1));
    ret.emplace_back(as::buffer(keep_alive_buf_.data(), keep_alive_buf_.size()));

    ret.emplace_back(as::buffer(client_id_length_buf_.data(), client_id_length_buf_.size()));
    ret.emplace_back(as::buffer(client_id_));

    if (connect_flags::has_will_flag(connect_flags_)) {
        ret.emplace_back(as::buffer(will_topic_length_buf_.data(), will_topic_length_buf_.size()));
        ret.emplace_back(as::buffer(will_topic_));
        ret.emplace_back(as::buffer(will_message_length_buf_.data(), will_message_length_buf_.size()));
        ret.emplace_back(as::buffer(will_message_));
    }

    if (connect_flags::has_user_name_flag(connect_flags_)) {
        ret.emplace_back(as::buffer(user_name_length_buf_.data(), user_name_length_buf_.size()));
        ret.emplace_back(as::buffer(user_name_));
    }

    if (connect_flags::has_password_flag(connect_flags_)) {
        ret.emplace_back(as::buffer(password_length_buf_.data(), password_length_buf_.size()));
        ret.emplace_back(as::buffer(password_));
    }

    return ret;
}

inline
std::size_t connect_packet::size() const {
    return
        1 +                            // fixed header
        remaining_length_buf_.size() +
        remaining_length_;
}

inline
std::size_t connect_packet::num_of_const_buffer_sequence() const {
    return
        1 +                   // fixed header
        1 +                   // remaining length
        1 +                   // protocol name and level
        1 +                   // connect flags
        1 +                   // keep alive

        2 +                   // client id length, client id

        [&] () -> std::size_t {
            if (connect_flags::has_will_flag(connect_flags_)) {
                return
                    2 +       // will topic name length, will topic name
                    2;        // will message length, will message
            }
            return 0;
        } () +
        [&] () -> std::size_t {
            if (connect_flags::has_user_name_flag(connect_flags_)) {
                return 2;     // user name length, user name
            }
            return 0;
        } () +
        [&] () -> std::size_t {
            if (connect_flags::has_password_flag(connect_flags_)) {
                return 2;     // password length, password
            }
            return 0;
        } ();
}

inline
bool connect_packet::clean_session() const {
    return connect_flags::has_clean_session(connect_flags_);
}

inline
std::uint16_t connect_packet::keep_alive() const {
    return endian_load<std::uint16_t>(keep_alive_buf_.data());
}

inline
std::string connect_packet::client_id() const {
    return std::string{client_id_};
}

inline
std::optional<std::string> connect_packet::user_name() const {
    if (connect_flags::has_user_name_flag(connect_flags_)) {
        return std::string{user_name_};
    }
    else {
        return std::nullopt;
    }
}

inline
std::optional<std::string> connect_packet::password() const {
    if (connect_flags::has_password_flag(connect_flags_)) {
        return std::string{password_};
    }
    else {
        return std::nullopt;
    }
}

inline
std::optional<will> connect_packet::get_will() const {
    if (connect_flags::has_will_flag(connect_flags_)) {
        pub::opts opts =
            connect_flags::will_retain(connect_flags_) |
            connect_flags::will_qos(connect_flags_);
        return
            async_mqtt::will{
            will_topic_,
                will_message_,
                opts,
                properties{}
        };
    }
    else {
        return std::nullopt;
    }
}

inline
connect_packet::connect_packet(buffer buf, error_code& ec) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }
    fixed_header_ = static_cast<std::uint8_t>(buf.front());
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(fixed_header_);
    if (!cpt_opt || *cpt_opt != control_packet_type::connect) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }

    // remaining_length
    if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
        remaining_length_ = *vl_opt;
    }
    else {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }
    if (remaining_length_ != buf.size()) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }

    // protocol name and level
    if (!insert_advance(buf, protocol_name_and_level_)) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }
    static_vector<char, 7> expected_protocol_name_and_level {
        0, 4, 'M', 'Q', 'T', 'T', 4
    };
    if (protocol_name_and_level_ != expected_protocol_name_and_level) {
        ec = make_error_code(
            connect_reason_code::unsupported_protocol_version

        );
        return;
    }

    // connect_flags
    if (buf.size() < 1) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }
    connect_flags_ = buf.front();
    if (connect_flags_ & 0b00000001) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }
    buf.remove_prefix(1);

    // keep_alive
    if (!insert_advance(buf, keep_alive_buf_)) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }

    // client_id_length
    if (!insert_advance(buf, client_id_length_buf_)) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }
    auto client_id_length = endian_load<std::uint16_t>(client_id_length_buf_.data());

    // client_id
    if (buf.size() < client_id_length) {
        ec = make_error_code(
            connect_reason_code::malformed_packet
        );
        return;
    }
    client_id_ = buf.substr(0, client_id_length);
    if (!utf8string_check(client_id_)) {
        ec = make_error_code(
            connect_reason_code::client_identifier_not_valid
        );
        return;
    }
    buf.remove_prefix(client_id_length);

    // will
    if (connect_flags::has_will_flag(connect_flags_)) {
        auto will_qos = connect_flags::will_qos(connect_flags_);
        if (will_qos != qos::at_most_once &&
            will_qos != qos::at_least_once &&
            will_qos != qos::exactly_once) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        // will_topic_length
        if (!insert_advance(buf, will_topic_length_buf_)) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        auto will_topic_length = endian_load<std::uint16_t>(will_topic_length_buf_.data());

        // will_topic
        if (buf.size() < will_topic_length) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        will_topic_ = buf.substr(0, will_topic_length);
        if (!utf8string_check(will_topic_)) {
            ec = make_error_code(
                connect_reason_code::topic_name_invalid
            );
            return;
        }
        buf.remove_prefix(will_topic_length);

        // will_message_length
        if (!insert_advance(buf, will_message_length_buf_)) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        auto will_message_length = endian_load<std::uint16_t>(will_message_length_buf_.data());

        // will_message
        if (buf.size() < will_message_length) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        will_message_ = buf.substr(0, will_message_length);
        buf.remove_prefix(will_message_length);
    }
    else {
        auto will_retain = connect_flags::will_retain(connect_flags_);
        auto will_qos = connect_flags::will_qos(connect_flags_);
        if (will_retain == pub::retain::yes) {
            ec = make_error_code(
                connect_reason_code::protocol_error
            );
            return;
        }
        if (will_qos != qos::at_most_once) {
            ec = make_error_code(
                connect_reason_code::protocol_error
            );
            return;
        }
    }
    // user_name
    if (connect_flags::has_user_name_flag(connect_flags_)) {
        // user_name_topic_name_length
        if (!insert_advance(buf, user_name_length_buf_)) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        auto user_name_length = endian_load<std::uint16_t>(user_name_length_buf_.data());

        // user_name
        if (buf.size() < user_name_length) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        user_name_ = buf.substr(0, user_name_length);
        if (!utf8string_check(user_name_)) {
            ec = make_error_code(
                connect_reason_code::bad_user_name_or_password
            );
            return;
        }
        buf.remove_prefix(user_name_length);
    }

    // password
    if (connect_flags::has_password_flag(connect_flags_)) {
        // password_topic_name_length
        if (!insert_advance(buf, password_length_buf_)) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        auto password_length = endian_load<std::uint16_t>(password_length_buf_.data());

        // password
        if (buf.size() != password_length) {
            ec = make_error_code(
                connect_reason_code::malformed_packet
            );
            return;
        }
        password_ = buf.substr(0, password_length);
        buf.remove_prefix(password_length);
    }
}

inline
bool operator==(connect_packet const& lhs, connect_packet const& rhs) {
    return detail::equal(lhs, rhs);
}

inline
bool operator<(connect_packet const& lhs, connect_packet const& rhs) {
    return detail::less_than(lhs, rhs);
}

inline
std::ostream& operator<<(std::ostream& o, connect_packet const& v) {
    o <<
        "v3_1_1::connect{" <<
        "cid:" << v.client_id() << "," <<
        "ka:" << v.keep_alive() << "," <<
        "cs:" << v.clean_session();
    if (v.user_name()) {
        o << ",un:" << *v.user_name();
    }
    if (v.password()) {
        o << ",pw:" << "*****";
    }
    if (v.get_will()) {
        o << ",will:" << *v.get_will();
    }
    o << "}";
    return o;
}

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_IMPL_V3_1_1_CONNECT_HPP
