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
#include <async_mqtt/stream.hpp>
#include <async_mqtt/store.hpp>
#include <async_mqtt/log.hpp>
#include <async_mqtt/topic_alias_send.hpp>
#include <async_mqtt/topic_alias_recv.hpp>
#include <async_mqtt/packet_id_manager.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

/// @file

namespace async_mqtt {

/**
 * @brief MQTT endpoint connection role
 */
enum class role {
    client = 0b01, ///< as client. Can't send CONNACK, SUBACK, UNSUBACK, PINGRESP. Can send Other packets.
    server = 0b10, ///< as server. Can't send CONNECT, SUBSCRIBE, UNSUBSCRIBE, PINGREQ, DISCONNECT(only on v3.1.1).
                   ///  Can send Other packets.
    any    = 0b11, ///< can send all packets. (no check)
};

/**
 * @brief receive packet filter
 */
enum class filter {
    match,  ///< matched control_packet_type is target
    except  ///< no matched control_packet_type is target
};

/**
 * @brief MQTT endpoint corresponding to the connection
 * @tparam Role          role for packet sendable checking
 * @tparam PacketIdBytes MQTT spec is 2. You can use `endpoint` for that.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <role Role, std::size_t PacketIdBytes, typename NextLayer>
class basic_endpoint {

    enum class connection_status {
        connecting,
        connected,
        disconnecting,
        disconnected
    };

    static constexpr bool can_send_as_client(role r) {
        return static_cast<int>(r) & static_cast<int>(role::client);
    }

    static constexpr bool can_send_as_server(role r) {
        return static_cast<int>(r) & static_cast<int>(role::server);
    }

    static inline optional<topic_alias_t> get_topic_alias(properties const& props) {
        optional<topic_alias_t> ta_opt;
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
    using stream_type =
        stream<
            NextLayer
        >;

public:
    /// @brief The type given as NextLayer
    using next_layer_type = NextLayer;
    /// @brief The type of stand that is used MQTT stream exclusive control
    using strand_type = typename stream_type::strand_type;
    /// @brief The value given as PacketIdBytes
    static constexpr std::size_t packet_id_bytes = PacketIdBytes;

    /// @brief Type of packet_variant.
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;

    /// @brief Type of MQTT Packet Identifier.
    using packet_id_t = typename packet_id_type<PacketIdBytes>::type;

    /**
     * @brief constructor
     * @tparam Args Types for the next layer
     * @param  ver  MQTT protocol version (v5 or v3_1_1)
     * @param  args args for the next layer. There are predefined next layer types:
     *              \n @link protocol::mqtt @endlink, @link protocol::mqtts @endlink,
     *              @link protocol::ws @endlink, and @link protocol::wss @endlink.
     */
    template <typename... Args>
    basic_endpoint(
        protocol_version ver,
        Args&&... args
    ): protocol_version_{ver},
       stream_{std::make_shared<stream_type>(std::forward<Args>(args)...)}
    {
    }

    /**
     * @brief strand getter
     * @return const reference of the strand
     */
    strand_type const& strand() const {
        return stream_->strand();
    }
    /**
     * @brief strand getter
     * @return eference of the strand
     */
    strand_type& strand() {
        return stream_->strand();
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
    auto const& lowest_layer() const {
        return stream_->lowest_layer();
    }
    /**
     * @brief lowest_layer getter
     * @return reference of the lowest_layer
     */
    auto& lowest_layer() {
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
    void set_ping_resp_recv_timeout_ms(std::size_t ms) {
        if (ms == 0) {
            pingresp_recv_timeout_ms_ = nullopt;
        }
        else {
            pingresp_recv_timeout_ms_.emplace(ms);
        }
    }

    // async functions

    /**
     * @brief acuire unique packet_id.
     * @param token the param is optional<packet_id_t>
     * @return deduced by token
     */
    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(optional<packet_id_t>)>::return_type
    acquire_unique_packet_id(
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "acquire_unique_packet_id";
        return
            as::async_compose<
                CompletionToken,
                void(optional<packet_id_t>)
            >(
                acquire_unique_packet_id_impl{
                    *this
                },
                token
            );
    }

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @param token     the param is bool. If true, success, otherwise the packet_id has already been used.
     * @return deduced by token
     */
    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(bool)>::return_type
    register_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "register_packet_id pid:" << packet_id;
        return
            as::async_compose<
                CompletionToken,
                void(bool)
            >(
                register_packet_id_impl{
                    *this,
                    packet_id
                },
                token
            );
    }

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @param token     the param is void
     * @return deduced by token
     */
    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void()>::return_type
    release_packet_id(
        packet_id_t packet_id,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "release_packet_id pid:" << packet_id;
        return
            as::async_compose<
                CompletionToken,
                void()
            >(
                release_packet_id_impl{
                    *this,
                    packet_id
                },
                token
            );
    }

    /**
     * @brief send packet
     *        users can call send() before the previous send()'s CompletionToken is invoked
     * @param packet packet to send
     * @param token  the param is system_error
     * @return deduced by token
     */
    template <typename Packet, typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(system_error)>::return_type
    send(
        Packet packet,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "send:" << packet;
        if constexpr(!std::is_same_v<Packet, basic_packet_variant<PacketIdBytes>>) {
            static_assert(
                (can_send_as_client(Role) && is_client_sendable<std::decay_t<Packet>>()) ||
                (can_send_as_server(Role) && is_server_sendable<std::decay_t<Packet>>()),
                "Packet cannot be send by MQTT protocol"
            );
        }

        return
            send(
                force_move(packet),
                false, // not from queue
                std::forward<CompletionToken>(token)
            );
    }

    /**
     * @brief receive packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     * @param token the param is packet_variant_type
     * @return deduced by token
     */
    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(packet_variant_type)>::return_type
    recv(
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "recv";
        BOOST_ASSERT(!recv_processing_);
        recv_processing_ = true;
        return
            as::async_compose<
                CompletionToken,
                void(packet_variant_type)
            >(
                recv_impl{
                    *this
                },
                token
            );
    }

    /**
     * @brief receive packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     *        if packet is not filterd, then next recv() starts automatically.
     *        if receive error happenes, then token would be invoked.
     * @param types target control_packet_types
     * @param token the param is packet_variant_type
     * @return deduced by token
     */
    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(packet_variant_type)>::return_type
    recv(
        std::set<control_packet_type> types,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "recv";
        BOOST_ASSERT(!recv_processing_);
        recv_processing_ = true;
        return
            as::async_compose<
                CompletionToken,
                void(packet_variant_type)
            >(
                recv_impl{
                    *this,
                    filter::match,
                    force_move(types)
                },
                token
            );
    }

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
    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(packet_variant_type)>::return_type
    recv(
        filter fil,
        std::set<control_packet_type> types,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "recv";
        BOOST_ASSERT(!recv_processing_);
        recv_processing_ = true;
        return
            as::async_compose<
                CompletionToken,
                void(packet_variant_type)
            >(
                recv_impl{
                    *this,
                    fil,
                    force_move(types)
                },
                token
            );
    }

    /**
     * @brief close the underlying connection
     * @param token  the param is void
     * @return deduced by token
     */
    template<typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void()>::return_type
    close(CompletionToken&& token) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "close";
        return
            as::async_compose<
                CompletionToken,
                void()
            >(
                close_impl{
                    *this
                },
                token
            );
    }

