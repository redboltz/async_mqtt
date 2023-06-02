// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_CONNECT_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_CONNECT_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/variable_bytes.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>
#include <async_mqtt/packet/connect_flags.hpp>
#include <async_mqtt/packet/will.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @brief MQTT CONNECT packet (v3.1.1)
 *
 * Only MQTT client can send this packet.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718028
 */
class connect_packet {
public:
    /**
     * @bried constructor
     * @param clean_session  When the endpoint sends CONNECT packet with clean_session is true,
     *                       then stored packets are erased.
     *                       When the endpoint receives CONNECT packet with clean_session is false,
     *                       then the endpoint start storing PUBLISH packet (QoS1 and QoS2) and PUBREL packet
     *                       that would send by the endpoint until the corresponding response would be received.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349231
     * @param keep_alive_sec When the endpoint sends CONNECT packet with keep_alive_sec,
     *                       then the endpoint start sending PINGREQ packet keep_alive_sec after the last
     *                       packet is sent.
     *                       When the endpoint receives CONNECT packet with keep_alive_sec,
     *                       then start keep_alive_sec * 1.5 timer.
     *                       The timer is reset if any packet is received. If the timer is fired, then
     *                       the endpoint close the underlying layer automatically.
     *                       At that time, if the endpoint recv() is called, then the CompletionToken is
     *                       invoked with system_error.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349237
     * @param client_id      MQTT ClientIdentifier. It is the request to the broker for generating ClientIdentifier
     *                       if it is empty string and clean_session is true, If false then protocol error.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349242
     * @param user_name      MQTT UserName. It is often used for authentication.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349245
     * @param password       MQTT Password. It is often used for authentication.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349246
     */
    connect_packet(
        bool clean_session,
        std::uint16_t keep_alive_sec,
        buffer client_id,
        optional<buffer> user_name,
        optional<buffer> password
    ):connect_packet(
        clean_session,
        keep_alive_sec,
        force_move(client_id),
        nullopt,
        force_move(user_name),
        force_move(password)
    )
    {}
    /**
     * @bried constructor
     * @param clean_session  When the endpoint sends CONNECT packet with clean_session is true,
     *                       then stored packets are erased.
     *                       When the endpoint receives CONNECT packet with clean_session is false,
     *                       then the endpoint start storing PUBLISH packet (QoS1 and QoS2) and PUBREL packet
     *                       that would send by the endpoint until the corresponding response would be received.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349231
     * @param keep_alive_sec When the endpoint sends CONNECT packet with keep_alive_sec,
     *                       then the endpoint start sending PINGREQ packet keep_alive_sec after the last
     *                       packet is sent.
     *                       When the endpoint receives CONNECT packet with keep_alive_sec,
     *                       then start keep_alive_sec * 1.5 timer.
     *                       The timer is reset if any packet is received. If the timer is fired, then
     *                       the endpoint close the underlying layer automatically.
     *                       At that time, if the endpoint recv() is called, then the CompletionToken is
     *                       invoked with system_error.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349237
     * @param client_id      MQTT ClientIdentifier. It is the request to the broker for generating ClientIdentifier
     *                       if it is empty string and clean_session is true, If false then protocol error.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349242
     * @param will           MQTT Will
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349243
     * @param user_name      MQTT UserName. It is often used for authentication.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349245
     * @param password       MQTT Password. It is often used for authentication.
     *                       \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349246
     */
    connect_packet(
        bool clean_session,
        std::uint16_t keep_alive_sec,
        buffer client_id,
        optional<will> w,
        optional<buffer> user_name,
        optional<buffer> password
    )
        : fixed_header_{
              make_fixed_header(control_packet_type::connect, 0b0000)
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

#if 0 // TBD
        utf8string_check(client_id_);
#endif
        if (clean_session) connect_flags_ |= connect_flags::mask_clean_session;
        if (user_name) {
#if 0 // TBD
            utf8string_check(*user_name);
#endif
            connect_flags_ |= connect_flags::mask_user_name_flag;
            user_name_ = force_move(*user_name);
            user_name_length_buf_ = endian_static_vector(boost::numeric_cast<std::uint16_t>(user_name_.size()));
            remaining_length_ += 2 + user_name_.size();
        }
        if (password) {
            connect_flags_ |= connect_flags::mask_password_flag;
            password_ = force_move(*password);
            password_length_buf_ = endian_static_vector(boost::numeric_cast<std::uint16_t>(password_.size()));
            remaining_length_ += 2 + password_.size();
        }
        if (w) {
            connect_flags_ |= connect_flags::mask_will_flag;
            if (w->get_retain() == pub::retain::yes) connect_flags_ |= connect_flags::mask_will_retain;
            connect_flags::set_will_qos(connect_flags_, w->get_qos());

#if 0 // TBD
            utf8string_check(w->topic());
#endif
            will_topic_ = force_move(w->topic());
            will_topic_length_buf_ = endian_static_vector(boost::numeric_cast<std::uint16_t>(will_topic_.size()));
            if (w->message().size() > 0xffffL) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet will message too long"
                );
            }
            will_message_ = force_move(w->message());
            will_message_length_buf_ = endian_static_vector(boost::numeric_cast<std::uint16_t>(will_message_.size()));

