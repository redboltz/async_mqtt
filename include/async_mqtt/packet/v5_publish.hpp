// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PUBLISH_HPP)
#define ASYNC_MQTT_PACKET_V5_PUBLISH_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/variable_bytes.hpp>
#include <async_mqtt/type.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/pubopts.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>
#include <async_mqtt/packet/property_variant.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
class basic_publish_packet {
public:
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    ~basic_publish_packet() {
        std::cout << "D:" << this << std::endl;
    }


    template <
        typename BufferSequence,
        std::enable_if_t<
            is_buffer_sequence<std::decay_t<BufferSequence>>::value,
            std::nullptr_t
        > = nullptr
    >
    basic_publish_packet(
        packet_id_t packet_id,
        buffer topic_name,
        BufferSequence payloads,
        pub::opts pubopts,
        properties props
    )
        : fixed_header_(
              make_fixed_header(control_packet_type::publish, 0b0000) | std::uint8_t(pubopts)
          ),
          topic_name_{force_move(topic_name)},
          packet_id_(PacketIdBytes),
          property_length_(async_mqtt::size(props)),
          props_(force_move(props)),
          remaining_length_(
              2                      // topic name length
              + topic_name_.size()   // topic name
              + (  (pubopts.get_qos() == qos::at_least_once || pubopts.get_qos() == qos::exactly_once)
                 ? PacketIdBytes // packet_id
                 : 0)
          )
    {
        std::cout << "C:" << this << std::endl;
        using namespace std::literals;
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

        auto pb = val_to_variable_bytes(property_length_);
        for (auto e : pb) {
            property_length_buf_.push_back(e);
        }

        for (auto const& prop : props_) {
            auto id = prop.id();
            if (!validate_property(property_location::publish, id)) {
                throw make_error(
                    errc::bad_message,
                    "v5::publish_packet property "s + id_to_str(id) + " is not allowed"
                );
            }
        }

        remaining_length_ += property_length_buf_.size() + property_length_;

        auto rb = val_to_variable_bytes(remaining_length_);
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }

        if (pubopts.get_qos() == qos::at_least_once ||
            pubopts.get_qos() == qos::exactly_once) {
            endian_store(packet_id, packet_id_.data());
        }
        else {
            endian_store(0, packet_id_.data());
        }
    }

    basic_publish_packet(buffer buf)
        : packet_id_(PacketIdBytes) {
        std::cout << "c:" << this << std::endl;
        // fixed_header
        if (buf.empty()) {
            throw make_error(
                errc::bad_message,
                "v5::publish_packet fixed_header doesn't exist"
            );
        }
        fixed_header_ = static_cast<std::uint8_t>(buf.front());
        buf.remove_prefix(1);

        // remaining_length
        if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
            remaining_length_ = *vl_opt;
        }
        else {
            throw make_error(errc::bad_message, "v5::publish_packet remaining length is invalid");
        }

        // topic_name_length
        if (!insert_advance(buf, topic_name_length_buf_)) {
            throw make_error(
                errc::bad_message,
                "v5::publish_packet length of topic_name is invalid"
            );
        }
        auto topic_name_length = endian_load<std::uint16_t>(topic_name_length_buf_.data());

        // topic_name
        if (buf.size() < topic_name_length) {
            throw make_error(
                errc::bad_message,
                "v5::publish_packet topic_name doesn't match its length"
            );
        }
        topic_name_ = buf.substr(0, topic_name_length);
#if 0 // TBD
        utf8string_check(topic_name_);
