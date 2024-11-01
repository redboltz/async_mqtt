// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDPOINT_HPP)
#define ASYNC_MQTT_ENDPOINT_HPP

#include <async_mqtt/detail/endpoint_impl.hpp>

/**
 * @defgroup connection MQTT connection
 */

/**
 * @defgroup endpoint endpoint (Packet level MQTT endpoint for client/server,broker)
 * @ingroup connection
 */

namespace async_mqtt {

/**
 * @ingroup endpoint
 * @brief receive packet filter
 */
enum class filter {
    match,  ///< matched control_packet_type is target
    except  ///< no matched control_packet_type is target
};

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
class basic_endpoint {
    using this_type = basic_endpoint<Role, PacketIdBytes, NextLayer>;
    using impl_type = detail::basic_endpoint_impl<Role, PacketIdBytes, NextLayer>;
    using stream_type =
        stream<
            NextLayer
        >;
public:
    /// @brief type of the given NextLayer
    using next_layer_type = typename stream_type::next_layer_type;

    /// @brief lowest_layer_type of the given NextLayer
    using lowest_layer_type = typename stream_type::lowest_layer_type;

    /// @brief executor_type of the given NextLayer
    using executor_type = typename next_layer_type::executor_type;

    /// @brief Type of packet_variant.
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;

    /**
     * @brief constructor
     * @tparam Args Types for the next layer
     * @param  ver  MQTT protocol version client can set v5 or v3_1_1, in addition
     *              server can set undetermined
     * @param  args args for the next layer.
     */
    template <typename... Args>
    explicit
    basic_endpoint(
        protocol_version ver,
        Args&&... args
    );

    /**
     * @brief destructor
     * This function destroys the basic_endpoint,
     * cancelling any outstanding asynchronous operations associated with the basic_endpoint.
     */
    ~basic_endpoint();

    /**
     * @brief copy constructor **deleted**
     */
    basic_endpoint(this_type const&) = delete;

    /**
     * @brief move constructor
     */
    basic_endpoint(this_type&&) = default;

    /**
     * @brief copy assign operator **deleted**
     */
    this_type& operator=(this_type const&) = delete;

    /**
     * @brief move assign operator
     */
    this_type& operator=(this_type&&) = default;

    /**
     * @brief executor getter
     * @return return internal stream's executor
     */
    as::any_io_executor get_executor();

    /**
     * @brief next_layer getter
     * @return const reference of the next_layer
     */
    next_layer_type const& next_layer() const;

    /**
     * @brief next_layer getter
     * @return reference of the next_layer
     */
    next_layer_type& next_layer();

    /**
     * @brief lowest_layer getter
     * @return const reference of the lowest_layer
     */
    lowest_layer_type const& lowest_layer() const;

    /**
     * @brief lowest_layer getter
     * @return reference of the lowest_layer
     */
    lowest_layer_type& lowest_layer();

    /**
     * @brief auto publish response setter. Should be called before async_send()/async_recv() call.
     * @note By default not automatically sending.
     * @param val if true, puback, pubrec, pubrel, and pubcomp are automatically sent
     */
    void set_auto_pub_response(bool val);

    /**
     * @brief auto publish response setter. Should be called before async_send()/async_recv() call.
     * @note By default not automatically sending.
     * @param val if true, puback, pubrec, pubrel, and pubcomp are automatically sent
     */
    void set_auto_ping_response(bool val);

    /**
     * @brief auto map (allocate) topic alias on send PUBLISH packet.
     * If all topic aliases are used, then overwrite by LRU algorithm.
     * \n This function should be called before async_send() call.
     * @note By default not automatically mapping.
     * @param val if true, enable auto mapping, otherwise disable.
     */
    void set_auto_map_topic_alias_send(bool val);

    /**
     * @brief auto replace topic with corresponding topic alias on send PUBLISH packet.
     * Registering topic alias need to do manually.
     * \n This function should be called before async_send() call.
     * @note By default not automatically replacing.
     * @param val if true, enable auto replacing, otherwise disable.
     */
    void set_auto_replace_topic_alias_send(bool val);

    /**
     * @brief Set timeout for receiving PINGRESP packet after PINGREQ packet is sent.
     * If the timer is fired, then the underlying layer is closed from the client side.
     * If the protocol_version is v5, then send DISCONNECT packet with the reason code
     * disconnect_reason_code::keep_alive_timeout automatically before underlying layer is closed.
     * \n This function should be called before async_send() call.
     * @note By default timeout is not set.
     * @param duration if zero, timer is not set; otherwise duration is set.
     *                 The minimum resolution is in milliseconds.
     */
    void set_pingresp_recv_timeout(std::chrono::milliseconds duration);

    /**
     * @brief Set bulk write mode.
     * If true, then concatenate multiple packets' const buffer sequence
     * when async_send() is called before the previous async_send() is not completed.
     * Otherwise, send packet one by one.
     * \n This function should be called before async_send() call.
     * @note By default bulk write mode is false (disabled)
     * @param val if true, enable bulk write mode, otherwise disable it.
     */
    void set_bulk_write(bool val);

