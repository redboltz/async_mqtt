// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CLIENT_HPP)
#define ASYNC_MQTT_CLIENT_HPP

#include <async_mqtt/endpoint.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/preprocessor/cat.hpp>

/// @file

namespace async_mqtt {

namespace detail {

#define ASYNC_MQTT_PACKET_TYPE_GETTER(packet)                     \
    template <protocol_version Ver>                               \
    struct BOOST_PP_CAT(meta_,packet) {                           \
        using type = v5::BOOST_PP_CAT(packet, _packet);           \
    };                                                            \
    template <>                                                   \
    struct BOOST_PP_CAT(meta_,packet)<protocol_version::v3_1_1> { \
        using type = v3_1_1::BOOST_PP_CAT(packet, _packet);       \
    };

ASYNC_MQTT_PACKET_TYPE_GETTER(connect)
ASYNC_MQTT_PACKET_TYPE_GETTER(connack)
ASYNC_MQTT_PACKET_TYPE_GETTER(subscribe)
ASYNC_MQTT_PACKET_TYPE_GETTER(suback)
ASYNC_MQTT_PACKET_TYPE_GETTER(unsubscribe)
ASYNC_MQTT_PACKET_TYPE_GETTER(unsuback)
ASYNC_MQTT_PACKET_TYPE_GETTER(publish)
ASYNC_MQTT_PACKET_TYPE_GETTER(puback)
ASYNC_MQTT_PACKET_TYPE_GETTER(pubrec)
ASYNC_MQTT_PACKET_TYPE_GETTER(pubrel)
ASYNC_MQTT_PACKET_TYPE_GETTER(pubcomp)
ASYNC_MQTT_PACKET_TYPE_GETTER(pingreq)
ASYNC_MQTT_PACKET_TYPE_GETTER(pingresp)
ASYNC_MQTT_PACKET_TYPE_GETTER(disconnect)

#undef ASYNC_MQTT_PACKET_TYPE_GETTER

#define ASYNC_MQTT_PACKET_TYPE(packet)                           \
    using BOOST_PP_CAT(packet, _packet) = typename BOOST_PP_CAT(detail::meta_, packet<Version>::type);

} // namespace detail

/**
 * @brief MQTT client for casual usecases
 * @tparam Version       MQTT protocol version.
 * @tparam Strand        strand class template type. By default boost::asio::strand<T> should be used.
 *                       You can replace it with null_strand if you run the endpoint on single thread environment.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, template <typename> typename Strand, typename NextLayer>
class basic_client {
    using ep_type = basic_endpoint<role::client, 2, Strand, NextLayer>;
    using ep_type_sp = std::shared_ptr<ep_type>;


    ASYNC_MQTT_PACKET_TYPE(connect)
    ASYNC_MQTT_PACKET_TYPE(connack)
    ASYNC_MQTT_PACKET_TYPE(subscribe)
    ASYNC_MQTT_PACKET_TYPE(suback)
    ASYNC_MQTT_PACKET_TYPE(unsubscribe)
    ASYNC_MQTT_PACKET_TYPE(unsuback)
    ASYNC_MQTT_PACKET_TYPE(publish)
    ASYNC_MQTT_PACKET_TYPE(puback)
    ASYNC_MQTT_PACKET_TYPE(pubrec)
    ASYNC_MQTT_PACKET_TYPE(pubrel)
    ASYNC_MQTT_PACKET_TYPE(pubcomp)
    ASYNC_MQTT_PACKET_TYPE(pingreq)
    ASYNC_MQTT_PACKET_TYPE(pingresp)
    ASYNC_MQTT_PACKET_TYPE(disconnect)

#undef ASYNC_MQTT_PACKET_TYPE

public:
    using packet_id_t = typename ep_type::packet_id_t;
    using strand_type = typename ep_type::strand_type;
    /**
     * @brief publish completion handler parameter class
     */
    struct pubres_t {
        optional<puback_packet> puback_opt;   ///< puback_packet as the response when you send QoS1 publish
        optional<pubrec_packet> pubrec_opt;   ///< pubrec_packet as the response when you send QoS2 publish
        optional<pubcomp_packet> pubcomp_opt; ///< pubcomp_packet as the response when you send QoS2 publish
    };

