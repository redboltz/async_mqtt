// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ENDPOINT_HPP)
#define ASYNC_MQTT_ENDPOINT_HPP

#include <set>
#include <deque>
#include <atomic>

#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/util/value_allocator.hpp>
#include <async_mqtt/util/make_shared_helper.hpp>
#include <async_mqtt/stream.hpp>
#include <async_mqtt/store.hpp>
#include <async_mqtt/role.hpp>
#include <async_mqtt/log.hpp>
#include <async_mqtt/topic_alias_send.hpp>
#include <async_mqtt/topic_alias_recv.hpp>
#include <async_mqtt/packet_id_manager.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

/**
 * @defgroup connection
 * @brief MQTT and underlying layer connection related topics
 */

/**
 * @defgroup endpoint
 * @ingroup connection
 * @brief MQTT connection layer that can be used not only a client but also a server (broker).
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

    static constexpr bool can_send_as_client(role r) {
        return static_cast<int>(r) & static_cast<int>(role::client);
    }

    static constexpr bool can_send_as_server(role r) {
        return static_cast<int>(r) & static_cast<int>(role::server);
    }

    static inline std::optional<topic_alias_t> get_topic_alias(properties const& props) {
        std::optional<topic_alias_t> ta_opt;
        for (auto const& prop : props) {
            prop.visit(
                overload {
                    [&](property::topic_alias const& p) {
                        ta_opt.emplace(p.val());
                    },
                    [](auto const&) {
                    }
                }
            );
            if (ta_opt) return ta_opt;
        }
        return ta_opt;
    }

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
    /// @brief The next layer type. The type is the same as NextLayer as the template argument.
    using next_layer_type = typename stream_type::next_layer_type;

    /// @brief The next layer's lowest layer type.
    using lowest_layer_type = typename stream_type::lowest_layer_type;

    /// @brief The next layer's executor type
    using executor_type = typename next_layer_type::executor_type;

    /// @brief The value given as PacketIdBytes
    static constexpr std::size_t packet_id_bytes = PacketIdBytes;

    /// @brief Type of packet_variant.
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;

    /// @brief Type of MQTT Packet Identifier.
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    /**
     * @brief create
     * @tparam Args Types for the next layer
     * @param  ver  MQTT protocol version client can set v5 or v3_1_1, in addition
     *              server can set undetermined
     * @param  args args for the next layer. There are predefined next layer types:
     *              \n @link protocol::mqtt @endlink, @link protocol::mqtts @endlink,
     *              @link protocol::ws @endlink, and @link protocol::wss @endlink.
     * @return shared_ptr of basic_endpoint.
     */
    template <typename... Args>
    static std::shared_ptr<this_type> create(
        protocol_version ver,
        Args&&... args
    ) {
        return make_shared_helper<this_type>::make_shared(ver, std::forward<Args>(args)...);
    }

    ~basic_endpoint() {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "destroy";
    }

    basic_endpoint(this_type const&) = delete;
    basic_endpoint(this_type&&) = delete;
    this_type& operator=(this_type const&) = delete;
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
     * @param val if 0, timer is not set, otherwise set val milliseconds.
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


    // async functions

    /**
     * @brief acuire unique packet_id.
     * @param token the param is std::optional<packet_id_t>
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(std::optional<packet_id_t>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    acquire_unique_packet_id(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief acuire unique packet_id.
     * If packet_id is fully acquired, then wait until released.
     * @param token the param is packet_id_t
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(packet_id_t)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    acquire_unique_packet_id_wait_until(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @param token     the param is bool. If true, success, otherwise the packet_id has already been used.
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(bool)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    register_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @param token     the param is void
     * @return deduced by token
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
    release_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief send packet
     *        users can call send() before the previous send()'s CompletionToken is invoked
     * @param packet packet to send
     * @param token  the param is system_error
     * @return deduced by token
     */
    template <
        typename Packet,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(system_error)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    send(
        Packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     * @param token the param is packet_variant_type
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(packet_variant_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    recv(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     *        if packet is not filterd, then next recv() starts automatically.
     *        if receive error happenes, then token would be invoked.
     * @param types target control_packet_types
     * @param token the param is packet_variant_type
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(packet_variant_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    recv(
        std::set<control_packet_type> types,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     *        if packet is not filterd, then next recv() starts automatically.
     *        if receive error happenes, then token would be invoked.
     * @params fil  if `match` then matched types are targets. if `except` then not matched types are targets.
     * @param types target control_packet_types
     * @param token the param is packet_variant_type
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(packet_variant_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    recv(
        filter fil,
        std::set<control_packet_type> types,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief close the underlying connection
     * @param token  the param is void
     * @return deduced by token
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
    close(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief restore packets
     *        the restored packets would automatically send when CONNACK packet is received
     * @param pvs packets to restore
     * @param token  the param is void
     * @return deduced by token
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
    restore_packets(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief get stored packets
     *        sotred packets mean inflight packets.
     *        - PUBLISH packet (QoS1) not received PUBACK packet
     *        - PUBLISH packet (QoS1) not received PUBREC packet
     *        - PUBREL  packet not received PUBCOMP packet
     * @param token  the param is std::vector<basic_store_packet_variant<PacketIdBytes>>
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(std::vector<basic_store_packet_variant<PacketIdBytes>>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    get_stored_packets(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    ) const;

    /**
     * @brief regulate publish packet for store
     *        remove topic alias from the packet and extract the topic name
     * @param token  the param is v5::basic_publish_packet<PacketIdBytes>
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(v5::basic_publish_packet<PacketIdBytes>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    regulate_for_store(
        v5::basic_publish_packet<PacketIdBytes> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    ) const;

    // sync APIs (Thread unsafe without strand)

    /**
     * @brief acuire unique packet_id.
     * @return std::optional<packet_id_t> if acquired return acquired packet id, otherwise std::nullopt
     * @note This function is SYNC function that thread unsafe without strand.
     */
    std::optional<packet_id_t> acquire_unique_packet_id();

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @return If true, success, otherwise the packet_id has already been used.
     * @note This function is SYNC function that thread unsafe without strand.
     */
    bool register_packet_id(packet_id_t pid);

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @note This function is SYNC function that thread unsafe without strand.
     */
    void release_packet_id(packet_id_t pid);

    /**
     * @brief Get processed but not released QoS2 packet ids
     *        This function should be called after disconnection
     * @return set of packet_ids
     * @note This function is SYNC function that thread unsafe without strand.
     */
    std::set<packet_id_t> get_qos2_publish_handled_pids() const;

    /**
     * @brief Restore processed but not released QoS2 packet ids
     *        This function should be called before receive the first publish
     * @param pids packet ids
     * @note This function is SYNC function that thread unsafe without strand.
     */
    void restore_qos2_publish_handled_pids(std::set<packet_id_t> pids);

    /**
     * @brief restore packets
     *        the restored packets would automatically send when CONNACK packet is received
     * @param pvs packets to restore
     * @note This function is SYNC function that thread unsafe without strand.
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
     * @note This function is SYNC function that thread unsafe without strand.
     */
    std::vector<basic_store_packet_variant<PacketIdBytes>> get_stored_packets() const;

    /**
     * @brief get MQTT protocol version
     * @return MQTT protocol version
     * @note This function is SYNC function that thread unsafe without strand.
     */
    protocol_version get_protocol_version() const;

    /**
     * @brief Get MQTT PUBLISH packet processing status
     * @param pid packet_id corresponding to the publish packet.
     * @return If the packet is processing, then true, otherwise false.
     * @note This function is SYNC function that thread unsafe without strand.
     */
    bool is_publish_processing(packet_id_t pid) const;

    /**
     * @brief Regulate publish packet for store
     *        If topic is empty, extract topic from topic alias, and remove topic alias
     *        Otherwise, remove topic alias if exists.
     * @param packet packet to regulate
     * @note This function is SYNC function that thread unsafe without strand.
     */
    void regulate_for_store(v5::basic_publish_packet<PacketIdBytes>& packet) const;

    void cancel_all_timers_for_test();

    void set_pingreq_send_interval_ms_for_test(std::size_t ms);

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
    basic_endpoint(
        protocol_version ver,
        Args&&... args
    );

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
        void(system_error)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    send(
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
    add_retry(
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
    void release_pid(packet_id_t pid);

private:
    protocol_version protocol_version_;
    std::shared_ptr<stream_type> stream_;
    packet_id_manager<packet_id_t> pid_man_;
    std::set<packet_id_t> pid_suback_;
    std::set<packet_id_t> pid_unsuback_;
    std::set<packet_id_t> pid_puback_;
    std::set<packet_id_t> pid_pubrec_;
    std::set<packet_id_t> pid_pubcomp_;

    bool need_store_ = false;
    store<PacketIdBytes> store_;

    bool auto_pub_response_ = false;
    bool auto_ping_response_ = false;

    bool auto_map_topic_alias_send_ = false;
    bool auto_replace_topic_alias_send_ = false;
    std::optional<topic_alias_send> topic_alias_send_;
    std::optional<topic_alias_recv> topic_alias_recv_;

    receive_maximum_t publish_send_max_{receive_maximum_max};
    receive_maximum_t publish_recv_max_{receive_maximum_max};
    receive_maximum_t publish_send_count_{0};

    std::set<packet_id_t> publish_recv_;
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

    std::set<packet_id_t> qos2_publish_handled_;

    bool recv_processing_ = false;
    std::set<packet_id_t> qos2_publish_processing_;

    struct tim_cancelled;
    std::deque<tim_cancelled> tim_retry_acq_pid_queue_;
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