    /**
     * @brief restore packets
     *        the restored packets would automatically send when CONNACK packet is received
     * @param pvs packets to restore
     * @param token  the param is void
     * @return deduced by token
     */
    template <typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void()>::return_type
    restore_packets(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "restore_packets";
        return
            as::async_compose<
                CompletionToken,
                void()
            >(
                restore_packets_impl{
                    *this,
                    force_move(pvs)
                },
                token
            );
    }

    /**
     * @brief get stored packets
     *        sotred packets mean inflight packets.
     *        - PUBLISH packet (QoS1) not received PUBACK packet
     *        - PUBLISH packet (QoS1) not received PUBREC packet
     *        - PUBREL  packet not received PUBCOMP packet
     * @param token  the param is std::vector<basic_store_packet_variant<PacketIdBytes>>
     * @return deduced by token
     */
    template <typename CompletionToken>
    typename as::async_result<
        std::decay_t<CompletionToken>,
        void(std::vector<basic_store_packet_variant<PacketIdBytes>>)
    >::return_type
    get_stored_packets(
        CompletionToken&& token
    ) const {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "get_stored_packets";
        return
            as::async_compose<
                CompletionToken,
                void(std::vector<basic_store_packet_variant<PacketIdBytes>>)
            >(
                get_stored_packets_impl{
                    *this
                },
                token
            );
    }

    template <typename CompletionToken>
    typename as::async_result<
        std::decay_t<CompletionToken>,
        void(v5::basic_publish_packet<PacketIdBytes>)
    >::return_type
    regulate_for_store(
        v5::basic_publish_packet<PacketIdBytes> packet,
        CompletionToken&& token
    ) const {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "regulate_for_store:" << packet;
        return
            as::async_compose<
                CompletionToken,
                void(v5::basic_publish_packet<PacketIdBytes>)
            >(
                regulate_for_store_impl{
                    *this,
                    force_move(packet)
                },
                token
            );
    }

    // sync APIs that require working on strand

    /**
     * @brief acuire unique packet_id.
     * @return optional<packet_id_t> if acquired return acquired packet id, otherwise nullopt
     * @note This function is SYNC function that must only be called in the strand.
     */
    optional<packet_id_t> acquire_unique_packet_id() {
        BOOST_ASSERT(strand().running_in_this_thread());
        auto pid = pid_man_.acquire_unique_id();
        if (pid) {
            ASYNC_MQTT_LOG("mqtt_api", info)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "acquire_unique_packet_id:" << *pid;
        }
        else {
            ASYNC_MQTT_LOG("mqtt_api", info)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "acquire_unique_packet_id:full";
        }
        return pid;
    }

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @return If true, success, otherwise the packet_id has already been used.
     * @note This function is SYNC function that must only be called in the strand.
     */
    bool register_packet_id(packet_id_t pid) {
        BOOST_ASSERT(strand().running_in_this_thread());
        auto ret = pid_man_.register_id(pid);
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "register_packet_id:" << pid << " result:" << ret;
        return ret;
    }

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @note This function is SYNC function that must only be called in the strand.
     */
    void release_packet_id(packet_id_t pid) {
        BOOST_ASSERT(strand().running_in_this_thread());
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "release_packet_id:" << pid;
        pid_man_.release_id(pid);
    }

    /**
     * @brief Get processed but not released QoS2 packet ids
     *        This function should be called after disconnection
     * @return set of packet_ids
     * @note This function is SYNC function that must only be called in the strand.
     */
    std::set<packet_id_t> get_qos2_publish_handled_pids() const {
        BOOST_ASSERT(strand().running_in_this_thread());
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "get_qos2_publish_handled_pids";
        return qos2_publish_handled_;
    }

    /**
     * @brief Restore processed but not released QoS2 packet ids
     *        This function should be called before receive the first publish
     * @param pids packet ids
     * @note This function is SYNC function that must only be called in the strand.
     */
    void restore_qos2_publish_handled_pids(std::set<packet_id_t> pids) {
        BOOST_ASSERT(strand().running_in_this_thread());
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "restore_qos2_publish_handled_pids";
        qos2_publish_handled_ = force_move(pids);
    }

    /**
     * @brief restore packets
     *        the restored packets would automatically send when CONNACK packet is received
     * @param pvs packets to restore
     * @note This function is SYNC function that must only be called in the strand.
     */
    void restore_packets(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
    ) {
        BOOST_ASSERT(strand().running_in_this_thread());
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "restore_packets";
        for (auto& pv : pvs) {
            pv.visit(
                [&](auto& p) {
                    if (pid_man_.register_id(p.packet_id())) {
                        store_.add(force_move(p));
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", error)
                            << ASYNC_MQTT_ADD_VALUE(address, this)
                            << "packet_id:" << p.packet_id()
                            << " has already been used. Skip it";
                    }
                }
            );
        }
    }

    /**
     * @brief get stored packets
     *        sotred packets mean inflight packets.
     *        - PUBLISH packet (QoS1) not received PUBACK packet
     *        - PUBLISH packet (QoS1) not received PUBREC packet
     *        - PUBREL  packet not received PUBCOMP packet
     * @return std::vector<basic_store_packet_variant<PacketIdBytes>>
     * @note This function is SYNC function that must only be called in the strand.
     */
    std::vector<basic_store_packet_variant<PacketIdBytes>> get_stored_packets() const {
        BOOST_ASSERT(strand().running_in_this_thread());
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "get_stored_packets";
        return store_.get_stored();
    }

    /**
     * @brief get MQTT protocol version
     * @return MQTT protocol version
     * @note This function is SYNC function that must only be called in the strand.
     */
    protocol_version get_protocol_version() const {
        BOOST_ASSERT(strand().running_in_this_thread());
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "get_protocol_version:" << protocol_version_;
        return protocol_version_;
    }

    /**
     * @brief Get MQTT PUBLISH packet processing status
     * @param pid packet_id corresponding to the publish packet.
     * @return If the packet is processing, then true, otherwise false.
     * @note This function is SYNC function that must only be called in the strand.
     */
    bool is_publish_processing(packet_id_t pid) const {
        BOOST_ASSERT(strand().running_in_this_thread());
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "is_publish_processing:" << pid;
        return qos2_publish_processing_.find(pid) != qos2_publish_processing_.end();
    }

    /**
     * @brief Regulate publish packet for store
     *        If topic is empty, extract topic from topic alias, and remove topic alias
     *        Otherwise, remove topic alias if exists.
     * @param packet packet to regulate
     * @note This function is SYNC function that must only be called in the strand.
     */
    void regulate_for_store(v5::basic_publish_packet<PacketIdBytes>& packet) const {
        BOOST_ASSERT(strand().running_in_this_thread());
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "regulate_for_store:" << packet;
        if (packet.topic().empty()) {
            if (auto ta_opt = get_topic_alias(packet.props())) {
                auto topic = topic_alias_send_->find_without_touch(*ta_opt);
                if (!topic.empty()) {
                    packet.remove_topic_alias_add_topic(allocate_buffer(topic));
                }
            }
        }
        else {
            packet.remove_topic_alias();
        }
    }

    void cancel_all_timers_for_test() {
        BOOST_ASSERT(strand().running_in_this_thread());
        tim_pingreq_send_.cancel();
        tim_pingreq_recv_.cancel();
        tim_pingresp_recv_.cancel();
    }