    /**
     * @brief constructor
     * @tparam Args Types for the next layer
     * @param  args args for the next layer. There are predefined next layer types:
     *              \n @link protocol::mqtt @endlink, @link protocol::mqtts @endlink,
     *              @link protocol::ws @endlink, and @link protocol::wss @endlink.
     */
    template <typename... Args>
    basic_client(
        Args&&... args
    ): ep_{ep_type::create(Version, std::forward<Args>(args)...)}
    {
        ep_->set_auto_pub_response(true);
        ep_->set_auto_ping_response(true);
    }


    /**
     * @brief send CONNECT packet and start packet receive loop
     * @param packet CONNECT packet
     * @param token the params are error_code, optional<connack_packet>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto start(
        connect_packet packet,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec, optional<connack_packet>)
        >(
            [this](
                auto completion_handler,
                connect_packet&& packet
            ) {
                ep_->send(
                    force_move(packet),
                    [this, completion_handler = force_move(completion_handler)]
                    (auto const& se) mutable {
                        if (se) {
                            force_move(completion_handler)(se.code(), nullopt);
                            return;
                        }
                        auto tim = std::make_shared<as::steady_timer>(ep_->strand());
                        tim->expires_at(std::chrono::steady_clock::time_point::max());
                        pid_tim_pv_res_col_.emplace(tim);
                        recv_loop();
                        tim->async_wait(
                            [this, tim, completion_handler = force_move(completion_handler)]
                            (error_code const& /*ec*/) mutable {
                                auto& idx = pid_tim_pv_res_col_.template get<tag_tim>();
                                auto it =idx.find(tim);
                                if (it != idx.end()) {
                                    auto pv = it->pv;
                                    idx.erase(it);
                                    if (auto *p = pv->template get_if<connack_packet>()) {
                                        force_move(completion_handler)(error_code{}, *p);
                                    }
                                    else {
                                        force_move(completion_handler)(
                                            errc::make_error_code(sys::errc::protocol_error),
                                            nullopt
                                        );
                                    }
                                }
                            }
                        );
                    }
                );
            },
            token,
            force_move(packet)
        );
    }

    /**
     * @brief send SUBSCRIBE packet
     * @param packet SUBSCRIBE packet
     * @param token the params are error_code, optional<suback_packet>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto subscribe(
        subscribe_packet packet,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec, optional<suback_packet>)
        >(
            [this](
                auto completion_handler,
                subscribe_packet&& packet
            ) {
                auto pid = packet.packet_id();
                ep_->send(
                    force_move(packet),
                    [this, pid, completion_handler = force_move(completion_handler)]
                    (auto const& se) mutable {
                        if (se) {
                            force_move(completion_handler)(se.code(), nullopt);
                            return;
                        }
                        auto tim = std::make_shared<as::steady_timer>(ep_->strand());
                        tim->expires_at(std::chrono::steady_clock::time_point::max());
                        pid_tim_pv_res_col_.emplace(pid, tim);
                        tim->async_wait(
                            [this, tim, completion_handler = force_move(completion_handler)]
                            (error_code const& /*ec*/) mutable {
                                auto& idx = pid_tim_pv_res_col_.template get<tag_tim>();
                                auto it = idx.find(tim);
                                if (it != idx.end()) {
                                    auto pv = it->pv;
                                    idx.erase(it);
                                    if (auto *p = pv->template get_if<suback_packet>()) {
                                        force_move(completion_handler)(error_code{}, *p);
                                    }
                                    else {
                                        force_move(completion_handler)(
                                            errc::make_error_code(sys::errc::protocol_error),
                                            nullopt
                                        );
                                    }
                                }
                            }
                        );
                    }
                );
            },
            token,
            force_move(packet)
        );
    }

    /**
     * @brief send UNSUBSCRIBE packet
     * @param packet UNSUBSCRIBE packet
     * @param token the params are error_code, optional<unsuback_packet>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto unsubscribe(
        unsubscribe_packet packet,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec, optional<unsuback_packet>)
        >(
            [this](
                auto completion_handler,
                unsubscribe_packet&& packet
            ) {
                auto pid = packet.packet_id();
                ep_->send(
                    force_move(packet),
                    [this, pid, completion_handler = force_move(completion_handler)]
                    (auto const& se) mutable {
                        if (se) {
                            force_move(completion_handler)(se.code(), nullopt);
                            return;
                        }
                        auto tim = std::make_shared<as::steady_timer>(ep_->strand());
                        tim->expires_at(std::chrono::steady_clock::time_point::max());
                        pid_tim_pv_res_col_.emplace(pid, tim);
                        tim->async_wait(
                            [this, tim, completion_handler = force_move(completion_handler)]
                            (error_code const& /*ec*/) mutable {
                                auto& idx = pid_tim_pv_res_col_.template get<tag_tim>();
                                auto it = idx.find(tim);
                                if (it != idx.end()) {
                                    auto pv = it->pv;
                                    idx.erase(it);
                                    if (auto *p = pv->template get_if<unsuback_packet>()) {
                                        force_move(completion_handler)(error_code{}, *p);
                                    }
                                    else {
                                        force_move(completion_handler)(
                                            errc::make_error_code(sys::errc::protocol_error),
                                            nullopt
                                        );
                                    }
                                }
                            }
                        );
                    }
                );
            },
            token,
            force_move(packet)
        );
    }

    /**
     * @brief send PUBLISH packet
     * @param packet PUBLISH packet
     * @param token the params are error_code, pubres_t
     *              When sending QoS0 packet, all members of pubres_t is nullopt.
     *              When sending QoS1 packet, only pubres_t::puback_opt is set.
     *              When sending QoS1 packet, only pubres_t::pubrec_opt pubres_t::pubcomp are set.
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto publish(
        publish_packet packet,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec, pubres_t res)
        >(
            [this](
                auto completion_handler,
                publish_packet&& packet
            ) {
                auto pid = packet.packet_id();
                ep_->send(
                    force_move(packet),
                    [this, pid, completion_handler = force_move(completion_handler)]
                    (auto const& se) mutable {
                        if (se) {
                            force_move(completion_handler)(se.code(), pubres_t{});
                            return;
                        }
                        auto tim = std::make_shared<as::steady_timer>(ep_->strand());
                        tim->expires_at(std::chrono::steady_clock::time_point::max());
                        if (pid == 0) {
                            // QoS: at_most_once
                            force_move(completion_handler)(se.code(), pubres_t{});
                            return;
                        }
                        pid_tim_pv_res_col_.emplace(pid, tim);
                        tim->async_wait(
                            [this, tim, completion_handler = force_move(completion_handler)]
                            (error_code const& /*ec*/) mutable {
                                auto& idx = pid_tim_pv_res_col_.template get<tag_tim>();
                                auto it = idx.find(tim);
                                if (it != idx.end()) {
                                    auto res = it->res;
                                    idx.erase(it);
                                    force_move(completion_handler)(error_code{}, res);
                                }
                            }
                        );
                    }
                );
            },
            token,
            force_move(packet)
        );
    }

    /**
     * @brief send DISCONNECT packet
     * @param packet DISCONNECT packet
     * @param token the params is error_code
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto disconnect(
        disconnect_packet packet,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec)
        >(
            [this](
                auto completion_handler,
                disconnect_packet&& packet
            ) {
                ep_->send(
                    force_move(packet),
                    [completion_handler = force_move(completion_handler)]
                    (auto const& se) mutable {
                        force_move(completion_handler)(se.code());
                    }
                );
            },
            token,
            force_move(packet)
        );
    }

    /**
     * @brief close the underlying connection
     * @param token  the param is void
     * @return deduced by token
     */
    template<typename CompletionToken>
    auto
    close(CompletionToken&& token) {
        return ep_->close(std::forward<CompletionToken>(token));
    }

    /**
     * @brief receive PUBLISH or DISCONNECT packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     * @param token the params are error_code, optional<publish_packet>, and optional<disconnect_packet>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto recv(
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec, optional<publish_packet>, optional<disconnect_packet>)
        >(
            [this](
                auto completion_handler
            ) {
                as::dispatch(
                    as::bind_executor(
                        ep_->strand(),
                        [this, completion_handler = force_move(completion_handler)]
                        () mutable {
                            auto call_completion_handler =
                                [this, completion_handler = force_move(completion_handler)]
                                () mutable {
                                    auto [ec, publish_opt, disconnect_opt] = recv_queue_.front();
                                    recv_queue_.pop_front();
                                    force_move(completion_handler)(
                                        ec,
                                        force_move(publish_opt),
                                        force_move(disconnect_opt)
                                    );
                                };
                            if (recv_queue_.empty()) {
                                auto tim = std::make_shared<as::steady_timer>(ep_->strand());
                                tim_notify_publish_recv_.expires_at(std::chrono::steady_clock::time_point::max());
                                tim_notify_publish_recv_.async_wait(
                                    [call_completion_handler = force_move(call_completion_handler)]
                                    (error_code const& /*ec*/) mutable {
                                        call_completion_handler();
                                    }
                                );
                            }
                            else {
                                call_completion_handler();
                            }
                        }
                    )
                );
            },
            token
        );
    }

    /**
     * @brief executor getter
     * @return strand as an executor
     */
    as::any_io_executor get_executor() const {
        return ep_->strand();
    }

    /**
     * @brief strand getter
     * @return const reference of the strand
     */
    strand_type const& strand() const {
        return ep_->strand();
    }

    /**
     * @brief strand getter
     * @return reference of the strand
     */
    strand_type& strand() {
        return ep_->strand();
    }

    /**
     * @brief strand checker
     * @return true if the current context running in the strand, otherwise false
     */
    bool in_strand() const {
        return ep_->in_strand();
    }

    /**
     * @brief next_layer getter
     * @return const reference of the next_layer
     */
    auto const& next_layer() const {
        return ep_->next_layer();
    }
    /**
     * @brief next_layer getter
     * @return reference of the next_layer
     */
    auto& next_layer() {
        return ep_->next_layer();
    }

    /**
     * @brief lowest_layer getter
     * @return const reference of the lowest_layer
     */
    auto const& lowest_layer() const {
        return ep_->lowest_layer();
    }
    /**
     * @brief lowest_layer getter
     * @return reference of the lowest_layer
     */
    auto& lowest_layer() {
        return ep_->lowest_layer();
    }

    /**
     * @brief auto map (allocate) topic alias on send PUBLISH packet.
     * If all topic aliases are used, then overwrite by LRU algorithm.
     * \n This function should be called before send() call.
     * @note By default not automatically mapping.
     * @param val if true, enable auto mapping, otherwise disable.
     */
    void set_auto_map_topic_alias_send(bool val) {
        ep_->set_auto_map_topic_alias_send(val);
    }

    /**
     * @brief auto replace topic with corresponding topic alias on send PUBLISH packet.
     * Registering topic alias need to do manually.
     * \n This function should be called before send() call.
     * @note By default not automatically replacing.
     * @param val if true, enable auto replacing, otherwise disable.
     */
    void set_auto_replace_topic_alias_send(bool val) {
        ep_->set_auto_replace_topic_alias_send(val);
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
        ep_->set_pingresp_recv_timeout_ms(ms);
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
        ep_->set_bulk_write(val);
    }

    /**
     * @brief acuire unique packet_id.
     * @param token the param is optional<packet_id_t>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto acquire_unique_packet_id(CompletionToken&& token) {
        return ep_->acquire_unique_packet_id(std::forward<CompletionToken>(token));
    }

    /**
     * @brief acuire unique packet_id.
     * If packet_id is fully acquired, then wait until released.
     * @param token the param is packet_id_t
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto
    acquire_unique_packet_id_wait_until(
        CompletionToken&& token
    ) {
        return ep_->acquire_unique_packet_id_wait_until(std::forward<CompletionToken>(token));
    }

    /**
     * @brief acuire unique packet_id.
     * If packet_id is fully acquired, then wait until released.
     * @param token the param is packet_id_t
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto register_packet_id(packet_id_t pid, CompletionToken&& token) {
        return ep_->register_packet_id(pid, std::forward<CompletionToken>(token));
    }

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @param token     the param is bool. If true, success, otherwise the packet_id has already been used.
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto release_packet_id(packet_id_t pid, CompletionToken&& token) {
        return ep_->release_packet_id(pid, std::forward<CompletionToken>(token));
    }

    /**
     * @brief acuire unique packet_id.
     * @return optional<packet_id_t> if acquired return acquired packet id, otherwise nullopt
     * @note This function is SYNC function that must only be called in the strand.
     */
    optional<packet_id_t> acquire_unique_packet_id() {
        return ep_->acquire_unique_packet_id();
    }

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @return If true, success, otherwise the packet_id has already been used.
     * @note This function is SYNC function that must only be called in the strand.
     */
    bool register_packet_id(packet_id_t pid) {
        return ep_->register_packet_id(pid);
    }

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @note This function is SYNC function that must only be called in the strand.
     */
    void release_packet_id(packet_id_t pid) {
        ep_->release_packet_id(pid);
    }

