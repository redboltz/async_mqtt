// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PUBLISH_HPP)
#define ASYNC_MQTT_PACKET_V5_PUBLISH_HPP

#include <async_mqtt/buffer_to_packet_variant_fwd.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/variable_bytes.hpp>
#include <async_mqtt/type.hpp>

#include <async_mqtt/util/static_vector.hpp>

#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/fixed_header.hpp>
#include <async_mqtt/packet/pubopts.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
#include <async_mqtt/util/json_like_out.hpp>
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)

/**
 * @defgroup publish_v5
 * @ingroup packet_v5
 */

/**
 * @defgroup publish_v5_detail
 * @ingroup publish_v5
 * @brief packet internal detailes (e.g. type-aliased API's actual type information)
 */

namespace async_mqtt::v5 {

namespace as = boost::asio;

/**
 * @ingroup publish_v5_detail
 * @brief MQTT PUBLISH packet (v5)
 * @tparam PacketIdBytes size of packet_id
 *
 * If both the client and the broker keeping the session, QoS1 and QoS2 PUBLISH packet is
 * stored in the endpoint for resending if disconnect/reconnect happens. In addition,
 * the client can sent the packet at offline. The packets are stored and will send after
 * the next connection is established.
 * If the session doesn' exist or lost, then the stored packets are erased.
 * \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901100
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
     *                   \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901108
     * @param topic_name MQTT TopicName
     *                   \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901107
     * @param payloads   The body message of the packet. It could be a single buffer of multiple buffer sequence.
     *                   \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901119
     * @param pubopts    Publish Options. It contains the following elements:
     *                   \n DUP See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901102
     *                   \n QoS See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901103
     *                   \n RETAIN See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901104
     */
    template <
        typename StringViewLike,
        typename BufferSequence,
        std::enable_if_t<
            std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view> &&
            (
                is_buffer_sequence<std::decay_t<BufferSequence>>::value ||
                std::is_convertible_v<std::decay_t<BufferSequence>, std::string_view>
            ),
            std::nullptr_t
        > = nullptr
    >
    basic_publish_packet(
        packet_id_t packet_id,
        StringViewLike&& topic_name,
        BufferSequence&& payloads,
        pub::opts pubopts,
        properties props = {}
    );

    /**
     * @brief constructor for QoS0
     * @param topic_name MQTT TopicName
     *                   \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901107
     * @param payloads   The body message of the packet. It could be a single buffer of multiple buffer sequence.
     *                   \n See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901119
     * @param pubopts    Publish Options. It contains the following elements:
     *                   \n DUP See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901102
     *                   \n QoS See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901103
     *                   \n RETAIN See https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901104
     */
    template <
        typename StringViewLike,
        typename BufferSequence,
        std::enable_if_t<
            std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view> &&
            (
                is_buffer_sequence<std::decay_t<BufferSequence>>::value ||
                std::is_convertible_v<std::decay_t<BufferSequence>, std::string_view>
            ),
            std::nullptr_t
        > = nullptr
    >
    basic_publish_packet(
        StringViewLike&& topic_name,
        BufferSequence&& payloads,
        pub::opts pubopts,
        properties props = {}
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type();

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const;

    /**
     * @brief Get packet size.
     * @return packet size
     */
    std::size_t size() const;

    /**
     * @brief Get number of element of const_buffer_sequence
     * @return number of element of const_buffer_sequence
     */
    std::size_t num_of_const_buffer_sequence() const;

    /**
     * @brief Get packet id
     * @return packet_id
     */
    packet_id_t packet_id() const;

    /**
     * @brief Get publish_options
     * @return publish_options.
     */
    constexpr pub::opts opts() const;

    /**
     * @brief Get topic name
     * @return topic name
     */
    std::string topic() const;

    /**
     * @brief Get topic as a buffer
     * @return topic name
     */
    buffer const& topic_as_buffer() const;

    /**
     * @brief Get payload
     * @return payload
     */
    std::string payload() const;

    /**
     * @brief Get payload range
     * @return A pair of forward iterators
     */
    auto payload_range() const;

    /**
     * @brief Get payload as a sequence of buffer
     * @return payload
     */
    std::vector<buffer> const& payload_as_buffer() const;

    /**
     * @brief Set dup flag
     * @param dup flag value to set
     */
    constexpr void set_dup(bool dup);

    /**
     * @brief Get properties
     * @return properties
     */
    properties const& props() const;

    /**
     * @brief Remove topic and add topic_alias
     *
     * This is for applying topic_alias.
     * @param val topic_alias
     */
    void remove_topic_add_topic_alias(topic_alias_t val);

    /**
     * @brief Add topic_alias
     *
     * This is for registering topic_alias.
     * @param val topic_alias
     */
    void add_topic_alias(topic_alias_t val);

    /**
     * @brief Remove topic and add topic_alias
     *
     * This is for extracting topic from the topic_alias.
     * @param val topic_alias
     */
    void add_topic(std::string topic);

    void remove_topic_alias();

    void remove_topic_alias_add_topic(std::string topic);

    /**
     * @brief Update MessageExpiryInterval property
     * @param val message_expiry_interval
     */
    void update_message_expiry_interval(std::uint32_t val);

private:
    void update_remaining_length_buf();

    std::tuple<std::size_t, std::size_t> update_property_length_buf();

    std::size_t remove_topic_alias_impl();

    void add_topic_impl(std::string topic);

private:

    template <std::size_t PacketIdBytesArg>
    friend basic_packet_variant<PacketIdBytesArg>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_publish;
    friend struct ::ut_packet::v5_publish_qos0;
    friend struct ::ut_packet::v5_publish_invalid;
    friend struct ::ut_packet::v5_publish_pid4;
    friend struct ::ut_packet::v5_publish_topic_alias;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    basic_publish_packet(buffer buf);

private:
    std::uint8_t fixed_header_;
    buffer topic_name_;
    static_vector<char, 2> topic_name_length_buf_;
    static_vector<char, PacketIdBytes> packet_id_;
    std::size_t property_length_;
    static_vector<char, 4> property_length_buf_;
    properties props_;
    std::vector<buffer> payloads_;
    std::size_t remaining_length_;
    static_vector<char, 4> remaining_length_buf_;
};

/**
 * @related basic_publish_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 */
template <std::size_t PacketIdBytes>
std::ostream& operator<<(std::ostream& o, basic_publish_packet<PacketIdBytes> const& v);

/**
 * @ingroup publish_v5
 * @related basic_publish_packet
 * @brief Type alias of basic_publish_packet (PacketIdBytes=2).
 */
using publish_packet = basic_publish_packet<2>;

} // namespace async_mqtt::v5

#include <async_mqtt/packet/impl/v5_publish.hpp>

#endif // ASYNC_MQTT_PACKET_V5_PUBLISH_HPP