    /**
     * @brief Set the read buffer size.
     * If bulk read is enabled, the `val` parameter specifies the size of the internal streambuf.
     * The default size is 65535.
     * \n This function should be called before async_recv() call.
     *
     * @param val buffer size.
     */
    void set_read_buffer_size(std::size_t val);


    // async functions

    /**
     * @brief acuire unique packet_id.
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, @ref packet_id_type)
     *
     * ##### error_code
     * If packet_id is acquired, <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     * If packet_id has already been fully allocated, @ref mqtt_error::packet_identifier_fully_used is set.
     *
     * ##### packet_id_type
     * If success, acquired packet_id is set. Otherwise, 0 is set.
     *
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_acquire_unique_packet_id(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief acuire unique packet_id.
     * If all packet_ids has already been used, then wait until one packet_id would be reusable.
     * packet_id becomes usable when SUBACK, UNSUBACK, PUBACK, PUBREC(with error), or PUBCOMP is received.
     *
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, @ref packet_id_type)
     *
     * ##### error_code
     * If packet_id is acquired, <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     * If the operation is cancelled, @ref boost::asio::error::operation_aborted is set.
     *
     * ##### packet_id_type
     * If success, acquired packet_id is set. Otherwise, 0 is set.
     *
     * ### Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  @li cancellation_type::terminal
     *  @li cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_acquire_unique_packet_id_wait_until(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code)
     *
     * ##### error_code
     * If packet_id is registered, <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     * If the packet_id has already been used, @ref mqtt_error::packet_identifier_conflict is set.
     *
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_register_packet_id(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void()
     *
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_release_packet_id(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief send packet
     *        users can call async_send() before the previous async_send()'s CompletionToken is invoked
     * @param packet packet to send
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code)
     *
     * ##### error_code
     * If packet_id is acquired, <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     * Otherwise,
     * @li If an error occurs during packet construction,
     *     and if it is a CONNECT packet specific error,
     *     @ref connect_reason_code is set.
     *     Otherwise, @ref disconnect_reason_code is set.
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during packet construction,
     *     but an error occurs while checking the packet for sending,
     *     @ref disconnect_reason_code is set.
     * @li If no error occurs during checking the packet for sending,
     *     but an error occurs at an underlying layer while sending a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     * @li If there are no errors during sending the packet,
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     *
     * ### Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  @li cancellation_type::terminal
     *  @li cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <
        typename Packet,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_send(
        Packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, @ref packet_variant_type)
     *
     * ##### error_code and packet_variant_type
     * @li If an error occurs at an underlying layer while receiving a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     @ref packet_variant_type is set to std::monostate
     * @li If there are no errors during receiving the packet,
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     *     @ref packet_variant_type is set to @ref basic_packet_variant.
     *
     * ### Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  @li cancellation_type::terminal
     *  @li cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_recv(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     *        if packet is not filterd, then next async_recv() starts automatically.
     *        if receive error happenes, then token would be invoked.
     * @param types target control_packet_types
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, @ref packet_variant_type)
     *
     * ##### error_code and packet_variant_type
     * @li If an error occurs at an underlying layer while receiving a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     @ref packet_variant_type is set to std::monostate
     * @li If there are no errors during receiving the packet,
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     *     @ref packet_variant_type is set to @ref basic_packet_variant.
     *
     * ### Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  @li cancellation_type::terminal
     *  @li cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_recv(
        std::set<control_packet_type> types,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     *        if packet is not filterd, then next async_recv() starts automatically.
     *        if receive error happenes, then token would be invoked.
     * @param fil  if `match` then matched types are targets. if `except` then not matched types are targets.
     * @param types target control_packet_types
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, @ref packet_variant_type)
     *
     * ##### error_code and packet_variant_type
     * @li If an error occurs at an underlying layer while receiving a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     @ref packet_variant_type is set to std::monostate
     * @li If there are no errors during receiving the packet,
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     *     @ref packet_variant_type is set to @ref basic_packet_variant.
     *
     * ### Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  @li cancellation_type::terminal
     *  @li cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_recv(
        filter fil,
        std::set<control_packet_type> types,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief close the underlying connection
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void()
     *
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_close(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief restore packets
     *        the restored packets would automatically send when CONNACK packet is received
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void()
     *
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_restore_packets(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief get stored packets
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, std::vector<@ref basic_store_packet_variant<PacketIdBytes>>)
     *
     * ##### error_code
     * <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     * Always success. This parameter is prepared for future expansion.
     *
     * ##### std::vector<basic_store_packet_variant<PacketIdBytes>>
     *
     * Inflight packets in sent order.
     * @li PUBLISH packet (QoS1) not received PUBACK packet
     * @li PUBLISH packet (QoS2) not received PUBREC packet
     * @li PUBREL  packet not received PUBCOMP packet
     *
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_get_stored_packets(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief regulate publish packet for store
     *        remove topic alias from the packet and extract the topic name
     * @param packet target packet to regulate
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, @ref v5::basic_publish_packet<PacketIdBytes>)
     *
     * ##### error_code and v5::basic_publish_packet<PacketIdBytes>
     * @li If the given packet has non empty TopicName,
     *     @ref error_code is set to
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a>,
     *     and @ref v5::basic_publish_packet<PacketIdBytes> is set to topic alias removed packet.
     * @li If the given packet has empty TopicName,
     *     and if the given packet has topic alias,
     *     and the topic alias has been registered,
     *     @ref error_code is set to
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a>,
     *     and @ref v5::basic_publish_packet<PacketIdBytes> is set to
     *     the packet that has extracted TopicName and no topic alias.
     * @li Otherwise, error_code is set to mqtt_@ref error::packet_not_regulated,
     *     and @ref v5::basic_publish_packet<PacketIdBytes> is set to the given packet.
     *
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_regulate_for_store(
        v5::basic_publish_packet<PacketIdBytes> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    // sync APIs

    /**
     * @brief acuire unique packet_id.
     * @return std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
     * if acquired return acquired packet id, otherwise std::nullopt
     */
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type> acquire_unique_packet_id();

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @return If true, success, otherwise the packet_id has already been used.
     */
    bool register_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id);

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     */
    void release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id);

    /**
     * @brief Get processed but not released QoS2 packet ids
     *        This function should be called after disconnection
     * @return set of packet_ids
     */
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> get_qos2_publish_handled_pids() const;

    /**
     * @brief Restore processed but not released QoS2 packet ids
     *        This function should be called before receive the first publish
     * @param pids packet ids
     */
    void restore_qos2_publish_handled_pids(std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids);

    /**
     * @brief restore packets
     *        the restored packets would automatically send when CONNACK packet is received
     * @param pvs packets to restore
     */
    void restore_packets(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
    );

    /**
     * @brief get stored packets
     *        sotred packets mean inflight packets.
     *        @li PUBLISH packet (QoS1) not received PUBACK packet
     *        @li PUBLISH packet (QoS1) not received PUBREC packet
     *        @li PUBREL  packet not received PUBCOMP packet
     * @return std::vector<basic_store_packet_variant<PacketIdBytes>>
     */
    std::vector<basic_store_packet_variant<PacketIdBytes>> get_stored_packets() const;

    /**
     * @brief get MQTT protocol version
     * @return MQTT protocol version
     */
    protocol_version get_protocol_version() const;

    /**
     * @brief Get MQTT PUBLISH packet processing status
     * @param pid packet_id corresponding to the publish packet.
     * @return If the packet is processing, then true, otherwise false.
     */
    bool is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const;

    /**
     * @brief Regulate publish packet for store
     *        If topic is empty, extract topic from topic alias, and remove topic alias
     *        Otherwise, remove topic alias if exists.
     * @param packet packet to regulate
     * @param ec     error_code for repoting error
     */
    void regulate_for_store(
        v5::basic_publish_packet<PacketIdBytes>& packet,
        error_code& ec
    ) const;

    /**
     * @brief Set PINGREQ packet sending interval.
     * @note By default, PINGREQ packet sending interval is set the same value as
     *       CONNECT packet keep alive seconds.
     *       This function overrides it.
     * @param duration if zero, timer is not set; otherwise duration is set.
     *                 The minimum resolution is in milliseconds.
     */
    void set_pingreq_send_interval(std::chrono::milliseconds duration);

    /**
     * @brief rebinds the basic_endpoint type to another executor
     */
    template <typename Executor1>
    struct rebind_executor {
        using other = basic_endpoint<
            Role,
            PacketIdBytes,
            typename NextLayer::template rebind_executor<Executor1>::other
        >;
    };