    void set_pingreq_send_interval_ms_for_test(std::size_t ms) {
        BOOST_ASSERT(strand().running_in_this_thread());
        pingreq_send_interval_ms_ = ms;
    }

private: // compose operation impl

    struct acquire_unique_packet_id_impl {
        this_type& ep;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = complete;
                auto& a_ep{ep};
                as::dispatch(
                    a_ep.strand(),
                    force_move(self)
                );
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ep.pid_man_.acquire_unique_id());
                break;
            }
        }
    };

    struct register_packet_id_impl {
        this_type& ep;
        packet_id_t packet_id;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = complete;
                auto& a_ep{ep};
                as::dispatch(
                    a_ep.strand(),
                    force_move(self)
                );
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ep.pid_man_.register_id(packet_id));
                break;
            }
        }
    };

    struct release_packet_id_impl {
        this_type& ep;
        packet_id_t packet_id;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = complete;
                auto& a_ep{ep};
                as::dispatch(
                    a_ep.strand(),
                    force_move(self)
                );
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                ep.pid_man_.release_id(packet_id);
                self.complete();
                break;
            }
        }
    };


    template <typename Packet>
    struct send_impl {
        this_type& ep;
        Packet packet;
        bool from_queue = false;
        enum { dispatch, write, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec = error_code{},
            std::size_t /*bytes_transferred*/ = 0
        ) {
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "send error:" << ec.message();
                self.complete(ec);
                return;
            }

            switch (state) {
            case dispatch: {
                state = write;
                auto& a_ep{ep};
                as::dispatch(
                    a_ep.strand(),
                    force_move(self)
                );
            } break;
            case write: {
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                state = complete;
                if constexpr(std::is_same_v<std::decay_t<Packet>, basic_packet_variant<PacketIdBytes>>) {
                    packet.visit(
                        overload {
                            [&](auto& actual_packet) {
                                if (process_send_packet(self, actual_packet)) {
                                    auto& a_ep{ep};
                                    a_ep.stream_->write_packet(
                                        force_move(actual_packet),
                                        force_move(self)
                                    );
                                    if constexpr(is_connack<std::remove_reference_t<decltype(actual_packet)>>()) {
                                        // server send connack after connack sent
                                        a_ep.send_stored();
                                    }
                                    if constexpr(Role == role::client) {
                                        a_ep.reset_pingreq_send_timer();
                                    }
                                }
                            },
                            [&](system_error&) {}
                        }
                    );
                }
                else {
                    if (process_send_packet(self, packet)) {
                        auto& a_ep{ep};
                        auto& a_packet{packet};
                        a_ep.stream_->write_packet(
                            force_move(a_packet),
                            force_move(self)
                        );
                        if constexpr(is_connack<Packet>()) {
                            // server send connack after connack sent
                            a_ep.send_stored();
                        }
                        if constexpr(Role == role::client) {
                            a_ep.reset_pingreq_send_timer();
                        }
                    }
                }
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ec);
                break;
            }
        }

        template <typename Self, typename ActualPacket>
        bool process_send_packet(Self& self, ActualPacket& actual_packet) {
            // MQTT protocol sendable packet check
            if ((can_send_as_client(Role) && !is_client_sendable<std::decay_t<ActualPacket>>()) ||
                (can_send_as_server(Role) && !is_server_sendable<std::decay_t<ActualPacket>>())
            ) {
                self.complete(
                    make_error(
                        errc::protocol_error,
                        "Packet cannot be send by MQTT protocol"
                    )
                );
                return false;
            }

            // connection status check
            if constexpr(is_connect<ActualPacket>()) {
                if (ep.status_ != connection_status::disconnected) {
                self.complete(
                        make_error(
                            errc::protocol_error,
                            "connect_packet can only be send on connection_status::disconnected"
                        )
                    );
                    return false;
                }
            }
            else if constexpr(is_connack<ActualPacket>()) {
                if (ep.status_ != connection_status::connecting) {
                    self.complete(
                        make_error(
                            errc::protocol_error,
                            "connack_packet can only be send on connection_status::connecting"
                        )
                    );
                    return false;
                }
            }
            else if constexpr(std::is_same_v<v5::auth_packet, Packet>) {
                if (ep.status_ != connection_status::connected &&
                    ep.status_ != connection_status::connecting) {
                    self.complete(
                        make_error(
                            errc::protocol_error,
                            "auth packet can only be send on connection_status::connecting or status::connected"
                        )
                    );
                    return false;
                }
            }
            else {
                if (ep.status_ != connection_status::connected) {
                    if constexpr(!is_publish<std::decay_t<ActualPacket>>()) {
                        self.complete(
                            make_error(
                                errc::protocol_error,
                                "packet can only be send on connection_status::connected"
                            )
                        );
                        return false;
                    }
                }
            }

            // sending process
            bool topic_alias_validated = false;

            if constexpr(std::is_same_v<v3_1_1::connect_packet, std::decay_t<ActualPacket>>) {
                ep.initialize();
                ep.status_ = connection_status::connecting;
                auto keep_alive = actual_packet.keep_alive();
                if (keep_alive != 0 && !ep.pingreq_send_interval_ms_) {
                    ep.pingreq_send_interval_ms_.emplace(keep_alive * 1000);
                }
                if (actual_packet.clean_session()) {
                    ep.pid_man_.clear();
                    ep.store_.clear();
                    ep.need_store_ = false;
                }
                else {
                    ep.need_store_ = true;
                }
                ep.topic_alias_send_ = nullopt;
            }

            if constexpr(std::is_same_v<v5::connect_packet, std::decay_t<ActualPacket>>) {
                ep.initialize();
                ep.status_ = connection_status::connecting;
                auto keep_alive = actual_packet.keep_alive();
                if (keep_alive != 0 && !ep.pingreq_send_interval_ms_) {
                    ep.pingreq_send_interval_ms_.emplace(keep_alive * 1000);
                }
                if (actual_packet.clean_start()) {
                    ep.pid_man_.clear();
                    ep.store_.clear();
                }
                for (auto const& prop : actual_packet.props()) {
                    prop.visit(
                        overload {
                            [&](property::topic_alias_maximum const& p) {
                                if (p.val() != 0) {
                                    ep.topic_alias_recv_.emplace(p.val());
                                }
                            },
                            [&](property::receive_maximum const& p) {
                                BOOST_ASSERT(p.val() != 0);
                                ep.publish_recv_max_ = p.val();
                            },
                            [&](property::maximum_packet_size const& p) {
                                BOOST_ASSERT(p.val() != 0);
                                ep.maximum_packet_size_recv_ = p.val();
                            },
                            [&](property::session_expiry_interval const& p) {
                                if (p.val() != 0) {
                                    ep.need_store_ = true;
                                }
                            },
                            [](auto const&){}
                        }
                    );
                }
            }

            if constexpr(std::is_same_v<v3_1_1::connack_packet, std::decay_t<ActualPacket>>) {
                if (actual_packet.code() == connect_return_code::accepted) {
                    ep.status_ = connection_status::connected;
                }
                else {
                    ep.status_ = connection_status::disconnecting;
                }
            }

            if constexpr(std::is_same_v<v5::connack_packet, std::decay_t<ActualPacket>>) {
                if (actual_packet.code() == connect_reason_code::success) {
                    ep.status_ = connection_status::connected;
                    for (auto const& prop : actual_packet.props()) {
                        prop.visit(
                            overload {
                                [&](property::topic_alias_maximum const& p) {
                                    if (p.val() != 0) {
                                        ep.topic_alias_recv_.emplace(p.val());
                                    }
                                },
                                [&](property::receive_maximum const& p) {
                                    BOOST_ASSERT(p.val() != 0);
                                    ep.publish_recv_max_ = p.val();
                                },
                                [&](property::maximum_packet_size const& p) {
                                    BOOST_ASSERT(p.val() != 0);
                                    ep.maximum_packet_size_recv_ = p.val();
                                },
                                [](auto const&){}
                            }
                        );
                    }
                }
                else {
                    ep.status_ = connection_status::disconnecting;
                }
            }

            // store publish/pubrel packet
            if constexpr(is_publish<std::decay_t<ActualPacket>>()) {
                if (actual_packet.opts().get_qos() == qos::at_least_once ||
                    actual_packet.opts().get_qos() == qos::exactly_once
                ) {
                    BOOST_ASSERT(ep.pid_man_.is_used_id(actual_packet.packet_id()));
                    if (ep.need_store_) {
                        if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
                            auto ta_opt = get_topic_alias(actual_packet.props());
                            if (actual_packet.topic().empty()) {
                                auto topic_opt = validate_topic_alias(self, ta_opt);
                                if (!topic_opt) {
                                    auto packet_id = actual_packet.packet_id();
                                    if (packet_id != 0) {
                                        ep.pid_man_.release_id(packet_id);
                                    }
                                    return false;
                                }
                                topic_alias_validated = true;
                                auto props = actual_packet.props();
                                auto it = props.cbegin();
                                auto end = props.cend();
                                for (; it != end; ++it) {
                                    if (it->id() == property::id::topic_alias) {
                                        props.erase(it);
                                        break;
                                    }
                                }

                                auto store_packet =
                                    ActualPacket(
                                        actual_packet.packet_id(),
                                        allocate_buffer(*topic_opt),
                                        actual_packet.payload(),
                                        actual_packet.opts(),
                                        force_move(props)
                                    );
                                if (!validate_maximum_packet_size(self, store_packet)) {
                                    auto packet_id = actual_packet.packet_id();
                                    if (packet_id != 0) {
                                        ep.pid_man_.release_id(packet_id);
                                    }
                                    return false;
                                }
                                // add new packet that doesn't have topic_aliass to store
                                // the original packet still use topic alias to send
                                store_packet.set_dup(true);
                                ep.store_.add(force_move(store_packet));
                            }
                            else {
                                auto props = actual_packet.props();
                                auto it = props.cbegin();
                                auto end = props.cend();
                                for (; it != end; ++it) {
                                    if (it->id() == property::id::topic_alias) {
                                        props.erase(it);
                                        break;
                                    }
                                }

                                auto store_packet =
                                    ActualPacket(
                                        actual_packet.packet_id(),
                                        actual_packet.topic(),
                                        actual_packet.payload(),
                                        actual_packet.opts(),
                                        force_move(props)
                                    );
                                if (!validate_maximum_packet_size(self, store_packet)) {
                                    auto packet_id = actual_packet.packet_id();
                                    if (packet_id != 0) {
                                        ep.pid_man_.release_id(packet_id);
                                    }
                                    return false;
                                }
                                store_packet.set_dup(true);
                                ep.store_.add(force_move(store_packet));
                            }
                        }
                        else {
                            if (!validate_maximum_packet_size(self, actual_packet)) {
                                auto packet_id = actual_packet.packet_id();
                                if (packet_id != 0) {
                                    ep.pid_man_.release_id(packet_id);
                                }
                                return false;
                            }
                            auto store_packet{actual_packet};
                            store_packet.set_dup(true);
                            ep.store_.add(force_move(store_packet));
                        }
                    }
                    if (actual_packet.opts().get_qos() == qos::exactly_once) {
                        ep.qos2_publish_processing_.insert(actual_packet.packet_id());
                    }
                }
            }

            if constexpr(is_instance_of<v5::basic_publish_packet, std::decay_t<ActualPacket>>::value) {
                // apply topic_alias
                auto ta_opt = get_topic_alias(actual_packet.props());
                if (actual_packet.topic().empty()) {
                    if (!topic_alias_validated &&
                        !validate_topic_alias(self, ta_opt)) {
                        auto packet_id = actual_packet.packet_id();
                        if (packet_id != 0) {
                            ep.pid_man_.release_id(packet_id);
                        }
                        return false;
                    }
                    // use topic_alias set by user
                }
                else {
                    if (ta_opt) {
                        if (validate_topic_alias_range(self, *ta_opt)) {
                            ASYNC_MQTT_LOG("mqtt_impl", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                << "topia alias : "
                                << actual_packet.topic() << " - " << *ta_opt
                                << " is registered." ;
                            ep.topic_alias_send_->insert_or_update(actual_packet.topic(), *ta_opt);
                        }
                        else {
                            auto packet_id = actual_packet.packet_id();
                            if (packet_id != 0) {
                                ep.pid_man_.release_id(packet_id);
                            }
                            return false;
                        }
                    }
                    else if (ep.auto_map_topic_alias_send_) {
                        if (ep.topic_alias_send_) {
                            if (auto ta_opt = ep.topic_alias_send_->find(actual_packet.topic())) {
                                ASYNC_MQTT_LOG("mqtt_impl", trace)
                                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                    << "topia alias : " << actual_packet.topic() << " - " << *ta_opt
                                    << " is found." ;
                                actual_packet.remove_topic_add_topic_alias(*ta_opt);
                            }
                            else {
                                auto lru_ta = ep.topic_alias_send_->get_lru_alias();
                                ep.topic_alias_send_->insert_or_update(actual_packet.topic(), lru_ta); // remap topic alias
                                actual_packet.add_topic_alias(lru_ta);
                            }
                        }
                    }
                    else if (ep.auto_replace_topic_alias_send_) {
                        if (ep.topic_alias_send_) {
                            if (auto ta_opt = ep.topic_alias_send_->find(actual_packet.topic())) {
                                ASYNC_MQTT_LOG("mqtt_impl", trace)
                                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                    << "topia alias : " << actual_packet.topic() << " - " << *ta_opt
                                    << " is found." ;
                                actual_packet.remove_topic_add_topic_alias(*ta_opt);
                            }
                        }
                    }
                }

                // receive_maximum for sending
                if (!from_queue && ep.enqueue_publish(actual_packet)) {
                    self.complete(
                        make_error(
                            errc::success,
                            "publish_packet is enqueued due to receive_maximum for sending"
                        )
                    );
                    return false;
                }
            }

            if constexpr(is_instance_of<v5::basic_puback_packet, std::decay_t<ActualPacket>>::value) {
                ep.publish_recv_.erase(actual_packet.packet_id());
            }


            if constexpr(is_instance_of<v5::basic_pubrec_packet, std::decay_t<ActualPacket>>::value) {
                if (is_error(actual_packet.code())) {
                    ep.publish_recv_.erase(actual_packet.packet_id());
                    ep.qos2_publish_handled_.erase(actual_packet.packet_id());
                }
            }

            if constexpr(is_pubrel<std::decay_t<ActualPacket>>()) {
                BOOST_ASSERT(ep.pid_man_.is_used_id(actual_packet.packet_id()));
                if (ep.need_store_) ep.store_.add(actual_packet);
            }

            if constexpr(is_instance_of<v5::basic_pubcomp_packet, std::decay_t<ActualPacket>>::value) {
                ep.publish_recv_.erase(actual_packet.packet_id());
            }

            if constexpr(is_subscribe<std::decay_t<ActualPacket>>()) {
                BOOST_ASSERT(ep.pid_man_.is_used_id(actual_packet.packet_id()));
            }

            if constexpr(is_unsubscribe<std::decay_t<ActualPacket>>()) {
                BOOST_ASSERT(ep.pid_man_.is_used_id(actual_packet.packet_id()));
            }

            if constexpr(is_pingreq<std::decay_t<ActualPacket>>()) {
                ep.reset_pingresp_recv_timer();
            }

            if constexpr(is_disconnect<std::decay_t<ActualPacket>>()) {
                ep.status_ = connection_status::disconnecting;
            }

            if (!validate_maximum_packet_size(self, actual_packet)) {
                if constexpr(own_packet_id<std::decay_t<ActualPacket>>()) {
                    auto packet_id = actual_packet.packet_id();
                    if (packet_id != 0) {
                        ep.pid_man_.release_id(packet_id);
                    }
                }
                return false;
            }

            if constexpr(is_publish<std::decay_t<ActualPacket>>()) {
                if (ep.status_ != connection_status::connected) {
                    // offline publish
                    self.complete(
                        make_error(
                            errc::success,
                            "packet is stored but not sent"
                        )
                    );
                    return false;
                }
            }

            return true;
        }

        template <typename Self>
        bool validate_topic_alias_range(Self& self, topic_alias_t ta) {
            if (!ep.topic_alias_send_) {
                self.complete(
                    make_error(
                        errc::bad_message,
                        "topic_alias is set but topic_alias_maximum is 0"
                    )
                );
                return false;
            }
            if (ta == 0 || ta > ep.topic_alias_send_->max()) {
                self.complete(
                    make_error(
                        errc::bad_message,
                        "topic_alias is set but out of range"
                    )
                );
                return false;
            }
            return true;
        }

        template <typename Self>
        optional<std::string> validate_topic_alias(Self& self, optional<topic_alias_t> ta_opt) {
            BOOST_ASSERT(ep.strand().running_in_this_thread());
            if (!ta_opt) {
                self.complete(
                    make_error(
                        errc::bad_message,
                        "topic is empty but topic_alias isn't set"
                    )
                );
                return nullopt;
            }

            if (!validate_topic_alias_range(self, *ta_opt)) {
                return nullopt;
            }

            auto topic = ep.topic_alias_send_->find(*ta_opt);
            if (topic.empty()) {
                self.complete(
                    make_error(
                        errc::bad_message,
                        "topic is empty but topic_alias is not registered"
                    )
                );
                return nullopt;
            }
            return topic;
        }

        template <typename Self, typename PacketArg>
        bool validate_maximum_packet_size(Self& self, PacketArg const& packet_arg) {
            if (packet_arg.size() > ep.maximum_packet_size_send_) {
                ASYNC_MQTT_LOG("mqtt_impl", error)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "packet size over maximum_packet_size for sending";
                self.complete(
                    make_error(
                        errc::bad_message,
                        "packet size is over maximum_packet_size for sending"
                    )
                );
                return false;
            }
            return true;
        }
    };

    struct recv_impl {
        this_type& ep;
        optional<filter> fil = nullopt;
        std::set<control_packet_type> types = {};
        optional<system_error> decided_error = nullopt;
        enum { initiate, disconnect, close, complete } state = initiate;

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec = error_code{},
            buffer buf = buffer{}
        ) {
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "recv error:" << ec.message();
                decided_error.emplace(ec);
                ep.status_ = connection_status::disconnected;
                ep.recv_processing_ = false;
                state = close;
                auto& a_ep{ep};
                a_ep.close(force_move(self));
                return;
            }

            switch (state) {
            case initiate: {
                state = complete;
                auto& a_ep{ep};
                a_ep.stream_->read_packet(force_move(self));
            } break;
            case complete: {
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                if (buf.size() > ep.maximum_packet_size_recv_) {
                    // on v3.1.1 maximum_packet_size_recv_ is initialized as packet_size_no_limit
                    BOOST_ASSERT(ep.protocol_version_ == protocol_version::v5);
                    state = disconnect;
                    decided_error.emplace(
                        make_error(
                            errc::bad_message,
                            "too large packet received"
                        )
                    );
                    auto& a_ep{ep};
                    a_ep.send(
                        v5::disconnect_packet{
                            disconnect_reason_code::packet_too_large
                        },
                        force_move(self)
                    );
                    return;
                }

                auto v = buffer_to_basic_packet_variant<PacketIdBytes>(buf, ep.protocol_version_);
                ASYNC_MQTT_LOG("mqtt_impl", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "recv:" << v;
                bool call_complete = true;
                v.visit(
                    // do internal protocol processing
                    overload {
                        [&](v3_1_1::connect_packet& p) {
                            ep.initialize();
                            ep.protocol_version_ = protocol_version::v3_1_1;
                            ep.status_ = connection_status::connecting;
                            auto keep_alive = p.keep_alive();
                            if (keep_alive != 0) {
                                ep.pingreq_recv_timeout_ms_.emplace(keep_alive * 1000 * 3 / 2);
                            }
                            if (p.clean_session()) {
                                ep.need_store_ = false;
                            }
                            else {
                                ep.need_store_ = true;
                            }
                        },
                        [&](v5::connect_packet& p) {
                            ep.initialize();
                            ep.protocol_version_ = protocol_version::v5;
                            ep.status_ = connection_status::connecting;
                            auto keep_alive = p.keep_alive();
                            if (keep_alive != 0) {
                                ep.pingreq_recv_timeout_ms_.emplace(keep_alive * 1000 * 3 / 2);
                            }
                            for (auto const& prop : p.props()) {
                                prop.visit(
                                    overload {
                                        [&](property::topic_alias_maximum const& p) {
                                            if (p.val() > 0) {
                                                ep.topic_alias_send_.emplace(p.val());
                                            }
                                        },
                                        [&](property::receive_maximum const& p) {
                                            BOOST_ASSERT(p.val() != 0);
                                            ep.publish_send_max_ = p.val();
                                        },
                                        [&](property::maximum_packet_size const& p) {
                                            BOOST_ASSERT(p.val() != 0);
                                            ep.maximum_packet_size_send_ = p.val();
                                        },
                                        [&](property::session_expiry_interval const& p) {
                                            if (p.val() != 0) {
                                                ep.need_store_ = true;
                                            }
                                        },
                                        [](auto const&) {
                                        }
                                    }
                                );
                            }
                        },
                        [&](v3_1_1::connack_packet& p) {
                            if (p.code() == connect_return_code::accepted) {
                                ep.status_ = connection_status::connected;
                                if (p.session_present()) {
                                    ep.send_stored();
                                }
                                else {
                                    ep.pid_man_.clear();
                                    ep.store_.clear();
                                }
                            }
                        },
                        [&](v5::connack_packet& p) {
                            if (p.code() == connect_reason_code::success) {
                                ep.status_ = connection_status::connected;

                                for (auto const& prop : p.props()) {
                                    prop.visit(
                                        overload {
                                            [&](property::topic_alias_maximum const& p) {
                                                if (p.val() > 0) {
                                                    ep.topic_alias_send_.emplace(p.val());
                                                }
                                            },
                                            [&](property::receive_maximum const& p) {
                                                BOOST_ASSERT(p.val() != 0);
                                                ep.publish_send_max_ = p.val();
                                            },
                                            [&](property::maximum_packet_size const& p) {
                                                BOOST_ASSERT(p.val() != 0);
                                                ep.maximum_packet_size_send_ = p.val();
                                            },
                                            [](auto const&) {
                                            }
                                        }
                                    );
                                }

                                if (p.session_present()) {
                                    ep.send_stored();
                                }
                                else {
                                    ep.pid_man_.clear();
                                    ep.store_.clear();
                                }
                            }
                        },
                        [&](v3_1_1::basic_publish_packet<PacketIdBytes>& p) {
                            switch (p.opts().get_qos()) {
                            case qos::at_least_once: {
                                if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                    ep.send(
                                        v3_1_1::basic_puback_packet<PacketIdBytes>(p.packet_id()),
                                        [](system_error const&){}
                                    );
                                }
                            } break;
                            case qos::exactly_once:
                                call_complete = process_qos2_publish(self, protocol_version::v3_1_1, p.packet_id());
                                break;
                            default:
                                break;
                            }
                        },
                        [&](v5::basic_publish_packet<PacketIdBytes>& p) {
                            switch (p.opts().get_qos()) {
                            case qos::at_least_once: {
                                if (ep.publish_recv_.size() == ep.publish_recv_max_) {
                                    state = disconnect;
                                    decided_error.emplace(
                                        make_error(
                                            errc::bad_message,
                                            "receive maximum exceeded"
                                        )
                                    );
                                    auto& a_ep{ep};
                                    a_ep.send(
                                        v5::disconnect_packet{
                                            disconnect_reason_code::receive_maximum_exceeded
                                        },
                                        force_move(self)
                                    );
                                    return;
                                }
                                auto packet_id = p.packet_id();
                                ep.publish_recv_.insert(packet_id);
                                if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                    ep.send(
                                        v5::basic_puback_packet<PacketIdBytes>{packet_id},
                                        [](system_error const&){}
                                    );
                                }
                            } break;
                            case qos::exactly_once: {
                                if (ep.publish_recv_.size() == ep.publish_recv_max_) {
                                    state = disconnect;
                                    decided_error.emplace(
                                        make_error(
                                            errc::bad_message,
                                            "receive maximum exceeded"
                                        )
                                    );
                                    auto& a_ep{ep};
                                    a_ep.send(
                                        v5::disconnect_packet{
                                            disconnect_reason_code::receive_maximum_exceeded
                                        },
                                        force_move(self)
                                    );
                                    return;
                                }
                                auto packet_id = p.packet_id();
                                ep.publish_recv_.insert(packet_id);
                                call_complete = process_qos2_publish(self, protocol_version::v5, packet_id);
                            } break;
                            default:
                                break;
                            }

                            if (p.topic().empty()) {
                                if (auto ta_opt = get_topic_alias(p.props())) {
                                    // extract topic from topic_alias
                                    if (*ta_opt == 0 ||
                                        *ta_opt > ep.topic_alias_recv_->max()) {
                                        state = disconnect;
                                        decided_error.emplace(
                                            make_error(
                                                errc::bad_message,
                                                "topic alias invalid"
                                            )
                                        );
                                        auto& a_ep{ep};
                                        a_ep.send(
                                            v5::disconnect_packet{
                                                disconnect_reason_code::topic_alias_invalid
                                            },
                                            force_move(self)
                                        );
                                        return;
                                    }
                                    else {
                                        auto topic = ep.topic_alias_recv_->find(*ta_opt);
                                        if (topic.empty()) {
                                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                                << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                                << "no matching topic alias: "
                                                << *ta_opt;
                                            state = disconnect;
                                            decided_error.emplace(
                                                make_error(
                                                    errc::bad_message,
                                                    "topic alias invalid"
                                                )
                                            );
                                            auto& a_ep{ep};
                                            a_ep.send(
                                                v5::disconnect_packet{
                                                    disconnect_reason_code::topic_alias_invalid
                                                },
                                                force_move(self)
                                            );
                                            return;
                                        }
                                        else {
                                            p.add_topic(allocate_buffer(topic));
                                        }
                                    }
                                }
                                else {
                                    ASYNC_MQTT_LOG("mqtt_impl", error)
                                        << ASYNC_MQTT_ADD_VALUE(address, &ep)
                                        << "topic is empty but topic_alias isn't set";
                                    state = disconnect;
                                    decided_error.emplace(
                                        make_error(
                                            errc::bad_message,
                                            "topic alias invalid"
                                        )
                                    );
                                    auto& a_ep{ep};
                                    a_ep.send(
                                        v5::disconnect_packet{
                                            disconnect_reason_code::topic_alias_invalid
                                        },
                                        force_move(self)
                                    );
                                    return;
                                }
                            }
                            else {
                                if (auto ta_opt = get_topic_alias(p.props())) {
                                    if (*ta_opt == 0 ||
                                        *ta_opt > ep.topic_alias_recv_->max()) {
                                        state = disconnect;
                                        decided_error.emplace(
                                            make_error(
                                                errc::bad_message,
                                                "topic alias invalid"
                                            )
                                        );
                                        auto& a_ep{ep};
                                        a_ep.send(
                                            v5::disconnect_packet{
                                                disconnect_reason_code::topic_alias_invalid
                                            },
                                            force_move(self)
                                        );
                                        return;
                                    }
                                    else {
                                        // extract topic from topic_alias
                                        if (ep.topic_alias_recv_) {
                                            ep.topic_alias_recv_->insert_or_update(p.topic(), *ta_opt);
                                        }
                                    }
                                }
                            }
                        },
                        [&](v3_1_1::basic_puback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v3_1_1_puback, packet_id);
                            ep.pid_man_.release_id(packet_id);
                        },
                        [&](v5::basic_puback_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v5_puback, packet_id);
                            ep.pid_man_.release_id(packet_id);
                            --ep.publish_send_count_;
                            send_publish_from_queue();
                        },
                        [&](v3_1_1::basic_pubrec_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v3_1_1_pubrec, packet_id);
                            if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                ep.send(
                                    v3_1_1::basic_pubrel_packet<PacketIdBytes>(packet_id),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v5::basic_pubrec_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v5_pubrec, packet_id);
                            if (is_error(p.code())) {
                                ep.pid_man_.release_id(packet_id);
                                ep.qos2_publish_processing_.erase(packet_id);
                                --ep.publish_send_count_;
                                send_publish_from_queue();
                            }
                            else if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                ep.send(
                                    v5::basic_pubrel_packet<PacketIdBytes>(packet_id),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubrel_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.qos2_publish_handled_.erase(packet_id);
                            if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                ep.send(
                                    v3_1_1::basic_pubcomp_packet<PacketIdBytes>(packet_id),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v5::basic_pubrel_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.qos2_publish_handled_.erase(packet_id);
                            if (ep.auto_pub_response_ && ep.status_ == connection_status::connected) {
                                ep.send(
                                    v5::basic_pubcomp_packet<PacketIdBytes>(packet_id),
                                    [](system_error const&){}
                                );
                            }
                        },
                        [&](v3_1_1::basic_pubcomp_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v3_1_1_pubcomp, packet_id);
                            ep.pid_man_.release_id(packet_id);
                            ep.qos2_publish_processing_.erase(packet_id);
                            --ep.publish_send_count_;
                            send_publish_from_queue();
                        },
                        [&](v5::basic_pubcomp_packet<PacketIdBytes>& p) {
                            auto packet_id = p.packet_id();
                            ep.store_.erase(response_packet::v5_pubcomp, packet_id);
                            ep.pid_man_.release_id(packet_id);
                            ep.qos2_publish_processing_.erase(packet_id);
                        },
                        [&](v3_1_1::basic_subscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v5::basic_subscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v3_1_1::basic_suback_packet<PacketIdBytes>& p) {
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v5::basic_suback_packet<PacketIdBytes>& p) {
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v3_1_1::basic_unsubscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v5::basic_unsubscribe_packet<PacketIdBytes>&) {
                        },
                        [&](v3_1_1::basic_unsuback_packet<PacketIdBytes>& p) {
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v5::basic_unsuback_packet<PacketIdBytes>& p) {
                            ep.pid_man_.release_id(p.packet_id());
                        },
                        [&](v3_1_1::pingreq_packet&) {
                            if constexpr(can_send_as_server(Role)) {
                                if (ep.auto_ping_response_ && ep.status_ == connection_status::connected) {
                                    ep.send(
                                        v3_1_1::pingresp_packet(),
                                        [](system_error const&){}
                                    );
                                }
                            }
                        },
                        [&](v5::pingreq_packet&) {
                            if constexpr(can_send_as_server(Role)) {
                                if (ep.auto_ping_response_ && ep.status_ == connection_status::connected) {
                                    ep.send(
                                        v5::pingresp_packet(),
                                        [](system_error const&){}
                                    );
                                }
                            }
                        },
                        [&](v3_1_1::pingresp_packet&) {
                            ep.tim_pingresp_recv_.cancel();
                        },
                        [&](v5::pingresp_packet&) {
                            ep.tim_pingresp_recv_.cancel();
                        },
                        [&](v3_1_1::disconnect_packet&) {
                            ep.status_ = connection_status::disconnecting;
                        },
                        [&](v5::disconnect_packet&) {
                            ep.status_ = connection_status::disconnecting;
                        },
                        [&](v5::auth_packet&) {
                        },
                        [&](system_error&) {
                            ep.status_ = connection_status::disconnected;
                        }
                    }
                );
                ep.reset_pingreq_recv_timer();
                ep.recv_processing_ = false;

                if (fil) {
                    if (auto type_opt = v.type()) {
                        if ((*fil == filter::match  && types.find(*type_opt) == types.end()) ||
                            (*fil == filter::except && types.find(*type_opt) != types.end())
                        ) {
                            // read the next packet
                            state = initiate;
                            auto& a_ep{ep};
                            as::dispatch(
                                a_ep.strand(),
                                force_move(self)
                            );
                        }
                        else {
                            if (call_complete && !decided_error) self.complete(force_move(v));
                        }
                    }
                    else {
                        if (call_complete && !decided_error) self.complete(force_move(v));
                    }
                }
                else {
                    if (call_complete && !decided_error) self.complete(force_move(v));
                }
            } break;
            case close:
                BOOST_ASSERT(decided_error);
                ep.recv_processing_ = false;
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << ASYNC_MQTT_ADD_VALUE(address, &ep)
                    << "recv error:" << decided_error->what();
                self.complete(*decided_error);
                break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            system_error const&
        ) {
            BOOST_ASSERT(state == disconnect);
            state = close;
            auto& a_ep{ep};
            a_ep.close(force_move(self));
        }

        void send_publish_from_queue() {
            BOOST_ASSERT(ep.strand().running_in_this_thread());
            if (ep.status_ != connection_status::connected) return;
            while (!ep.publish_queue_.empty() &&
                   ep.publish_send_count_ != ep.publish_send_max_) {
                ep.send(
                    force_move(ep.publish_queue_.front()),
                    true, // from queue
                    [](system_error const&){}
                );
                ep.publish_queue_.pop_front();
            }
        }

        template <typename Self>
        bool process_qos2_publish(
            Self& self,
            protocol_version ver,
            packet_id_t packet_id
        ) {
            BOOST_ASSERT(ep.strand().running_in_this_thread());
            bool already_handled = false;
            if (ep.qos2_publish_handled_.find(packet_id) == ep.qos2_publish_handled_.end()) {
                ep.qos2_publish_handled_.emplace(packet_id);
            }
            else {
                already_handled = true;
            }
            if (ep.status_ == connection_status::connected &&
                (ep.auto_pub_response_ ||
                 already_handled) // already_handled is true only if the pubrec packet
            ) {                   // corresponding to the publish packet has already
                                  // been sent as success
                switch (ver) {
                case protocol_version::v3_1_1:
                    ep.send(
                        v3_1_1::basic_pubrec_packet<PacketIdBytes>(packet_id),
                        [](system_error const&){}
                    );
                    break;
                case protocol_version::v5:
                    ep.send(
                        v5::basic_pubrec_packet<PacketIdBytes>(packet_id),
                        [](system_error const&){}
                    );
                    break;
                default:
                    BOOST_ASSERT(false);
                    break;
                }
            }
            if (already_handled) {
                // do the next read
                auto& a_ep{ep};
                a_ep.stream_->read_packet(force_move(self));
                return false;
            }
            return true;
        }
    };

    struct close_impl {
        this_type& ep;
        enum { initiate, complete } state = initiate;

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& = error_code{}
        ) {
            switch (state) {
            case initiate: {
                state = complete;
                auto& a_ep{ep};
                a_ep.stream_->close(force_move(self));
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                ep.tim_pingreq_send_.cancel();
                ep.tim_pingreq_recv_.cancel();
                ep.tim_pingresp_recv_.cancel();
                ep.status_ = connection_status::disconnected;
                self.complete();
                break;
            }
        }
    };

    struct restore_packets_impl {
        this_type& ep;
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = complete;
                auto& a_ep{ep};
                as::dispatch(
                    a_ep.strand(),
                    force_move(self)
                );
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                ep.restore_packets(force_move(pvs));
                self.complete();
                break;
            }
        }
    };

    struct get_stored_packets_impl {
        this_type const& ep;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = complete;
                auto& a_ep{ep};
                as::dispatch(
                    a_ep.strand(),
                    force_move(self)
                );
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                self.complete(ep.get_stored_packets());
                break;
            }
        }
    };

    struct regulate_for_store_impl {
        this_type const& ep;
        v5::basic_publish_packet<PacketIdBytes> packet;
        enum { dispatch, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = complete;
                auto& a_ep{ep};
                as::dispatch(
                    a_ep.strand(),
                    force_move(self)
                );
            } break;
            case complete:
                BOOST_ASSERT(ep.strand().running_in_this_thread());
                ep.regulate_for_store(packet);
                self.complete(force_move(packet));
                break;
            }
        }
    };

