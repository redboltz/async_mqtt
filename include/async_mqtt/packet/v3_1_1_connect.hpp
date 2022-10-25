// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_CONNECT_HPP)
#define ASYNC_MQTT_PACKET_CONNECT_HPP

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

class connect_packet {
public:
    template <
        typename BufferSequence,
        typename std::enable_if<
            is_buffer_sequence<BufferSequence>::value,
            std::nullptr_t
        >::type = nullptr
    >
    connect_packet(
        std::uint16_t keep_alive_sec,
        buffer client_id,
        bool clean_session,
        optional<will> w,
        optional<buffer> user_name,
        optional<buffer> password
    )
        : fixed_header_{
              static_cast<std::uint8_t>(make_fixed_header(control_packet_type::connect, 0b0000))
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
        if (clean_session) connect_flags_ |= connect_flags::clean_session;
        if (user_name) {
#if 0 // TBD
            utf8string_check(user_name.value());
#endif
            connect_flags_ |= connect_flags::user_name_flag;
            user_name_ = force_move(user_name.value());
            endian_store(
                boost::numeric_cast<std::uint16_t>(user_name_.size()),
                user_name_length_buf_.data()
            );
            remaining_length_ += 2 + user_name_.size();
        }
        if (password) {
            connect_flags_ |= connect_flags::password_flag;
            password_ = force_move(password.value());
            endian_store(
                boost::numeric_cast<std::uint16_t>(password_.size()),
                password_length_buf_.data()
            );
            remaining_length_ += 2 + password_.size();
        }
        if (w) {
            connect_flags_ |= connect_flags::will_flag;
            if (w.value().get_retain() == pub::retain::yes) connect_flags_ |= connect_flags::will_retain;
            connect_flags::set_will_qos(connect_flags_, w.value().get_qos());

#if 0 // TBD
            utf8string_check(w.value().topic());
#endif
            will_topic_name_ = force_move(w.value().topic());
            endian_store(
                boost::numeric_cast<std::uint16_t>(will_topic_name_.size()),
                will_topic_name_length_buf_.data()
            );
            if (w.value().message().size() > 0xffffL) throw will_message_length_error();
            will_message_ = force_move(w.value().message());
            endian_store(
                boost::numeric_cast<std::uint16_t>(will_message_.size()),
                will_message_length_buf_.data()
            );

            remaining_length_ += 2 + will_topic_name_.size() + 2 + will_message_.size();
        }

        auto rb = val_to_variable_bytes(remaining_length_);
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }
    }

    connect_packet(buffer buf) {
        // fixed_header
        if (buf.empty())  throw remaining_length_error();
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);

        // remaining_length
        remaining_length_ = copy_advance_variable_length(buf, remaining_length_buf_);

        // protocol name and level
        copy_advance(buf, protocol_name_and_level_);
        static_vector<char, 7> expected_protocol_name_and_level {
            0, 4, 'M', 'Q', 'T', 'T', 4
        };
        if (protocol_name_and_level_ != expected_protocol_name_and_level) {
            throw protocol_error();
        }

        // connect_flags
        if (buf.size() < 1) throw remaining_length_error();
        connect_flags_ = buf.front();
        if (connect_flags_ & 0b00000001) {
            throw protocol_error();
        }
        buf.remove_prefix(1);

        // keep_alive
        copy_advance(buf, keep_alive_buf_);

        // client_id_length
        copy_advance(buf, client_id_length_buf_);
        auto client_id_length = endian_load<std::uint16_t>(client_id_length_buf_.data());
        buf.remove_prefix(2);

        // client_id
        if (buf.size() < client_id_length) throw remaining_length_error();
        client_id_ = buf.substr(0, client_id_length);
#if 0 // TBD
        utf8string_check(client_id_);