private: // compose operation impl
    /**
     *  @brief Rebinding constructor
     *         This constructor creates a basic_endpoint from the basic_endpoint with a different executor.
     *  @param other The other basic_endpoint to construct from.
     */
    template <typename Other>
    explicit
    basic_endpoint(
        basic_endpoint<Role, PacketIdBytes, Other>&& other
    );

private:
    std::shared_ptr<impl_type> impl_;
};

} // namespace async_mqtt

#include <async_mqtt/impl/endpoint_impl.hpp>
#include <async_mqtt/impl/endpoint_acquire_unique_packet_id.hpp>
#include <async_mqtt/impl/endpoint_acquire_unique_packet_id_wait_until.hpp>
#include <async_mqtt/impl/endpoint_register_packet_id.hpp>
#include <async_mqtt/impl/endpoint_release_packet_id.hpp>
#include <async_mqtt/impl/endpoint_send.hpp>
#include <async_mqtt/impl/endpoint_recv.hpp>
#include <async_mqtt/impl/endpoint_close.hpp>
#include <async_mqtt/impl/endpoint_restore_packets.hpp>
#include <async_mqtt/impl/endpoint_get_stored_packets.hpp>
#include <async_mqtt/impl/endpoint_regulate_for_store.hpp>
#include <async_mqtt/impl/endpoint_add_retry.hpp>

#endif // ASYNC_MQTT_ENDPOINT_HPP