private:

    void recv_loop() {
        ep_->recv(
            [this]
            (packet_variant pv) mutable {
                pv.visit(
                    overload {
                        [&](connack_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(0);
                            if (it != idx.end()) {
                                const_cast<optional<packet_variant>&>(it->pv).emplace(p);
                                it->tim->cancel();
                                recv_loop();
                            }
                        },
                        [&](suback_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<optional<packet_variant>&>(it->pv).emplace(p);
                                it->tim->cancel();
                            }
                            recv_loop();
                        },
                        [&](unsuback_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<optional<packet_variant>&>(it->pv).emplace(p);
                                it->tim->cancel();
                            }
                            recv_loop();
                        },
                        [&](publish_packet& p) {
                            recv_queue_.emplace_back(force_move(p));
                            tim_notify_publish_recv_.cancel();
                            recv_loop();
                        },
                        [&](puback_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<optional<puback_packet>&>(it->res.puback_opt).emplace(p);
                                it->tim->cancel();
                            }
                            recv_loop();
                        },
                        [&](pubrec_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<optional<pubrec_packet>&>(it->res.pubrec_opt).emplace(p);
                                if constexpr (Version == protocol_version::v5) {
                                    if (is_error(p.code())) {
                                        it->tim->cancel();
                                    }
                                }
                            }
                            recv_loop();
                        },
                        [&](pubcomp_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<optional<pubcomp_packet>&>(it->res.pubcomp_opt).emplace(p);
                                it->tim->cancel();
                            }
                            recv_loop();
                        },
                        [&](disconnect_packet& p) {
                            recv_queue_.emplace_back(force_move(p));
                            tim_notify_publish_recv_.cancel();
                            recv_loop();
                        },
                        [&](system_error const& se) {
                            recv_queue_.emplace_back(se.code());
                            tim_notify_publish_recv_.cancel();
                        },
                        [&](auto const&) {
                            recv_loop();
                        }
                    }
                );
            }
        );
    }

