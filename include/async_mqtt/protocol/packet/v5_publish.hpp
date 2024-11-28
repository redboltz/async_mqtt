// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_V5_PUBLISH_HPP)
#define ASYNC_MQTT_PACKET_V5_PUBLISH_HPP

#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/error.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/pubopts.hpp>
#include <async_mqtt/packet/property_variant.hpp>
#include <async_mqtt/packet/detail/is_payload.hpp>

#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/variable_bytes.hpp>
#include <async_mqtt/util/static_vector.hpp>

#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
#include <async_mqtt/util/json_like_out.hpp>
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)

/**
 * @defgroup publish_v5 PUBLISH packet (v5.0)
 * @ingroup packet_v5
 */

/**
 * @defgroup publish_v5_detail implementation class
 * @ingroup publish_v5
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
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901100"></a>
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_publish.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
class basic_publish_packet {
public:

    /**
     * @brief constructor
     * @tparam StringViewLike Type of the topic. Any type can convert to std::string_view.
     * @tparam Payload Type of the payload. Any type can convert to std::string_view or its sequence.
     * @param packet_id  MQTT PacketIdentifier. If QoS0 then it must be 0. You can use no packet_id version constructor.
     *                   If QoS is 0 or 1 then, the packet_id must be acquired by
     *                   basic_endpoint::acquire_unique_packet_id(), or must be registered by
     *                   basic_endpoint::register_packet_id().
     *                   \n If QoS0, the packet_id is not sent actually.
     *                   \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901108"></a>
     * @param topic_name MQTT TopicName
     *                   \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901107"></a>
     * @param payloads   The body message of the packet. It could be a single buffer of multiple buffer sequence.
     *                   \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901119"></a>
     * @param pubopts    Publish Options. It contains the following elements:
     *                   \n DUP See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901102"></a>
     *                   \n QoS See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901103"></a>
     *                   \n RETAIN See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901104"></a>
     * @param props      Publish properties.
     *                   \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901109"></a>
     */
    template <
        typename StringViewLike,
        typename Payload,
        std::enable_if_t<
            std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view> &&
            detail::is_payload<Payload>(),
            std::nullptr_t
        > = nullptr
    >
    explicit basic_publish_packet(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        StringViewLike&& topic_name,
        Payload&& payloads,
        pub::opts pubopts,
        properties props = {}
    );

    /**
     * @brief constructor for QoS0
     * @tparam StringViewLike Type of the topic. Any type can convert to std::string_view.
     * @tparam Payload Type of the payload. Any type can convert to std::string_view or its sequence.
     * @param topic_name MQTT TopicName
     *                   \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901107"></a>
     * @param payloads   The body message of the packet. It could be a single buffer of multiple buffer sequence.
     *                   \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901119"></a>
     * @param pubopts    Publish Options. It contains the following elements:
     *                   \n DUP See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901102"></a>
     *                   \n QoS See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901103"></a>
     *                   \n RETAIN See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901104"></a>
     * @param props      Publish properties.
     *                   \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901109"></a>
     */
    template <
        typename StringViewLike,
        typename Payload,
        std::enable_if_t<
            std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view> &&
            detail::is_payload<Payload>(),
            std::nullptr_t
        > = nullptr
    >
    explicit basic_publish_packet(
        StringViewLike&& topic_name,
        Payload&& payloads,
        pub::opts pubopts,
        properties props = {}
    );

    /**
     * @brief Get MQTT control packet type
     * @return control packet type
     */
    static constexpr control_packet_type type() {
        return control_packet_type::publish;
    }

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
    typename basic_packet_id_type<PacketIdBytes>::type packet_id() const;

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
    constexpr void set_dup(bool dup) {
        pub::set_dup(fixed_header_, dup);
    }

    /**
     * @brief Get properties
     * @return properties
     */
    properties const& props() const;

    /**
     * @brief Remove topic and add topic_alias
     * This is for applying topic_alias.
     * @param val topic_alias
     */
    void remove_topic_add_topic_alias(topic_alias_type val);

    /**
     * @brief Add topic_alias
     * This is for registering topic_alias.
     * @param val topic_alias
     */
    void add_topic_alias(topic_alias_type val);

    /**
     * @brief Add topic
     * This is for extracting topic_alias.
     * @param topic to add
     */
    void add_topic(std::string topic);

    /**
     * @brief Remove topic alias
     * This is for prepareing to store packet.
     */
    void remove_topic_alias();

    /**
     * @brief Remove topic and add topic_alias
     * This is for extracting topic from the topic_alias.
     * @param topic topic_alias
     */
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
    friend std::optional<basic_packet_variant<PacketIdBytesArg>>
    async_mqtt::buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

#if defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)
    friend struct ::ut_packet::v5_publish;
    friend struct ::ut_packet::v5_publish_qos0;
    friend struct ::ut_packet::v5_publish_invalid;
    friend struct ::ut_packet::v5_publish_pid4;
    friend struct ::ut_packet::v5_publish_topic_alias;
    friend struct ::ut_packet::v5_publish_error;
#endif // defined(ASYNC_MQTT_UNIT_TEST_FOR_PACKET)

    // private constructor for internal use
    explicit basic_publish_packet(buffer buf, error_code& ec);

    struct tag_internal{};
    explicit basic_publish_packet(
        tag_internal,
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        buffer&& topic_name,
        std::vector<buffer>&& payloads,
        pub::opts pubopts,
        properties props
    );

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
 * @brief less than operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs less than the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_publish.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
bool operator<(basic_publish_packet<PacketIdBytes> const& lhs, basic_publish_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_publish_packet
 * @brief equal operator
 * @param lhs compare target
 * @param rhs compare target
 * @return true if the lhs equal to the rhs, otherwise false.
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_publish.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
bool operator==(basic_publish_packet<PacketIdBytes> const& lhs, basic_publish_packet<PacketIdBytes> const& rhs);

/**
 * @related basic_publish_packet
 * @brief stream output operator
 * @param o output stream
 * @param v target
 * @return  output stream
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_publish.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
std::ostream& operator<<(std::ostream& o, basic_publish_packet<PacketIdBytes> const& v);

/**
 * @ingroup publish_v5
 * @related basic_publish_packet
 * @brief Type alias of basic_publish_packet (PacketIdBytes=2).
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/v5_publish.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using publish_packet = basic_publish_packet<2>;

} // namespace async_mqtt::v5

#include <async_mqtt/packet/impl/v5_publish.hpp>

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/packet/impl/v5_publish.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PACKET_V5_PUBLISH_HPP