private:

    template <typename Packet, typename CompletionToken>
    typename as::async_result<std::decay_t<CompletionToken>, void(system_error)>::return_type
    send(
        Packet packet,
        bool from_queue,
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(system_error)
            >(
                send_impl<Packet>{
                    *this,
                    force_move(packet),
                    from_queue
                },
                token
            );
    }

    bool enqueue_publish(v5::basic_publish_packet<PacketIdBytes>& packet) {
        BOOST_ASSERT(strand().running_in_this_thread());
        if (packet.opts().get_qos() == qos::at_least_once ||
            packet.opts().get_qos() == qos::exactly_once
        ) {
            if (publish_send_count_ == publish_send_max_) {
                publish_queue_.push_back(force_move(packet));
                return true;
            }
            else {
                ++publish_send_count_;
                if (!publish_queue_.empty()) {
                    publish_queue_.push_back(force_move(packet));
                    return true;
                }
            }
        }
        return false;
    }

    void send_stored() {
        BOOST_ASSERT(strand().running_in_this_thread());
        store_.for_each(
            [&](basic_store_packet_variant<PacketIdBytes> const& pv) {
                if (pv.size() > maximum_packet_size_send_) {
                    pid_man_.release_id(pv.packet_id());
                    return false;
                }
                pv.visit(
                    // copy packet because the stored packets need to be preserved
                    // until receiving puback/pubrec/pubcomp
                    overload {
                        [&](v3_1_1::basic_publish_packet<PacketIdBytes> p) {
                            send(
                                p,
                                [](system_error const&){}
                            );
                        },
                        [&](v5::basic_publish_packet<PacketIdBytes> p) {
                            if (enqueue_publish(p)) return;
                            send(
                                p,
                                [](system_error const&){}
                            );
                        },
                        [&](v3_1_1::basic_pubrel_packet<PacketIdBytes> p) {
                            send(
                                p,
                                [](system_error const&){}
                            );
                        },
                        [&](v5::basic_pubrel_packet<PacketIdBytes> p) {
                            send(
                                p,
                                [](system_error const&){}
                            );
                        }
                    }
                );
                return true;
            }
        );
    }

    void initialize() {
        BOOST_ASSERT(strand().running_in_this_thread());
        publish_send_count_ = 0;
        publish_queue_.clear();
        topic_alias_send_ = nullopt;
        topic_alias_recv_ = nullopt;
        publish_recv_.clear();
        qos2_publish_processing_.clear();
        need_store_ = false;
    }

    void reset_pingreq_send_timer() {
        BOOST_ASSERT(strand().running_in_this_thread());
        if (pingreq_send_interval_ms_) {
            tim_pingreq_send_.cancel();
            if (status_ == connection_status::disconnecting ||
                status_ == connection_status::disconnected) return;
            tim_pingreq_send_.expires_after(
                std::chrono::milliseconds{*pingreq_send_interval_ms_}
            );
            tim_pingreq_send_.async_wait(
                [this](error_code const& ec) {
                    if (!ec) {
                        switch (protocol_version_) {
                        case protocol_version::v3_1_1:
                            send(
                                v3_1_1::pingreq_packet(),
                                [](system_error const&){}
                            );
                            break;
                        case protocol_version::v5:
                            send(
                                v5::pingreq_packet(),
                                [](system_error const&){}
                            );
                            break;
                        default:
                            BOOST_ASSERT(false);
                            break;
                        }
                    }
                }
            );
        }
    }

    void reset_pingreq_recv_timer() {
        BOOST_ASSERT(strand().running_in_this_thread());
        if (pingreq_recv_timeout_ms_) {
            tim_pingreq_recv_.cancel();
            if (status_ == connection_status::disconnecting ||
                status_ == connection_status::disconnected) return;
            tim_pingreq_recv_.expires_after(
                std::chrono::milliseconds{*pingreq_recv_timeout_ms_}
            );
            tim_pingreq_recv_.async_wait(
                [this](error_code const& ec) {
                    if (!ec) {
                        switch (protocol_version_) {
                        case protocol_version::v3_1_1:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "pingreq recv timeout. close.";
                            close(
                                []{}
                            );
                            break;
                        case protocol_version::v5:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "pingreq recv timeout. close.";
                            send(
                                v5::disconnect_packet{
                                    disconnect_reason_code::keep_alive_timeout,
                                    properties{}
                                },
                                [this](system_error const&){
                                    close(
                                        []{}
                                    );
                                }
                            );
                            break;
                        default:
                            BOOST_ASSERT(false);
                            break;
                        }
                    }
                }
            );
        }
    }

    void reset_pingresp_recv_timer() {
        BOOST_ASSERT(strand().running_in_this_thread());
        if (pingresp_recv_timeout_ms_) {
            tim_pingresp_recv_.cancel();
            if (status_ == connection_status::disconnecting ||
                status_ == connection_status::disconnected) return;
            tim_pingresp_recv_.expires_after(
                std::chrono::milliseconds{*pingresp_recv_timeout_ms_}
            );
            tim_pingresp_recv_.async_wait(
                [this](error_code const& ec) {
                    if (!ec) {
                        switch (protocol_version_) {
                        case protocol_version::v3_1_1:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "pingresp recv timeout. close.";
                            close(
                                []{}
                            );
                            break;
                        case protocol_version::v5:
                            ASYNC_MQTT_LOG("mqtt_impl", error)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "pingresp recv timeout. close.";
                            if (status_ == connection_status::connected) {
                                send(
                                    v5::disconnect_packet{
                                        disconnect_reason_code::keep_alive_timeout,
                                            properties{}
                                    },
                                    [this](system_error const&){
                                        close(
                                            []{}
                                        );
                                    }
                                );
                            }
                            else {
                                close(
                                    []{}
                                );
                            }
                            break;
                        default:
                            BOOST_ASSERT(false);
                            break;
                        }
                    }
                }
            );
        }
    }