#endif
        buf.remove_prefix(topic_name_length);

        // packet_id
        switch (pub::get_qos(fixed_header_)) {
        case qos::at_most_once:
            endian_store(packet_id_t{0}, packet_id_.data());
            break;
        case qos::at_least_once:
        case qos::exactly_once:
            if (!copy_advance(buf, packet_id_)) {
                throw make_error(
                    errc::bad_message,
                    "v5::publish_packet packet_id doesn't exist"
                );
            }
            break;
        default:
            throw make_error(
                errc::bad_message,
                "v5::publish_packet qos is invalid"
            );
            break;
        };

        // property
        auto it = buf.begin();
        if (auto pl_opt = variable_bytes_to_val(it, buf.end())) {
            property_length_ = *pl_opt;
            std::copy(buf.begin(), it, std::back_inserter(property_length_buf_));
            buf.remove_prefix(std::distance(buf.begin(), it));
            if (buf.size() < property_length_) {
                throw make_error(
                    errc::bad_message,
                    "v5::publish_packet properties_don't match its length"
                );
            }
            auto prop_buf = buf.substr(0, property_length_);
            props_ = make_properties(prop_buf, property_location::publish);
            buf.remove_prefix(property_length_);
        }
        else {
            throw make_error(
                errc::bad_message,
                "v5::publish_packet property_length is invalid"
            );
        }

        // payload
        if (!buf.empty()) {
            payloads_.emplace_back(force_move(buf));
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
    std::size_t num_of_const_buffer_sequence() const {
        return
            1 +                   // fixed header
            1 +                   // remaining length
            2 +                   // topic name length, topic name
            [&] {
                if (packet_id() == 0) return 0;
                return 1;
            }() +
            1 +                   // property length
            async_mqtt::num_of_const_buffer_sequence(props_) +
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

    properties const& props() const {
        return props_;
    }

    void remove_topic_add_topic_alias(topic_alias_t val) {
        // add topic_alias property
        auto prop{property::topic_alias{val}};
        auto prop_size = prop.size();
        property_length_ += prop_size;
        props_.push_back(force_move(prop));

        // update property_length_buf
        auto old_property_length_buf_size = property_length_buf_.size();
        property_length_buf_.clear();
        auto pb = val_to_variable_bytes(property_length_);
        for (auto e : pb) {
            property_length_buf_.push_back(e);
        }
        auto new_property_length_buf_size = property_length_buf_.size();

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
        remaining_length_buf_.clear();
        auto rb = val_to_variable_bytes(remaining_length_);
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }
    }

    void add_topic_alias(topic_alias_t val) {
        // add topic_alias property
        auto prop{property::topic_alias{val}};
        auto prop_size = prop.size();
        property_length_ += prop_size;
        props_.push_back(force_move(prop));

        // update property_length_buf
        auto old_property_length_buf_size = property_length_buf_.size();
        property_length_buf_.clear();
        auto pb = val_to_variable_bytes(property_length_);
        for (auto e : pb) {
            property_length_buf_.push_back(e);
        }
        auto new_property_length_buf_size = property_length_buf_.size();

        // update remaining_length
        remaining_length_ +=
            prop_size +
            (new_property_length_buf_size - old_property_length_buf_size);
        remaining_length_buf_.clear();
        auto rb = val_to_variable_bytes(remaining_length_);
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }
    }

    void add_topic(buffer topic) {
        BOOST_ASSERT(topic_name_.empty());

        // add topic
        topic_name_ = force_move(topic);
        endian_store(
            boost::numeric_cast<std::uint16_t>(topic_name_.size()),
            topic_name_length_buf_.data()
        );

        // update remaining_length
        remaining_length_ += topic_name_.size();
        remaining_length_buf_.clear();
        auto rb = val_to_variable_bytes(remaining_length_);
        for (auto e : rb) {
            remaining_length_buf_.push_back(e);
        }
    }

    void update_message_expiry_interval(std::uint32_t val) {
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

private:
    std::uint8_t fixed_header_;
    buffer topic_name_;
    static_vector<char, 2> topic_name_length_buf_;
    static_vector<char, PacketIdBytes> packet_id_;
    std::uint32_t property_length_;
    static_vector<char, 4> property_length_buf_;
    properties props_;
    std::vector<buffer> payloads_;
    std::uint32_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
};

using publish_packet = basic_publish_packet<2>;

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_V5_PUBLISH_HPP