#endif
        buf.remove_prefix(client_id_length);

        // will
        if (connect_flags::has_will_flag(connect_flags_)) {
            // will_topic_name_length
            copy_advance(buf, will_topic_name_length_buf_);
            auto will_topic_name_length = endian_load<std::uint16_t>(will_topic_name_length_buf_.data());

            // will_topic_name
            if (buf.size() < will_topic_name_length) throw remaining_length_error();
            will_topic_name_ = buf.substr(0, will_topic_name_length);
#if 0 // TBD
            utf8string_check(will_topic_name_);
#endif
            buf.remove_prefix(will_topic_name_length);

            // will_message_length
            copy_advance(buf, will_message_length_buf_);
            auto will_message_length = endian_load<std::uint16_t>(will_message_length_buf_.data());

            // will_message
            if (buf.size() < will_message_length) throw remaining_length_error();
            will_message_ = buf.substr(0, will_message_length);
            buf.remove_prefix(will_message_length);
        }

        // user_name
        if (connect_flags::has_user_name_flag(connect_flags_)) {
            // user_name_topic_name_length
            copy_advance(buf, user_name_length_buf_);
            auto user_name_length = endian_load<std::uint16_t>(user_name_length_buf_.data());

            // user_name
            if (buf.size() < user_name_length) throw remaining_length_error();
            user_name_ = buf.substr(0, user_name_length);
#if 0 // TBD
            utf8string_check(user_name_);
#endif
            buf.remove_prefix(user_name_length);
        }

        // password
        if (connect_flags::has_password_flag(connect_flags_)) {
            // password_topic_name_length
            copy_advance(buf, password_length_buf_);
            auto password_length = endian_load<std::uint16_t>(password_length_buf_.data());

            // password
            if (buf.size() < password_length) throw remaining_length_error();
            password_ = buf.substr(0, password_length);
            buf.remove_prefix(password_length);
        }
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
            ret.emplace_back(as::buffer(will_topic_name_length_buf_.data(), will_topic_name_length_buf_.size()));
            ret.emplace_back(as::buffer(will_topic_name_));
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
     * @brief Get whole size of sequence
     * @return whole size
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
     * @brief Create one continuours buffer.
     *        All sequence of buffers are concatinated.
     *        It is useful to store to file/database.
     * @return continuous buffer
     */
    std::string continuous_buffer() const {
        std::string ret;

        ret.reserve(size());

        ret.push_back(static_cast<char>(fixed_header_));
        ret.append(remaining_length_buf_.data(), remaining_length_buf_.size());
        ret.append(protocol_name_and_level_.data(), protocol_name_and_level_.size());
        ret.push_back(connect_flags_);
        ret.append(keep_alive_buf_.data(), keep_alive_buf_.size());

        ret.append(client_id_length_buf_.data(), client_id_length_buf_.size());
        ret.append(client_id_.data(), client_id_.size());

        if (connect_flags::has_will_flag(connect_flags_)) {
            ret.append(will_topic_name_length_buf_.data(), will_topic_name_length_buf_.size());
            ret.append(will_topic_name_.data(), will_topic_name_.size());
            ret.append(will_message_length_buf_.data(), will_message_length_buf_.size());
            ret.append(will_message_.data(), will_message_.size());
        }

        if (connect_flags::has_user_name_flag(connect_flags_)) {
            ret.append(user_name_length_buf_.data(), user_name_length_buf_.size());
            ret.append(user_name_.data(), user_name_.size());
        }

        if (connect_flags::has_password_flag(connect_flags_)) {
            ret.append(password_length_buf_.data(), password_length_buf_.size());
            ret.append(password_.data(), password_.size());
        }

        return ret;
    }

private:
    std::uint8_t fixed_header_;
    char connect_flags_;

    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;

    static_vector<char, 7> protocol_name_and_level_;
    buffer client_id_;
    static_vector<char, 2> client_id_length_buf_;

    buffer will_topic_name_;
    static_vector<char, 2> will_topic_name_length_buf_;
    buffer will_message_;
    static_vector<char, 2> will_message_length_buf_;

    buffer user_name_;
    static_vector<char, 2> user_name_length_buf_;
    buffer password_;
    static_vector<char, 2> password_length_buf_;

    static_vector<char, 2> keep_alive_buf_;
};

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_HPP
