// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V5_PUBLISH_IPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V5_PUBLISH_IPP

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
#include <async_mqtt/protocol/packet/v5_publish.hpp>
#include <async_mqtt/protocol/packet/impl/packet_helper.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>
#include <async_mqtt/protocol/packet/detail/fixed_header.hpp>
#include <async_mqtt/protocol/packet/pubopts.hpp>
#include <async_mqtt/protocol/packet/property_variant.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>

#include <async_mqtt/protocol/packet/impl/copy_to_static_vector.hpp>
#include <async_mqtt/protocol/packet/impl/validate_property.hpp>

#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
#include <async_mqtt/util/json_like_out.hpp>
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/asio_bind/detail/instantiate_helper.hpp>
#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

namespace async_mqtt::v5 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_publish_packet<PacketIdBytes>::basic_publish_packet(
    tag_internal,
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    buffer&& topic_name,
    std::vector<buffer>&& payloads,
    pub::opts pubopts,
    properties props
)
    : fixed_header_(
        detail::make_fixed_header(control_packet_type::publish, 0b0000) | std::uint8_t(pubopts)
    ),
      topic_name_{force_move(topic_name)},
      packet_id_(PacketIdBytes),
      property_length_(async_mqtt::size(props)),
      props_(force_move(props)),
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

    auto pb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(property_length_));
    for (auto e : pb) {
        property_length_buf_.push_back(e);
    }

    for (auto const& prop : props_) {
        auto id = prop.id();
        if (!validate_property(property_location::publish, id)) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            );
        }
    }

    remaining_length_ += property_length_buf_.size() + property_length_;

    auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
    for (auto e : rb) {
        remaining_length_buf_.push_back(e);
    }

    if (pubopts.get_qos() == qos::at_least_once ||
        pubopts.get_qos() == qos::exactly_once) {
        if (packet_id == 0) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::protocol_error
                )
            );
        }
        endian_store(packet_id, packet_id_.data());
    }
    else {
        if (packet_id != 0) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::protocol_error
                )
            );
        }
        endian_store(typename basic_packet_id_type<PacketIdBytes>::type{0}, packet_id_.data());
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
    ret.emplace_back(as::buffer(property_length_buf_.data(), property_length_buf_.size()));
    auto props_cbs = async_mqtt::const_buffer_sequence(props_);
    std::move(props_cbs.begin(), props_cbs.end(), std::back_inserter(ret));
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
        1U +                   // fixed header
        1U +                   // remaining length
        2U +                   // topic name length, topic name
        [&] {
            if (packet_id() == 0) return 0U;
            return 1U;
        }() +
        1U +                   // property length
        async_mqtt::num_of_const_buffer_sequence(props_) +
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
properties const& basic_publish_packet<PacketIdBytes>::props() const {
    return props_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void basic_publish_packet<PacketIdBytes>::remove_topic_add_topic_alias(topic_alias_type val) {
    // add topic_alias property
    auto prop{property::topic_alias{val}};
    auto prop_size = prop.size();
    property_length_ += prop_size;
    props_.push_back(force_move(prop));

    // update property_length_buf
    auto [old_property_length_buf_size, new_property_length_buf_size] =
        update_property_length_buf();

    // remove topic_name
    auto old_topic_name_size = topic_name_.size();
    topic_name_ = buffer{};
    endian_store(
        boost::numeric_cast<std::uint16_t>(topic_name_.size()),
        topic_name_length_buf_.data()
    );

    // update remaining_length
    remaining_length_ +=
        prop_size +
        (new_property_length_buf_size - old_property_length_buf_size) -
        old_topic_name_size;
    update_remaining_length_buf();
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void basic_publish_packet<PacketIdBytes>::add_topic_alias(topic_alias_type val) {
    // add topic_alias property
    auto prop{property::topic_alias{val}};
    auto prop_size = prop.size();
    property_length_ += prop_size;
    props_.push_back(force_move(prop));

    // update property_length_buf
    auto [old_property_length_buf_size, new_property_length_buf_size] =
        update_property_length_buf();

    // update remaining_length
    remaining_length_ +=
        prop_size +
        (new_property_length_buf_size - old_property_length_buf_size);
    update_remaining_length_buf();
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void basic_publish_packet<PacketIdBytes>::add_topic(std::string topic) {
    add_topic_impl(force_move(topic));
    // update remaining_length
    remaining_length_ += topic_name_.size();
    update_remaining_length_buf();
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void basic_publish_packet<PacketIdBytes>::remove_topic_alias() {
    auto prop_size = remove_topic_alias_impl();
    property_length_ -= prop_size;
    // update property_length_buf
    auto [old_property_length_buf_size, new_property_length_buf_size] =
        update_property_length_buf();

    // update remaining_length
    remaining_length_ +=
        -prop_size +
        (new_property_length_buf_size - old_property_length_buf_size);
    update_remaining_length_buf();
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void basic_publish_packet<PacketIdBytes>::remove_topic_alias_add_topic(std::string topic) {
    auto prop_size = remove_topic_alias_impl();
    property_length_ -= prop_size;
    add_topic_impl(force_move(topic));
    // update property_length_buf
    auto [old_property_length_buf_size, new_property_length_buf_size] =
        update_property_length_buf();

    // update remaining_length
    remaining_length_ +=
        topic_name_.size() -
        prop_size +
        (new_property_length_buf_size - old_property_length_buf_size);
    update_remaining_length_buf();
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void basic_publish_packet<PacketIdBytes>::update_message_expiry_interval(std::uint32_t val) {
    bool updated = false;
    for (auto& prop : props_) {
        prop.visit(
            overload {
                [&](property::message_expiry_interval& p) {
                    p = property::message_expiry_interval(val);
                    updated = true;
                },
                [&](auto&){}
                }
        );
        if (updated) return;
    }
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void basic_publish_packet<PacketIdBytes>::update_remaining_length_buf() {
    remaining_length_buf_.clear();
    auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
    for (auto e : rb) {
        remaining_length_buf_.push_back(e);
    }
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::tuple<std::size_t, std::size_t> basic_publish_packet<PacketIdBytes>::update_property_length_buf() {
    auto old_property_length_buf_size = property_length_buf_.size();
    property_length_buf_.clear();
    auto pb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(property_length_));
    for (auto e : pb) {
        property_length_buf_.push_back(e);
    }
    auto new_property_length_buf_size = property_length_buf_.size();
    return {old_property_length_buf_size, new_property_length_buf_size};
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t basic_publish_packet<PacketIdBytes>::remove_topic_alias_impl() {
    auto it = props_.cbegin();
    std::size_t size = 0;
    while (it != props_.cend()) {
        if (it->id() == property::id::topic_alias) {
            size += it->size();
            it = props_.erase(it);
        }
        else {
            ++it;
        }
    }
    return size;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void basic_publish_packet<PacketIdBytes>::add_topic_impl(std::string topic) {
    BOOST_ASSERT(topic_name_.empty());

    // add topic
    topic_name_ = buffer{force_move(topic)};
    endian_store(
        boost::numeric_cast<std::uint16_t>(topic_name_.size()),
        topic_name_length_buf_.data()
    );
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
    switch (pub::get_qos(fixed_header_)) {
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
        props_ = make_properties(prop_buf, property_location::publish, ec);
        if (ec) return;
        buf.remove_prefix(property_length_);
    }
    else {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

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
    o << "v5::publish{" <<
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
    if (!v.props().empty()) {
        o << ",ps:" << v.props();
    };
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

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PROTOCOL_PACKET_IMPL_V5_PUBLISH_IPP
