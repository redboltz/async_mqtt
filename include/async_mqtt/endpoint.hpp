// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDPOINT_HPP)
#define ASYNC_MQTT_ENDPOINT_HPP

#include <set>
#include <deque>

#include <async_mqtt/error.hpp>
#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/util/value_allocator.hpp>
#include <async_mqtt/util/make_shared_helper.hpp>
#include <async_mqtt/util/stream.hpp>
#include <async_mqtt/util/store.hpp>
#include <async_mqtt/role.hpp>
#include <async_mqtt/util/log.hpp>
#include <async_mqtt/util/topic_alias_send.hpp>
#include <async_mqtt/util/topic_alias_recv.hpp>
#include <async_mqtt/util/packet_id_manager.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

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

/**
 * @ingroup endpoint
 * @brief MQTT endpoint corresponding to the connection
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 * @tparam Role          role for packet sendable checking
 * @tparam PacketIdBytes MQTT spec is 2. You can use `endpoint` for that.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <role Role, std::size_t PacketIdBytes, typename NextLayer>
class basic_endpoint : public std::enable_shared_from_this<basic_endpoint<Role, PacketIdBytes, NextLayer>>{
    enum class connection_status {
        connecting,
        connected,
        disconnecting,
        closing,
        closed
    };

    using this_type = basic_endpoint<Role, PacketIdBytes, NextLayer>;
    using this_type_sp = std::shared_ptr<this_type>;
    using this_type_wp = std::weak_ptr<this_type>;
    using stream_type =
        stream<
            NextLayer
        >;

    template <typename T>
    friend class make_shared_helper;

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
     * @brief create
     * @tparam Args Types for the next layer
     * @param  ver  MQTT protocol version client can set v5 or v3_1_1, in addition
     *              server can set undetermined
     * @param  args args for the next layer.
     * - There are predefined next layer types:
     *    - protocol::mqtt
     *    - protocol::mqtts
     *    - protocol::ws
     *    - protocol::wss
     * @return shared_ptr of basic_endpoint.
     */
    template <typename... Args>
    static std::shared_ptr<this_type> create(
        protocol_version ver,
        Args&&... args
    ) {
        return make_shared_helper<this_type>::make_shared(ver, std::forward<Args>(args)...);
    }

    /**
     * @brief destructor
     */
    ~basic_endpoint();

    /**
     * @brief copy constructor **deleted**
     */
    basic_endpoint(this_type const&) = delete;

    /**
     * @brief move constructor **deleted**
     */
    basic_endpoint(this_type&&) = delete;

    /**
     * @brief copy assign operator **deleted**
     */
    this_type& operator=(this_type const&) = delete;

    /**
     * @brief move assign operator **deleted**
     */
    this_type& operator=(this_type&&) = delete;

    /**
     * @brief executor getter
     * @return return internal stream's executor
     */
    as::any_io_executor get_executor() const {
        return stream_->get_executor();
    }

    /**
     * @brief next_layer getter
     * @return const reference of the next_layer
     */
    next_layer_type const& next_layer() const {
        return stream_->next_layer();
    }
    /**
     * @brief next_layer getter
     * @return reference of the next_layer
     */
    next_layer_type& next_layer() {
        return stream_->next_layer();
    }

    /**
     * @brief lowest_layer getter
     * @return const reference of the lowest_layer
     */
    lowest_layer_type const& lowest_layer() const {
        return stream_->lowest_layer();
    }
    /**
     * @brief lowest_layer getter
     * @return reference of the lowest_layer
     */
    lowest_layer_type& lowest_layer() {
        return stream_->lowest_layer();
    }

    /**
     * @brief auto publish response setter. Should be called before send()/recv() call.
     * @note By default not automatically sending.
     * @param val if true, puback, pubrec, pubrel, and pubcomp are automatically sent
     */
    void set_auto_pub_response(bool val) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "set_auto_pub_response val:" << val;
        auto_pub_response_ = val;
    }

    /**
     * @brief auto pingreq response setter. Should be called before send()/recv() call.
     * @note By default not automatically sending.
     * @param val if true, puback, pubrec, pubrel, and pubcomp are automatically sent
     */
    void set_auto_ping_response(bool val) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "set_auto_ping_response val:" << val;
        auto_ping_response_ = val;
    }

    /**
     * @brief auto map (allocate) topic alias on send PUBLISH packet.
     * If all topic aliases are used, then overwrite by LRU algorithm.
     * \n This function should be called before send() call.
     * @note By default not automatically mapping.
     * @param val if true, enable auto mapping, otherwise disable.
     */
    void set_auto_map_topic_alias_send(bool val) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "set_auto_map_topic_alias_send val:" << val;
        auto_map_topic_alias_send_ = val;
    }

    /**
     * @brief auto replace topic with corresponding topic alias on send PUBLISH packet.
     * Registering topic alias need to do manually.
     * \n This function should be called before send() call.
     * @note By default not automatically replacing.
     * @param val if true, enable auto replacing, otherwise disable.
     */
    void set_auto_replace_topic_alias_send(bool val) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "set_auto_replace_topic_alias_send val:" << val;
        auto_replace_topic_alias_send_ = val;
    }

    /**
     * @brief Set timeout for receiving PINGRESP packet after PINGREQ packet is sent.
     * If the timer is fired, then the underlying layer is closed from the client side.
     * If the protocol_version is v5, then send DISCONNECT packet with the reason code
     * disconnect_reason_code::keep_alive_timeout automatically before underlying layer is closed.
     * \n This function should be called before send() call.
     * @note By default timeout is not set.
     * @param ms if 0, timer is not set, otherwise set val milliseconds.
     */
    void set_pingresp_recv_timeout_ms(std::size_t ms) {
        if (ms == 0) {
            pingresp_recv_timeout_ms_ = std::nullopt;
        }
        else {
            pingresp_recv_timeout_ms_.emplace(ms);
        }
    }

    /**
     * @brief Set bulk write mode.
     * If true, then concatenate multiple packets' const buffer sequence
     * when send() is called before the previous send() is not completed.
     * Otherwise, send packet one by one.
     * \n This function should be called before send() call.
     * @note By default bulk write mode is false (disabled)
     * @param val if true, enable bulk write mode, otherwise disable it.
     */
    void set_bulk_write(bool val) {
        stream_->set_bulk_write(val);
    }

    /**
     * @brief Set read buffer size.
     * Set buffer size of internal async_read_some().
     * @param val buffer size
     */
    void set_read_buffer_size(std::size_t val) {
        stream_->set_read_buffer_size(val);
    }


    // async functions

    /**
     * @brief acuire unique packet_id.
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code, typename basic_packet_id_type<PacketIdBytes>::type)
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *  - cancellation_type::total
     *
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, typename basic_packet_id_type<PacketIdBytes>::type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_acquire_unique_packet_id(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief acuire unique packet_id.
     * If packet_id is fully acquired, then wait until released.
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code, typename basic_packet_id_type<PacketIdBytes>::type)
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *  - cancellation_type::total
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, typename basic_packet_id_type<PacketIdBytes>::type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_acquire_unique_packet_id_wait_until(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code)
     *    - success if packet is acquired, otherwise packet_identifier_fully_used
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *  - cancellation_type::total
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_register_packet_id(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @param token
     * - CompletionToken
     *    - Signature: void()
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *  - cancellation_type::total
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void()
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_release_packet_id(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief send packet
     *        users can call send() before the previous send()'s CompletionToken is invoked
     * @param packet packet to send
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code)
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_write_some operation.
     */
    template <
        typename Packet,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_send(
        Packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code, packet_variant_type)
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some operation.
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, packet_variant_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_recv(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     *        if packet is not filterd, then next recv() starts automatically.
     *        if receive error happenes, then token would be invoked.
     * @param types target control_packet_types
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code, packet_variant_type)
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some operation.
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, packet_variant_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_recv(
        std::set<control_packet_type> types,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     *        if packet is not filterd, then next recv() starts automatically.
     *        if receive error happenes, then token would be invoked.
     * @param fil  if `match` then matched types are targets. if `except` then not matched types are targets.
     * @param types target control_packet_types
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code, packet_variant_type)
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some operation.
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, packet_variant_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_recv(
        filter fil,
        std::set<control_packet_type> types,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief close the underlying connection
     * @param token
     * - CompletionToken
     *    - Signature: void()
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void()
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_close(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief restore packets
     *        the restored packets would automatically send when CONNACK packet is received
     * @param pvs packets to restore
     * @param token
     * - CompletionToken
     *    - Signature: void()
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *  - cancellation_type::total
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void()
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_restore_packets(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief get stored packets
     *        - stored packets mean inflight packets.
     *           - PUBLISH packet (QoS1) not received PUBACK packet
     *           - PUBLISH packet (QoS2) not received PUBREC packet
     *           - PUBREL  packet not received PUBCOMP packet
     *
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code, std::vector<basic_store_packet_variant<PacketIdBytes>>)
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *  - cancellation_type::total
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, std::vector<basic_store_packet_variant<PacketIdBytes>>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_get_stored_packets(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    ) const;

    /**
     * @brief regulate publish packet for store
     *        remove topic alias from the packet and extract the topic name
     * @param packet target packet to regulate
     * @param token
     * - CompletionToken
     *    - Signature: void(error_code, v5::basic_publish_packet<PacketIdBytes>)
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *  - cancellation_type::total
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, v5::basic_publish_packet<PacketIdBytes>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_regulate_for_store(
        v5::basic_publish_packet<PacketIdBytes> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    ) const;

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
     *        - PUBLISH packet (QoS1) not received PUBACK packet
     *        - PUBLISH packet (QoS1) not received PUBREC packet
     *        - PUBREL  packet not received PUBCOMP packet
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
     * @param ms if 0, timer is not set, otherwise set val milliseconds.
     */
    void set_pingreq_send_interval_ms(std::size_t ms);

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
     * @brief constructor
     * @tparam Args Types for the next layer
     * @param  ver  MQTT protocol version client can set v5 or v3_1_1, in addition
     *              server can set undetermined
     * @param  args args for the next layer. There are predefined next layer types:
     *              \n @link protocol::mqtt @endlink, @link protocol::mqtts @endlink,
     *              @link protocol::ws @endlink, and @link protocol::wss @endlink.
     */
    template <typename... Args>
    explicit
    basic_endpoint(
        protocol_version ver,
        Args&&... args
    );

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

    static constexpr bool can_send_as_client(role r);
    static constexpr bool can_send_as_server(role r);
    static std::optional<topic_alias_type> get_topic_alias(properties const& props);

    struct acquire_unique_packet_id_op;
    struct acquire_unique_packet_id_wait_until_op;
    struct register_packet_id_op;
    struct release_packet_id_op;
    template <typename Packet> struct send_op;
    struct recv_op;
    struct close_op;
    struct restore_packets_op;
    struct get_stored_packets_op;
    struct regulate_for_store_op;
    struct add_retry_op;

private:

    template <
        typename Packet,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_send(
        Packet packet,
        bool from_queue,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_add_retry(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    bool enqueue_publish(v5::basic_publish_packet<PacketIdBytes>& packet);
    void send_stored();
    void initialize();

    void reset_pingreq_send_timer();
    void reset_pingreq_recv_timer();
    void reset_pingresp_recv_timer();

    void notify_retry_one();
    void complete_retry_one();
    void notify_retry_all();
    bool has_retry() const;

    void clear_pid_man();
    void release_pid(typename basic_packet_id_type<PacketIdBytes>::type pid);

private:
    protocol_version protocol_version_;
    std::shared_ptr<stream_type> stream_;
    packet_id_manager<typename basic_packet_id_type<PacketIdBytes>::type> pid_man_;
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pid_suback_;
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pid_unsuback_;
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pid_puback_;
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pid_pubrec_;
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> pid_pubcomp_;

    bool need_store_ = false;
    store<PacketIdBytes> store_;

    bool auto_pub_response_ = false;
    bool auto_ping_response_ = false;

    bool auto_map_topic_alias_send_ = false;
    bool auto_replace_topic_alias_send_ = false;
    std::optional<topic_alias_send> topic_alias_send_;
    std::optional<topic_alias_recv> topic_alias_recv_;

    receive_maximum_type publish_send_max_{receive_maximum_max};
    receive_maximum_type publish_recv_max_{receive_maximum_max};
    receive_maximum_type publish_send_count_{0};

    std::set<typename basic_packet_id_type<PacketIdBytes>::type> publish_recv_;
    std::deque<v5::basic_publish_packet<PacketIdBytes>> publish_queue_;

    ioc_queue close_queue_;

    std::uint32_t maximum_packet_size_send_{packet_size_no_limit};
    std::uint32_t maximum_packet_size_recv_{packet_size_no_limit};

    connection_status status_{connection_status::closed};

    std::optional<std::size_t> pingreq_send_interval_ms_;
    std::optional<std::size_t> pingreq_recv_timeout_ms_;
    std::optional<std::size_t> pingresp_recv_timeout_ms_;

    std::shared_ptr<as::steady_timer> tim_pingreq_send_;
    std::shared_ptr<as::steady_timer> tim_pingreq_recv_;
    std::shared_ptr<as::steady_timer> tim_pingresp_recv_;

    std::set<typename basic_packet_id_type<PacketIdBytes>::type> qos2_publish_handled_;

    bool recv_processing_ = false;
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> qos2_publish_processing_;

    struct tim_cancelled;
    std::deque<tim_cancelled> tim_retry_acq_pid_queue_;
    bool packet_id_released_ = false;
};

/**
 * @ingroup endpoint
 * @related basic_endpoint
 * @brief Type alias of basic_endpoint (PacketIdBytes=2).
 *        This is for typical usecase.
 * @tparam Role          role for packet sendable checking
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <role Role, typename NextLayer>
using endpoint = basic_endpoint<Role, 2, NextLayer>;

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
