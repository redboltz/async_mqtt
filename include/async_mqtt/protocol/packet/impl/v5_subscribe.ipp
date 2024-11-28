// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V5_SUBSCRIBE_IPP)
#define ASYNC_MQTT_PACKET_IMPL_V5_SUBSCRIBE_IPP

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/v5_subscribe.hpp>
#include <async_mqtt/packet/impl/packet_helper.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/inline.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/utf8validate.hpp>

#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/detail/fixed_header.hpp>
#include <async_mqtt/packet/topic_subopts.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/impl/copy_to_static_vector.hpp>
#include <async_mqtt/packet/impl/validate_property.hpp>

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/detail/instantiate_helper.hpp>
#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

namespace async_mqtt::v5 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_subscribe_packet<PacketIdBytes>::basic_subscribe_packet(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    std::vector<topic_subopts> params,
    properties props
)
    : fixed_header_{detail::make_fixed_header(control_packet_type::subscribe, 0b0010)},
      entries_{force_move(params)},
      remaining_length_{PacketIdBytes},
      property_length_(async_mqtt::size(props)),
      props_(force_move(props))
{
    using namespace std::literals;
    topic_length_buf_entries_.reserve(entries_.size());
    for (auto const& e : entries_) {
        if (e.all_topic().size() > 0xffff) {
            throw system_error{
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            };
        }
        topic_length_buf_entries_.push_back(
            endian_static_vector(
                static_cast<std::uint16_t>(e.all_topic().size())
            )
        );
    }

    endian_store(packet_id, packet_id_.data());

    auto pb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(property_length_));
    for (auto e : pb) {
        property_length_buf_.push_back(e);
    }

    for (auto const& prop : props_) {
        auto id = prop.id();
        if (!validate_property(property_location::subscribe, id)) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            );
        }
    }

    remaining_length_ += property_length_buf_.size() + property_length_;

    for (auto const& e : entries_) {
        // reserved bits check
        if (static_cast<std::uint8_t>(e.opts()) & 0b11000000) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            );
        }
        switch (e.opts().get_qos()) {
        case qos::at_most_once:
        case qos::at_least_once:
        case qos::exactly_once:
            break;
        default:
            throw system_error{
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            };
            break;
        }
        switch (e.opts().get_retain_handling()) {
        case sub::retain_handling::send:
        case sub::retain_handling::send_only_new_subscription:
        case sub::retain_handling::not_send:
            break;
        default:
            throw system_error{
                make_error_code(
                    disconnect_reason_code::malformed_packet
                )
            };
            break;
        }
        if (e.opts().get_nl() == sub::nl::yes && !e.sharename().empty()) {
            throw system_error{
                make_error_code(
                    disconnect_reason_code::protocol_error
                )
            };
        }
        auto size = e.all_topic().size();
        remaining_length_ +=
            2 +                     // topic name length
            size +                  // topic filter
            1;                      // opts

        if (!utf8string_check(e.all_topic())) {
            throw system_error{
                make_error_code(
                    disconnect_reason_code::topic_filter_invalid
                )
            };
        }
    }

    remaining_length_buf_ = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<as::const_buffer> basic_subscribe_packet<PacketIdBytes>::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;
    ret.reserve(num_of_const_buffer_sequence());

    ret.emplace_back(as::buffer(&fixed_header_, 1));

    ret.emplace_back(as::buffer(remaining_length_buf_.data(), remaining_length_buf_.size()));

    ret.emplace_back(as::buffer(packet_id_.data(), packet_id_.size()));

    ret.emplace_back(as::buffer(property_length_buf_.data(), property_length_buf_.size()));
    auto props_cbs = async_mqtt::const_buffer_sequence(props_);
    std::move(props_cbs.begin(), props_cbs.end(), std::back_inserter(ret));

        BOOST_ASSERT(entries_.size() == topic_length_buf_entries_.size());
    auto it = topic_length_buf_entries_.begin();
    for (auto const& e : entries_) {
        ret.emplace_back(as::buffer(it->data(), it->size()));
        ret.emplace_back(as::buffer(e.all_topic()));
        ret.emplace_back(as::buffer(&e.opts(), 1));
        ++it;
    }

    return ret;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t basic_subscribe_packet<PacketIdBytes>::size() const {
    return
        1 +                            // fixed header
        remaining_length_buf_.size() +
        remaining_length_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::size_t basic_subscribe_packet<PacketIdBytes>::num_of_const_buffer_sequence() const {
    return
        1 +                   // fixed header
        1 +                   // remaining length
        1 +                   // packet id
        [&] () -> std::size_t {
            if (property_length_buf_.size() == 0) return 0;
            return
                1 +                   // property length
                async_mqtt::num_of_const_buffer_sequence(props_);
        }() +
        entries_.size() * 3;  // topic name length, topic name, opts
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
typename basic_packet_id_type<PacketIdBytes>::type basic_subscribe_packet<PacketIdBytes>::packet_id() const {
    return endian_load<typename basic_packet_id_type<PacketIdBytes>::type>(packet_id_.data());
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<topic_subopts> const& basic_subscribe_packet<PacketIdBytes>::entries() const {
    return entries_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
properties const& basic_subscribe_packet<PacketIdBytes>::props() const {
    return props_;
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_subscribe_packet<PacketIdBytes>::basic_subscribe_packet(buffer buf, error_code& ec) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    fixed_header_ = static_cast<std::uint8_t>(buf.front());
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(fixed_header_));
    if (!cpt_opt || *cpt_opt != control_packet_type::subscribe) {
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

    // packet_id
    if (!copy_advance(buf, packet_id_)) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
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
        props_ = make_properties(prop_buf, property_location::subscribe, ec);
        if (ec) return;
        buf.remove_prefix(property_length_);
    }
    else {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::protocol_error // no entry
        );
        return;
    }
    while (!buf.empty()) {
        // topic_length
        static_vector<char, 2> topic_length_buf;
        if (!insert_advance(buf, topic_length_buf)) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        auto topic_length = endian_load<std::uint16_t>(topic_length_buf.data());
        topic_length_buf_entries_.push_back(topic_length_buf);

        // topic
        if (buf.size() < topic_length) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        auto topic = buf.substr(0, topic_length);

        if (!utf8string_check(topic)) {
            ec = make_error_code(
                disconnect_reason_code::topic_filter_invalid
            );
            return;
        }

        buf.remove_prefix(topic_length);

        // opts
        if (buf.empty()) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        auto opts = static_cast<sub::opts>(std::uint8_t(buf.front()));
        // reserved bits check
        if (static_cast<std::uint8_t>(opts) & 0b11000000) {
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        switch (opts.get_qos()) {
        case qos::at_most_once:
        case qos::at_least_once:
        case qos::exactly_once:
            break;
        default:
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        switch (opts.get_retain_handling()) {
        case sub::retain_handling::send:
        case sub::retain_handling::send_only_new_subscription:
        case sub::retain_handling::not_send:
            break;
        default:
            ec = make_error_code(
                disconnect_reason_code::malformed_packet
            );
            return;
        }
        auto entry = topic_subopts{std::string{topic}, opts};
        if (entry.opts().get_nl() == sub::nl::yes && !entry.sharename().empty()) {
            ec = make_error_code(
                disconnect_reason_code::protocol_error
            );
            return;
        }

        entries_.push_back(force_move(entry));
        buf.remove_prefix(1);
    }
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator==(basic_subscribe_packet<PacketIdBytes> const& lhs, basic_subscribe_packet<PacketIdBytes> const& rhs) {
    return detail::equal(lhs, rhs);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
bool operator<(basic_subscribe_packet<PacketIdBytes> const& lhs, basic_subscribe_packet<PacketIdBytes> const& rhs) {
    return detail::less_than(lhs, rhs);
}

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::ostream& operator<<(std::ostream& o, basic_subscribe_packet<PacketIdBytes> const& v) {
    o <<
        "v5::subscribe{" <<
        "pid:" << v.packet_id() << ",[";
    auto b = v.entries().cbegin();
    auto e = v.entries().cend();
    if (b != e) {
        o <<
            "{"
            "topic:" << b->topic() << "," <<
            "sn:" << b->sharename() << "," <<
            "qos:" << b->opts().get_qos() << "," <<
            "rh:" << b->opts().get_retain_handling() << "," <<
            "nl:" << b->opts().get_nl() << "," <<
            "rap:" << b->opts().get_rap() <<
            "}";
        ++b;
    }
    for (; b != e; ++b) {
        o << "," <<
            "{"
            "topic:" << b->topic() << "," <<
            "sn:" << b->sharename() << "," <<
            "qos:" << b->opts().get_qos() << "," <<
            "rh:" << b->opts().get_retain_handling() << "," <<
            "nl:" << b->opts().get_nl() << "," <<
            "rap:" << b->opts().get_rap() <<
            "}";
    }
    o << "]";
    if (!v.props().empty()) {
        o << ",ps:" << v.props();
    };
    o << "}";
    return o;
}

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#define ASYNC_MQTT_INSTANTIATE_EACH(a_size) \
template class basic_subscribe_packet<a_size>; \
template bool operator==(basic_subscribe_packet<a_size> const& lhs, basic_subscribe_packet<a_size> const& rhs); \
template bool operator<(basic_subscribe_packet<a_size> const& lhs, basic_subscribe_packet<a_size> const& rhs); \
template std::ostream& operator<<(std::ostream& o, basic_subscribe_packet<a_size> const& v);

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

#endif // ASYNC_MQTT_PACKET_IMPL_V5_SUBSCRIBE_IPP