private:

    struct pid_tim_pv_res {
        pid_tim_pv_res(
            packet_id_t pid,
            std::shared_ptr<as::steady_timer> tim
        ): pid{pid},
           tim{force_move(tim)}
        {
        }
        pid_tim_pv_res(
            std::shared_ptr<as::steady_timer> tim
        ): tim{force_move(tim)}
        {
        }
        packet_id_t pid = 0;
        std::shared_ptr<as::steady_timer> tim;
        optional<packet_variant> pv;
        pubres_t res;
    };
    struct tag_pid {};
    struct tag_tim {};

    using mi_pid_tim_pv_res = mi::multi_index_container<
        pid_tim_pv_res,
        mi::indexed_by<
            mi::ordered_unique<
                mi::tag<tag_pid>,
                mi::key<&pid_tim_pv_res::pid>
            >,
            mi::ordered_unique<
                mi::tag<tag_tim>,
                mi::key<&pid_tim_pv_res::tim>
            >
        >
    >;

    struct recv_t {
        recv_t(publish_packet packet)
            :publish_opt{force_move(packet)}
        {
        }
        recv_t(disconnect_packet packet)
            :disconnect_opt{force_move(packet)}
        {
        }
        recv_t(error_code ec)
            :ec{ec}
        {
        }
        error_code ec = error_code{};
        optional<publish_packet> publish_opt;
        optional<disconnect_packet> disconnect_opt;
    };

    ep_type_sp ep_;
    mi_pid_tim_pv_res pid_tim_pv_res_col_;
    std::deque<recv_t> recv_queue_;
    as::steady_timer tim_notify_publish_recv_{ep_->strand()};
};


/**
 * @related basic_client
 * @brief Type alias of basic_client (Strand=boost::asio::strand).
 *        This is for typical usecase.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, typename NextLayer>
using client = basic_client<Version, as::strand, NextLayer>;

/**
 * @related basic_client
 * @brief Type alias of basic_client (Strand=null_strand).
 *        This is for typical usecase.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, typename NextLayer>
using client_st = basic_client<Version, null_strand, NextLayer>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_CLIENT_HPP
