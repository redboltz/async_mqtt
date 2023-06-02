// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V3_1_1_PUBLISH_HPP)
#define ASYNC_MQTT_PACKET_V3_1_1_PUBLISH_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/variable_bytes.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/pubopts.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

/**
 * @brief MQTT PUBLISH packet (v3.1.1)
 * @tparam PacketIdBytes size of packet_id
 *
 * If both the client and the broker keeping the session, QoS1 and QoS2 PUBLISH packet is
 * stored in the endpoint for resending if disconnect/reconnect happens. In addition,
 * the client can sent the packet at offline. The packets are stored and will send after
 * the next connection is established.
 * If the session doesn' exist or lost, then the stored packets are erased.
 * \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718037
 */
template <std::size_t PacketIdBytes>
class basic_publish_packet {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    /**
     * @brief constructor
     * @tparam BufferSequence Type of the payload
     * @param packet_id  MQTT PacketIdentifier. If QoS0 then it must be 0. You can use no packet_id version constructor.
     *                   If QoS is 0 or 1 then, the packet_id must be acquired by
     *                   basic_endpoint::acquire_unique_packet_id(), or must be registered by
     *                   basic_endpoint::register_packet_id().
     *                   \n If QoS0, the packet_id is not sent actually.
     *                   \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349268
     * @param topic_name MQTT TopicName
     *                   \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349267
     * @param payloads   The body message of the packet. It could be a single buffer of multiple buffer sequence.
     *                   \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc384800413
     * @param pubopts    Publish Options. It contains the following elements:
     *                   \n DUP See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349262
     *                   \n QoS See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349263
     *                   \n RETAIN See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349265
     */
    template <
        typename BufferSequence,
        typename std::enable_if<
            is_buffer_sequence<std::decay_t<BufferSequence>>::value,
            std::nullptr_t
        >::type = nullptr
    >
    basic_publish_packet(
        packet_id_t packet_id,
        buffer topic_name,
        BufferSequence payloads,
        pub::opts pubopts
    )
        : fixed_header_(
              make_fixed_header(control_packet_type::publish, 0b0000) | std::uint8_t(pubopts)
          ),
          topic_name_{force_move(topic_name)},
          packet_id_(PacketIdBytes),
          remaining_length_(
              2                      // topic name length
              + topic_name_.size()   // topic name
              + (  (pubopts.get_qos() == qos::at_least_once || pubopts.get_qos() == qos::exactly_once)
                 ? PacketIdBytes // packet_id
                 : 0)
          )
    {
        topic_name_length_buf_.resize(topic_name_length_buf_.capacity());
        endian_store(
            boost::numeric_cast<std::uint16_t>(topic_name.size()),
            topic_name_length_buf_.data()
        );
        auto b = buffer_sequence_begin(payloads);
        auto e = buffer_sequence_end(payloads);
        auto num_of_payloads = static_cast<std::size_t>(std::distance(b, e));
        payloads_.reserve(num_of_payloads);
        for (; b != e; ++b) {
            auto const& payload = *b;
            remaining_length_ += payload.size();
            payloads_.push_back(payload);
        }
#if 0 // TBD
        utf8string_check(topic_name_);
#endif
        auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }
        switch (pubopts.get_qos()) {
        case qos::at_most_once:
            if (packet_id != 0) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::publish_packet qos0 but non 0 packet_id"
                );
            }
            endian_store(0, packet_id_.data());
            break;
        case qos::at_least_once:
        case qos::exactly_once:
            if (packet_id == 0) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::publish_packet qos not 0 but packet_id is 0"
                );
            }
            endian_store(packet_id, packet_id_.data());
            break;
        default:
            throw make_error(
                errc::bad_message,
                "v3_1_1::publish_packet qos is invalid"
            );
            break;
        }
    }

    /**
     * @brief constructor for QoS0
     * This constructor doesn't have packet_id parameter. The packet_id is set to 0 internally and not send actually.
     * @tparam BufferSequence Type of the payload
     * @param topic_name MQTT TopicName
     *                   \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349267
     * @param payloads   The body message of the packet. It could be a single buffer of multiple buffer sequence.
     *                   \n See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc384800413
     * @param pubopts    Publish Options. It contains the following elements:
     *                   \n DUP See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349262
     *                   \n QoS See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349263
     *                   \n RETAIN See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349265
     */
    template <
        typename BufferSequence,
        typename std::enable_if<
            is_buffer_sequence<std::decay_t<BufferSequence>>::value,
            std::nullptr_t
        >::type = nullptr
    >
    basic_publish_packet(
        buffer topic_name,
        BufferSequence payloads,
        pub::opts pubopts
    ) : basic_publish_packet(0, force_move(topic_name), force_move(payloads), pubopts) {
    }

    basic_publish_packet(buffer buf)
        : packet_id_(PacketIdBytes) {
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::publish_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        auto qos_value = pub::get_qos(fixed_header_);
        buf.remove_prefix(1);
        auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(fixed_header_));
        if (!cpt_opt || *cpt_opt != control_packet_type::publish) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::publish_packet fixed_header is invalid"
            );
        }

        // remaining_length
        if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
            remaining_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v3_1_1::publish_packet remaining length is invalid");
        }
        if (remaining_length_ != buf.size()) {
            throw make_error(errc::bad_message, "v3_1_1::publish_packet remaining length doesn't match buf.size()");
        }

        // topic_name_length
        if (!insert_advance(buf, topic_name_length_buf_)) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::publish_packet length of topic_name is invalid"
            );
        }
        auto topic_name_length = endian_load<std::uint16_t>(topic_name_length_buf_.data());

        // topic_name
        if (buf.size() < topic_name_length) {
            throw make_error(
                errc::bad_message,
                "v3_1_1::publish_packet topic_name doesn't match its length"
            );
        }
        topic_name_ = buf.substr(0, topic_name_length);