            remaining_length_ += 2 + will_topic_.size() + 2 + will_message_.size();
        }

        auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }
    }

    connect_packet(buffer buf) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);
        auto cpt_opt = get_control_packet_type_with_check(fixed_header_);
        if (!cpt_opt || *cpt_opt != control_packet_type::connect) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet fixed_header is invalid"
            );
        }

        // remaining_length
        if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
            remaining_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v3_1_1::connect_packet remaining length is invalid");
        }
        if (remaining_length_ != buf.size()) {
            throw make_error(errc::bad_message, "v3_1_1::connect_packet remaining length doesn't match buf.size()");
        }

        // protocol name and level
        if (!insert_advance(buf, protocol_name_and_level_)) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet length of protocol_name or level is invalid"
            );
        }
        static_vector<char, 7> expected_protocol_name_and_level {
            0, 4, 'M', 'Q', 'T', 'T', 4
        };
        if (protocol_name_and_level_ != expected_protocol_name_and_level) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet contents of protocol_name or level is invalid"
            );
        }

        // connect_flags
        if (buf.size() < 1) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet connect_flags doesn't exist"
            );
        }
        connect_flags_ = buf.front();
        if (connect_flags_ & 0b00000001) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet connect_flags reserved bit0 is 1 (must be 0)"
            );
        }
        buf.remove_prefix(1);

        // keep_alive
        if (!insert_advance(buf, keep_alive_buf_)) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet keep_alive is invalid"
            );
        }

        // client_id_length
        if (!insert_advance(buf, client_id_length_buf_)) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet length of client_id is invalid"
            );
        }
        auto client_id_length = endian_load<std::uint16_t>(client_id_length_buf_.data());

        // client_id
        if (buf.size() < client_id_length) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::connect_packet client_id doesn't match its length"
            );
        }
        client_id_ = buf.substr(0, client_id_length);
#if 0 // TBD
        utf8string_check(client_id_);
#endif
        buf.remove_prefix(client_id_length);

        // will
        if (connect_flags::has_will_flag(connect_flags_)) {
            auto will_qos = connect_flags::will_qos(connect_flags_);
            if (will_qos != qos::at_most_once &&
                will_qos != qos::at_least_once &&
                will_qos != qos::exactly_once) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet will_qos is invalid"
                );
            }
            // will_topic_length
            if (!insert_advance(buf, will_topic_length_buf_)) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet length of will_topic is invalid"
                );
            }
            auto will_topic_length = endian_load<std::uint16_t>(will_topic_length_buf_.data());

            // will_topic
            if (buf.size() < will_topic_length) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet will_topic doesn't match its length"
                );
            }
            will_topic_ = buf.substr(0, will_topic_length);
#if 0 // TBD
            utf8string_check(will_topic_);
