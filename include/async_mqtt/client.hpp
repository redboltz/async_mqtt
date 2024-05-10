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
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, typename NextLayer>
class client {
    using this_type = client<Version, NextLayer>;
    using ep_type = basic_endpoint<role::client, 2, NextLayer>;
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
    /**
     * @brief publish completion handler parameter class
     */
    struct pubres_t {
        std::optional<puback_packet> puback_opt;   ///< puback_packet as the response when you send QoS1 publish
        std::optional<pubrec_packet> pubrec_opt;   ///< pubrec_packet as the response when you send QoS2 publish
        std::optional<pubcomp_packet> pubcomp_opt; ///< pubcomp_packet as the response when you send QoS2 publish
    };

    /**
     * @brief constructor
     * @tparam Args Types for the next layer
     * @param  args args for the next layer. There are predefined next layer types:
     *              \n @link protocol::mqtt @endlink, @link protocol::mqtts @endlink,
     *              @link protocol::ws @endlink, and @link protocol::wss @endlink.
     */
    template <typename... Args>
    client(
        Args&&... args
    ): ep_{ep_type::create(Version, std::forward<Args>(args)...)}
    {
        ep_->set_auto_pub_response(true);
        ep_->set_auto_ping_response(true);
    }


    /**
     * @brief send CONNECT packet and start packet receive loop
     * @param packet CONNECT packet
     * @param token the params are error_code, std::optional<connack_packet>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto start(
        connect_packet packet,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "start: " << packet;
        return
            as::async_compose<
                CompletionToken,
                void(error_code const& ec, std::optional<connack_packet>)
            >(
                start_impl{
                    *this,
                    force_move(packet)
                },
                token
            );
    }

    /**
     * @brief send SUBSCRIBE packet
     * @param packet SUBSCRIBE packet
     * @param token the params are error_code, std::optional<suback_packet>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto subscribe(
        subscribe_packet packet,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << packet;
        return
            as::async_compose<
                CompletionToken,
                void(error_code const& ec, std::optional<suback_packet>)
            >(
                subscribe_impl{
                    *this,
                    force_move(packet)
                },
                token
            );
    }

    /**
     * @brief send UNSUBSCRIBE packet
     * @param packet UNSUBSCRIBE packet
     * @param token the params are error_code, std::optional<unsuback_packet>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto unsubscribe(
        unsubscribe_packet packet,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << packet;
        return
            as::async_compose<
                CompletionToken,
                void(error_code const& ec, std::optional<unsuback_packet>)
            >(
                unsubscribe_impl{
                    *this,
                    force_move(packet)
                },
                token
            );
    }

    /**
     * @brief send PUBLISH packet
     * @param packet PUBLISH packet
     * @param token the params are error_code, pubres_t
     *              When sending QoS0 packet, all members of pubres_t is std::nullopt.
     *              When sending QoS1 packet, only pubres_t::puback_opt is set.
     *              When sending QoS1 packet, only pubres_t::pubrec_opt pubres_t::pubcomp are set.
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto publish(
        publish_packet packet,
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << packet;
        return
            as::async_compose<
                CompletionToken,
                void(error_code const& ec, pubres_t)
            >(
                publish_impl{
                    *this,
                    force_move(packet)
                },
                token
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
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << packet;
        return
            as::async_compose<
                CompletionToken,
                void(error_code const& ec)
            >(
                disconnect_impl{
                    *this,
                    force_move(packet)
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
    auto
    close(CompletionToken&& token) {
        return ep_->close(std::forward<CompletionToken>(token));
    }

    /**
     * @brief receive PUBLISH or DISCONNECT packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     * @param token the params are error_code, std::optional<publish_packet>, and std::optional<disconnect_packet>
     * @return deduced by token
     */
    template <typename CompletionToken>
    auto recv(
        CompletionToken&& token
    ) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "recv";
        return
            as::async_compose<
                CompletionToken,
                void(error_code const& ec, std::optional<publish_packet>, std::optional<disconnect_packet>)
            >(
                recv_impl{
                    *this
                },
                token
            );
    }

    /**
     * @brief executor getter
     * @return return endpoint's  executor.
     */
    as::any_io_executor get_executor() const {
        return ep_->get_executor();
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
     * @param token the param is std::optional<packet_id_t>
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
     * @return std::optional<packet_id_t> if acquired return acquired packet id, otherwise std::nullopt
     * @note This function is SYNC function that thread unsafe without strand.
     */
    std::optional<packet_id_t> acquire_unique_packet_id() {
        return ep_->acquire_unique_packet_id();
    }

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @return If true, success, otherwise the packet_id has already been used.
     * @note This function is SYNC function that thread unsafe without strand.
     */
    bool register_packet_id(packet_id_t pid) {
        return ep_->register_packet_id(pid);
    }

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @note This function is SYNC function that thread unsafe without strand.
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
                                const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                                it->tim->cancel();
                                recv_loop();
                            }
                        },
                        [&](suback_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                                it->tim->cancel();
                            }
                            recv_loop();
                        },
                        [&](unsuback_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<std::optional<packet_variant>&>(it->pv).emplace(p);
                                it->tim->cancel();
                            }
                            recv_loop();
                        },
                        [&](publish_packet& p) {
                            recv_queue_.emplace_back(force_move(p));
                            recv_queue_inserted_  = true;
                            tim_notify_publish_recv_.cancel();
                            recv_loop();
                        },
                        [&](puback_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<std::optional<puback_packet>&>(it->res.puback_opt).emplace(p);
                                it->tim->cancel();
                            }
                            recv_loop();
                        },
                        [&](pubrec_packet& p) {
                            auto& idx = pid_tim_pv_res_col_.template get<tag_pid>();
                            auto it = idx.find(p.packet_id());
                            if (it != idx.end()) {
                                const_cast<std::optional<pubrec_packet>&>(it->res.pubrec_opt).emplace(p);
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
                                const_cast<std::optional<pubcomp_packet>&>(it->res.pubcomp_opt).emplace(p);
                                it->tim->cancel();
                            }
                            recv_loop();
                        },
                        [&](disconnect_packet& p) {
                            recv_queue_.emplace_back(force_move(p));
                            recv_queue_inserted_  = true;
                            tim_notify_publish_recv_.cancel();
                            recv_loop();
                        },
                        [&](system_error const& se) {
                            recv_queue_.emplace_back(se.code());
                            recv_queue_inserted_  = true;
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

    struct start_impl {
        this_type& cl;
        connect_packet packet;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            auto& a_cl{cl};
            auto a_packet{packet};
            a_cl.ep_->send(
                force_move(a_packet),
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            system_error const& se
        ) {
            if (se) {
                self.complete(se.code(), std::nullopt);
                return;
            }

            auto tim = std::make_shared<as::steady_timer>(cl.ep_->get_executor());
            tim->expires_at(std::chrono::steady_clock::time_point::max());
            cl.pid_tim_pv_res_col_.emplace(tim);
            cl.recv_loop();
            auto& a_cl{cl};
            tim->async_wait(
                as::bind_executor(
                    a_cl.get_executor(),
                    as::append(
                        force_move(self),
                        tim
                    )
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& /* ec */,
            std::shared_ptr<as::steady_timer> tim
        ) {
            auto& idx = cl.pid_tim_pv_res_col_.template get<tag_tim>();
            auto it = idx.find(tim);
            if (it == idx.end()) {
                self.complete(
                    errc::make_error_code(sys::errc::operation_canceled),
                    std::nullopt
                );
            }
            else {
                auto pv = it->pv;
                idx.erase(it);
                if (auto *p = pv->template get_if<connack_packet>()) {
                    self.complete(error_code{}, *p);
                }
                else {
                    self.complete(
                        errc::make_error_code(sys::errc::protocol_error),
                        std::nullopt
                    );
                }
            }
        }
    };

    struct subscribe_impl {
        this_type& cl;
        subscribe_packet packet;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            auto& a_cl{cl};
            auto pid = packet.packet_id();
            auto a_packet{packet};
            a_cl.ep_->send(
                force_move(a_packet),
                as::append(
                    force_move(self),
                    pid
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            system_error const& se,
            packet_id_t pid
        ) {
            if (se) {
                self.complete(se.code(), std::nullopt);
                return;
            }

            auto tim = std::make_shared<as::steady_timer>(cl.ep_->get_executor());
            tim->expires_at(std::chrono::steady_clock::time_point::max());
            cl.pid_tim_pv_res_col_.emplace(pid, tim);
            auto& a_cl{cl};
            tim->async_wait(
                as::bind_executor(
                    a_cl.get_executor(),
                    as::append(
                        force_move(self),
                        tim
                    )
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& /* ec */,
            std::shared_ptr<as::steady_timer> tim
        ) {
            auto& idx = cl.pid_tim_pv_res_col_.template get<tag_tim>();
            auto it = idx.find(tim);
            if (it == idx.end()) {
                self.complete(
                    errc::make_error_code(sys::errc::operation_canceled),
                    std::nullopt
                );
            }
            else {
                auto pv = it->pv;
                idx.erase(it);
                if (auto *p = pv->template get_if<suback_packet>()) {
                    self.complete(error_code{}, *p);
                }
                else {
                    self.complete(
                        errc::make_error_code(sys::errc::protocol_error),
                        std::nullopt
                    );
                }
            }
        }
    };

    struct unsubscribe_impl {
        this_type& cl;
        unsubscribe_packet packet;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            auto& a_cl{cl};
            auto pid = packet.packet_id();
            auto a_packet{packet};
            a_cl.ep_->send(
                force_move(a_packet),
                as::append(
                    force_move(self),
                    pid
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            system_error const& se,
            packet_id_t pid
        ) {
            if (se) {
                self.complete(se.code(), std::nullopt);
                return;
            }

            auto tim = std::make_shared<as::steady_timer>(cl.ep_->get_executor());
            tim->expires_at(std::chrono::steady_clock::time_point::max());
            cl.pid_tim_pv_res_col_.emplace(pid, tim);
            auto& a_cl{cl};
            tim->async_wait(
                as::bind_executor(
                    a_cl.get_executor(),
                    as::append(
                        force_move(self),
                        tim
                    )
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& /* ec */,
            std::shared_ptr<as::steady_timer> tim
        ) {
            auto& idx = cl.pid_tim_pv_res_col_.template get<tag_tim>();
            auto it = idx.find(tim);
            if (it == idx.end()) {
                self.complete(
                    errc::make_error_code(sys::errc::operation_canceled),
                    std::nullopt
                );
            }
            else {
                auto pv = it->pv;
                idx.erase(it);
                if (auto *p = pv->template get_if<unsuback_packet>()) {
                    self.complete(error_code{}, *p);
                }
                else {
                    self.complete(
                        errc::make_error_code(sys::errc::protocol_error),
                        std::nullopt
                    );
                }
            }
        }
    };

    struct publish_impl {
        this_type& cl;
        publish_packet packet;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            auto& a_cl{cl};
            auto pid = packet.packet_id();
            auto a_packet{packet};
            a_cl.ep_->send(
                force_move(a_packet),
                as::append(
                    force_move(self),
                    pid
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            system_error const& se,
            packet_id_t pid
        ) {
            if (se) {
                self.complete(se.code(), pubres_t{});
                return;
            }
            if (pid == 0) {
                // QoS: at_most_once
                self.complete(se.code(), pubres_t{});
                return;
            }
            auto tim = std::make_shared<as::steady_timer>(cl.ep_->get_executor());
            tim->expires_at(std::chrono::steady_clock::time_point::max());
            cl.pid_tim_pv_res_col_.emplace(pid, tim);
            auto& a_cl{cl};
            tim->async_wait(
                as::bind_executor(
                    a_cl.get_executor(),
                    as::append(
                        force_move(self),
                        tim
                    )
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& /* ec */,
            std::shared_ptr<as::steady_timer> tim
        ) {
            auto& idx = cl.pid_tim_pv_res_col_.template get<tag_tim>();
            auto it = idx.find(tim);
            if (it == idx.end()) {
                self.complete(
                    errc::make_error_code(sys::errc::operation_canceled),
                    pubres_t{}
                );
            }
            else {
                auto res = it->res;
                idx.erase(it);
                self.complete(error_code{}, force_move(res));
            }
        }
    };

    struct disconnect_impl {
        this_type& cl;
        disconnect_packet packet;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            auto& a_cl{cl};
            auto a_packet{packet};
            a_cl.ep_->send(
                force_move(a_packet),
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            system_error const& se
        ) {
            self.complete(se.code());
        }
    };

    struct recv_impl {
        this_type& cl;
        enum { dispatch, recv, complete } state = dispatch;
        template <typename Self>
        void operator()(
            Self& self
        ) {
            if (state == dispatch) {
                state = recv;
                auto& a_cl{cl};
                as::dispatch(
                    as::bind_executor(
                        a_cl.ep_->get_executor(),
                        force_move(self)
                    )
                );
            }
            else {
                BOOST_ASSERT(state == recv);
                state = complete;
                if (cl.recv_queue_.empty()) {
                    cl.recv_queue_inserted_ = false;
                    auto tim = std::make_shared<as::steady_timer>(
                        cl.ep_->get_executor()
                    );
                    cl.tim_notify_publish_recv_.expires_at(
                        std::chrono::steady_clock::time_point::max()
                    );
                    auto& a_cl{cl};
                    a_cl.tim_notify_publish_recv_.async_wait(
                        as::bind_executor(
                            a_cl.get_executor(),
                            as::append(
                                force_move(self),
                                a_cl.recv_queue_inserted_
                            )
                        )
                    );
                }
                else {
                    auto [ec, publish_opt, disconnect_opt] = cl.recv_queue_.front();
                    cl.recv_queue_.pop_front();
                    self.complete(
                        ec,
                        force_move(publish_opt),
                        force_move(disconnect_opt)
                    );
                }
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& /* ec */,
            bool get_queue
        ) {
            BOOST_ASSERT(state == complete);
            if (get_queue) {
                auto [ec, publish_opt, disconnect_opt] = cl.recv_queue_.front();
                cl.recv_queue_.pop_front();
                self.complete(
                    ec,
                    force_move(publish_opt),
                    force_move(disconnect_opt)
                );
            }
            else {
                self.complete(
                    errc::make_error_code(sys::errc::operation_canceled),
                    std::nullopt,
                    std::nullopt
                );
            }
        }
    };

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
        std::optional<packet_variant> pv;
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
        std::optional<publish_packet> publish_opt;
        std::optional<disconnect_packet> disconnect_opt;
    };

    ep_type_sp ep_;
    mi_pid_tim_pv_res pid_tim_pv_res_col_;
    std::deque<recv_t> recv_queue_;
    bool recv_queue_inserted_ = false;
    as::steady_timer tim_notify_publish_recv_{ep_->get_executor()};
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_CLIENT_HPP