#if 0 // TBD
        utf8string_check(topic_name_);
#endif
        buf.remove_prefix(topic_name_length);

        // packet_id
        switch (qos_value) {
        case qos::at_most_once:
            endian_store(packet_id_t{0}, packet_id_.data());
            break;
        case qos::at_least_once:
        case qos::exactly_once:
            if (!copy_advance(buf, packet_id_)) {
                throw make_error(
                    errc::bad_message,
                    "v3_1_1::publish_packet packet_id doesn't exist"
                );
            }
            break;
        default:
            throw make_error(
                errc::bad_message,
                "v3_1_1::publish_packet qos is invalid"
            );
            break;
        };

        // payload
        if (!buf.empty()) {
            payloads_.emplace_back(force_move(buf));
        }
    }

    constexpr control_packet_type type() const {
        return control_packet_type::publish;
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
        ret.emplace_back(as::buffer(topic_name_length_buf_.data(), topic_name_length_buf_.size()));
        ret.emplace_back(as::buffer(topic_name_));
        if (packet_id() != 0) {
            ret.emplace_back(as::buffer(packet_id_.data(), packet_id_.size()));
        }
        for (auto const& payload : payloads_) {
            ret.emplace_back(as::buffer(payload));
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
            1U +                   // fixed header
            1U +                   // remaining length
            2U +                   // topic name length, topic name
            [&] {
                if (packet_id() == 0) return 0U;
                return 1U;
            }() +
            payloads_.size();

    }

    /**
     * @brief Get packet id
     * @return packet_id
     */
    packet_id_t packet_id() const {
        return endian_load<packet_id_t>(packet_id_.data());
    }

    /**
     * @brief Get publish_options
     * @return publish_options.
     */
    constexpr pub::opts opts() const {
        return pub::opts(fixed_header_);
    }

    /**
     * @brief Get topic name
     * @return topic name
     */
    constexpr buffer const& topic() const {
        return topic_name_;
    }

    /**
     * @brief Get payload
     * @return payload
     */
    std::vector<buffer> const& payload() const {
        return payloads_;
    }

    /**
     * @brief Get payload range
     * @return A pair of forward iterators
     */
    auto payload_range() const {
        return make_packet_range(payloads_);
    }

    /**
     * @brief Set dup flag
     * @param dup flag value to set
     */
    constexpr void set_dup(bool dup) {
        pub::set_dup(fixed_header_, dup);
    }

private:
    std::uint8_t fixed_header_;
    buffer topic_name_;
    static_vector<char, 2> topic_name_length_buf_;
    static_vector<char, PacketIdBytes> packet_id_;
    std::vector<buffer> payloads_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
};

template <std::size_t PacketIdBytes>
inline std::ostream& operator<<(std::ostream& o, basic_publish_packet<PacketIdBytes> const& v) {
    o << "v3_1_1::publish{" <<
        "topic:" << v.topic() << "," <<
        "qos:" << v.opts().get_qos() << "," <<
        "retain:" << v.opts().get_retain() << "," <<
        "dup:" << v.opts().get_dup();
    if (v.opts().get_qos() == qos::at_least_once ||
        v.opts().get_qos() == qos::exactly_once) {
        o << ",pid:" << v.packet_id();
    }
#if 0
    o << ",payload:";
    for (auto const& e : v.payload()) {
        o << json_like_out(e);
    }
#endif
    o << "}";
    return o;
}

/**
 * @related basic_publish_packet
 * @brief Type alias of basic_publish_packet (PacketIdBytes=2).
 */
using publish_packet = basic_publish_packet<2>;

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_V3_1_1_PUBLISH_HPP