private:
    protocol_version protocol_version_;
    std::shared_ptr<stream_type> stream_;
    packet_id_manager<packet_id_t> pid_man_;

    bool need_store_ = false;
    store<PacketIdBytes, strand_type> store_{strand()};

    bool auto_pub_response_ = false;
    bool auto_ping_response_ = false;

    bool auto_map_topic_alias_send_ = false;
    bool auto_replace_topic_alias_send_ = false;
    optional<topic_alias_send> topic_alias_send_;
    optional<topic_alias_recv> topic_alias_recv_;

    receive_maximum_t publish_send_max_{receive_maximum_max};
    receive_maximum_t publish_recv_max_{receive_maximum_max};
    receive_maximum_t publish_send_count_{0};

    std::set<packet_id_t> publish_recv_;
    std::deque<v5::basic_publish_packet<PacketIdBytes>> publish_queue_;

    std::uint32_t maximum_packet_size_send_{packet_size_no_limit};
    std::uint32_t maximum_packet_size_recv_{packet_size_no_limit};

    connection_status status_{connection_status::disconnected};

    optional<std::size_t> pingreq_send_interval_ms_;
    optional<std::size_t> pingreq_recv_timeout_ms_;
    optional<std::size_t> pingresp_recv_timeout_ms_;

    as::steady_timer tim_pingreq_send_{strand()};
    as::steady_timer tim_pingreq_recv_{strand()};
    as::steady_timer tim_pingresp_recv_{strand()};

    std::set<packet_id_t> qos2_publish_handled_;

    bool recv_processing_ = false;
    std::set<packet_id_t> qos2_publish_processing_;
};

/**
 * @related basic_endpoint
 * @brief Type alias of basic_endpoint (PacketIdBytes=2).
 * @tparam Role          role for packet sendable checking
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <role Role, typename NextLayer>
using endpoint = basic_endpoint<Role, 2, NextLayer>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_ENDPOINT_HPP
