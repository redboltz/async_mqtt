// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V3_1_1_PUBLISH_IPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V3_1_1_PUBLISH_IPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/util/inline.hpp>
#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/variable_bytes.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/utf8validate.hpp>
#include <async_mqtt/protocol/packet/v3_1_1_publish.hpp>
#include <async_mqtt/protocol/packet/impl/packet_helper.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>
#include <async_mqtt/protocol/packet/pubopts.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/detail/fixed_header.hpp>

#include <async_mqtt/protocol/packet/impl/copy_to_static_vector.hpp>

#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
#include <async_mqtt/util/json_like_out.hpp>
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/detail/instantiate_helper.hpp>
#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_publish_packet<PacketIdBytes>::basic_publish_packet(
    tag_internal,
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    buffer&& topic_name,
    std::vector<buffer>&& payloads,
    pub::opts pubopts
)
    : fixed_header_(
          detail::make_fixed_header(control_packet_type::publish, 0b0000) | std::uint8_t(pubopts)
      ),
      topic_name_{force_move(topic_name)},
      packet_id_(PacketIdBytes),
      payloads_{force_move(payloads)},
      remaining_length_(
          2                      // topic name length
          + topic_name_.size()   // topic name
          + (  (pubopts.get_qos() == qos::at_least_once || pubopts.get_qos() == qos::exactly_once)
               ? PacketIdBytes // packet_id
               : 0)
      )
{
    if (topic_name_.size() > 0xffff) {
        throw system_error{
            make_error_code(
                disconnect_reason_code::malformed_packet
            )
        };
    }

    topic_name_length_buf_.resize(topic_name_length_buf_.capacity());
    endian_store(
        boost::numeric_cast<std::uint16_t>(topic_name_.size()),
        topic_name_length_buf_.data()
    );

    for (auto const& payload : payloads_) {
        remaining_length_ += payload.size();
    }

    if (!utf8string_check(topic_name_)) {
        throw system_error(
            make_error_code(
                disconnect_reason_code::malformed_packet
            )
        );
    }

    auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
    for (auto e : rb) {
        remaining_length_buf_.push_back(e);
    }
    switch (pubopts.get_qos()) {
    case qos::at_most_once:
        if (packet_id != 0) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            );
        }
        endian_store(0, packet_id_.data());
        break;
    case qos::at_least_once:
    case qos::exactly_once:
        if (packet_id == 0) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            );
        }
        endian_store(packet_id, packet_id_.data());
        break;
    default:
        throw system_error(
            make_error_code(
                disconnect_reason_code::malformed_packet
            )
        );
        break;
    }
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> basic_publish_packet<PacketIdBytes>::const_buffer_sequence() const {
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

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t basic_publish_packet<PacketIdBytes>::size() const {
    return
        1 +                            // fixed header
        remaining_length_buf_.size() +
        remaining_length_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t basic_publish_packet<PacketIdBytes>::num_of_const_buffer_sequence() const {
    return
        1 +                   // fixed header
        1 +                   // remaining length
        2 +                   // topic name length, topic name
        [&] () -> std::size_t {
            if (packet_id() == 0) return 0;
            return 1;
        }() +
        payloads_.size();

}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
typename basic_packet_id_type<PacketIdBytes>::type basic_publish_packet<PacketIdBytes>::packet_id() const {
    return endian_load<typename basic_packet_id_type<PacketIdBytes>::type>(packet_id_.data());
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::string basic_publish_packet<PacketIdBytes>::topic() const {
    return std::string{topic_name_};
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
buffer const& basic_publish_packet<PacketIdBytes>::topic_as_buffer() const {
    return topic_name_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::string basic_publish_packet<PacketIdBytes>::payload() const {
    return to_string(payloads_);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
auto basic_publish_packet<PacketIdBytes>::payload_range() const {
    return make_packet_range(payloads_);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<buffer> const& basic_publish_packet<PacketIdBytes>::payload_as_buffer() const {
    return payloads_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_publish_packet<PacketIdBytes>::basic_publish_packet(buffer buf, error_code& ec)
    : packet_id_(PacketIdBytes) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    fixed_header_ = static_cast<std::uint8_t>(buf.front());
    auto qos_value = pub::get_qos(fixed_header_);
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(fixed_header_));
    if (!cpt_opt || *cpt_opt != control_packet_type::publish) {
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

    // topic_name_length
    if (!insert_advance(buf, topic_name_length_buf_)) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    auto topic_name_length = endian_load<std::uint16_t>(topic_name_length_buf_.data());

    // topic_name
    if (buf.size() < topic_name_length) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    topic_name_ = buf.substr(0, topic_name_length);

    if (!utf8string_check(topic_name_)) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    buf.remove_prefix(topic_name_length);

    // packet_id
    switch (qos_value) {
    case qos::at_most_once:
        endian_store(typename basic_packet_id_type<PacketIdBytes>::type{0}, packet_id_.data());
        break;
    case qos::at_least_once:
    case qos::exactly_once:
        if (!copy_advance(buf, packet_id_)) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        break;
    default:
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    };

    // payload
    if (!buf.empty()) {
        payloads_.emplace_back(force_move(buf));
    }
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator==(basic_publish_packet<PacketIdBytes> const& lhs, basic_publish_packet<PacketIdBytes> const& rhs) {
    return detail::equal(lhs, rhs);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator<(basic_publish_packet<PacketIdBytes> const& lhs, basic_publish_packet<PacketIdBytes> const& rhs) {
    return detail::less_than(lhs, rhs);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, basic_publish_packet<PacketIdBytes> const& v) {
    o << "v3_1_1::publish{" <<
        "topic:" << v.topic() << "," <<
        "qos:" << v.opts().get_qos() << "," <<
        "retain:" << v.opts().get_retain() << "," <<
        "dup:" << v.opts().get_dup();
    if (v.opts().get_qos() == qos::at_least_once ||
        v.opts().get_qos() == qos::exactly_once) {
        o << ",pid:" << v.packet_id();
    }
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    o << ",payload:";
    for (auto const& e : v.payload_as_buffer()) {
        o << json_like_out(e);
    }
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    o << "}";
    return o;
}

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#define ASYNC_MQTT_INSTANTIATE_EACH(a_size) \
template class basic_publish_packet<a_size>; \
template bool operator==(basic_publish_packet<a_size> const& lhs, basic_publish_packet<a_size> const& rhs); \
template bool operator<(basic_publish_packet<a_size> const& lhs, basic_publish_packet<a_size> const& rhs); \
template std::ostream& operator<<(std::ostream& o, basic_publish_packet<a_size> const& v);

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_SIZE))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V3_1_1_PUBLISH_IPP