#endif
            buf.remove_prefix(will_topic_length);

            // will_message_length
            if (!insert_advance(buf, will_message_length_buf_)) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet length of will_message is invalid"
                );
            }
            auto will_message_length = endian_load<std::uint16_t>(will_message_length_buf_.data());

            // will_message
            if (buf.size() < will_message_length) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet will_message doesn't match its length"
                );
            }
            will_message_ = buf.substr(0, will_message_length);
            buf.remove_prefix(will_message_length);
        }
        else {
            auto will_retain = connect_flags::will_retain(connect_flags_);
            auto will_qos = connect_flags::will_qos(connect_flags_);
            if (will_retain == pub::retain::yes) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet combination of will_flag and will_retain is invalid"
                );
            }
            if (will_qos != qos::at_most_once) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet combination of will_flag and will_qos is invalid"
                );
            }
        }
        // user_name
        if (connect_flags::has_user_name_flag(connect_flags_)) {
            // user_name_topic_name_length
            if (!insert_advance(buf, user_name_length_buf_)) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet length of user_name is invalid"
                );
            }
            auto user_name_length = endian_load<std::uint16_t>(user_name_length_buf_.data());

            // user_name
            if (buf.size() < user_name_length) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet user_name doesn't match its length"
                );
            }
            user_name_ = buf.substr(0, user_name_length);
#if 0 // TBD
            utf8string_check(user_name_);
#endif
            buf.remove_prefix(user_name_length);
        }

        // password
        if (connect_flags::has_password_flag(connect_flags_)) {
            // password_topic_name_length
            if (!insert_advance(buf, password_length_buf_)) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet length of password is invalid"
                );
            }
            auto password_length = endian_load<std::uint16_t>(password_length_buf_.data());

            // password
            if (buf.size() != password_length) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::connect_packet password doesn't match its length"
                );
            }
            password_ = buf.substr(0, password_length);
            buf.remove_prefix(password_length);
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::connect;
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
    static constexpr std::size_t num_of_const_buffer_sequence() {
        return
            1 +                   // fixed header
            1 +                   // remaining length
            1 +                   // protocol name and level
            1 +                   // connect flags
            1 +                   // keep alive

            2 +                   // client id length, client id

            2 +                   // will topic name length, will topic name
            2 +                   // will message length, will message
            2 +                   // user name length, user name
            2;                    // password length, password
    }

    /**
     * @brief Get clean_session.
     * @return clean_session
     */
    bool clean_session() const {
        return connect_flags::has_clean_session(connect_flags_);
    }

    /**
     * @brief Get keep_alive.
     * @return keep_alive
     */
    std::uint16_t keep_alive() const {
        return endian_load<std::uint16_t>(keep_alive_buf_.data());
    }

    /**
     * @brief Get client_id
     * @return client_id
     */
    buffer client_id() const {
        return client_id_;
    }

    /**
     * @brief Get user_name.
     * @return user_name
     */
    optional<buffer> user_name() const {
        if (connect_flags::has_user_name_flag(connect_flags_)) {
            return user_name_;
        }
        else {
            return nullopt;
        }
    }

    /**
     * @brief Get password.
     * @return password
     */
    optional<buffer> password() const {
        if (connect_flags::has_password_flag(connect_flags_)) {
            return password_;
        }
        else {
            return nullopt;
        }
    }

    /**
     * @brief Get will.
     * @return will
     */
    optional<will> get_will() const {
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
            return nullopt;
        }
    }

private:
    std::uint8_t fixed_header_;
    char connect_flags_;

    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;

    static_vector<char, 7> protocol_name_and_level_;
    buffer client_id_;
    static_vector<char, 2> client_id_length_buf_;

    buffer will_topic_;
    static_vector<char, 2> will_topic_length_buf_;
    buffer will_message_;
    static_vector<char, 2> will_message_length_buf_;

    buffer user_name_;
    static_vector<char, 2> user_name_length_buf_;
    buffer password_;
    static_vector<char, 2> password_length_buf_;

    static_vector<char, 2> keep_alive_buf_;
};

inline std::ostream& operator<<(std::ostream& o, connect_packet const& v) {
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

#endif // ASYNC_MQTT_PACKET_V3_1_1_CONNECT_HPP
