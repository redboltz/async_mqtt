// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CLIENT_HPP)
#define ASYNC_MQTT_CLIENT_HPP

#include <deque>
#include <optional>

#include <boost/asio/async_result.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>

#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/endpoint_fwd.hpp>
#include <async_mqtt/detail/client_packet_type_getter.hpp>

/**
 * @defgroup client
 * @ingroup connection
 * @brief High level MQTT connection layer that can be used client.
 * Receiving MQTT response packet is automated.
 */

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @ingroup client
 * @brief MQTT client for casual usecases
 * @tparam Version       MQTT protocol version.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, typename NextLayer>
class client {
    using this_type = client<Version, NextLayer>;
    using ep_type = basic_endpoint<role::client, 2, NextLayer>;
    using ep_type_sp = std::shared_ptr<ep_type>;
    using executor_type = typename ep_type::executor_type;
    using next_layer_type = typename ep_type::next_layer_type;
    using lowest_layer_type = typename ep_type::lowest_layer_type;

    ASYNC_MQTT_PACKET_TYPE(Version, connect)
    ASYNC_MQTT_PACKET_TYPE(Version, connack)
    ASYNC_MQTT_PACKET_TYPE(Version, subscribe)
    ASYNC_MQTT_PACKET_TYPE(Version, suback)
    ASYNC_MQTT_PACKET_TYPE(Version, unsubscribe)
    ASYNC_MQTT_PACKET_TYPE(Version, unsuback)
    ASYNC_MQTT_PACKET_TYPE(Version, publish)
    ASYNC_MQTT_PACKET_TYPE(Version, puback)
    ASYNC_MQTT_PACKET_TYPE(Version, pubrec)
    ASYNC_MQTT_PACKET_TYPE(Version, pubrel)
    ASYNC_MQTT_PACKET_TYPE(Version, pubcomp)
    ASYNC_MQTT_PACKET_TYPE(Version, pingreq)
    ASYNC_MQTT_PACKET_TYPE(Version, pingresp)
    ASYNC_MQTT_PACKET_TYPE(Version, disconnect)

public:
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
    );

    /**
     * @brief send CONNECT packet and start packet receive loop
     * @param packet CONNECT packet
     * @param token the params are error_code, std::optional<connack_packet>
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, std::optional<connack_packet>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    start(
        connect_packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief send SUBSCRIBE packet
     * @param packet SUBSCRIBE packet
     * @param token the params are error_code, std::optional<suback_packet>
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, std::optional<suback_packet>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    subscribe(
        subscribe_packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief send UNSUBSCRIBE packet
     * @param packet UNSUBSCRIBE packet
     * @param token the params are error_code, std::optional<unsuback_packet>
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, std::optional<suback_packet>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    unsubscribe(
        unsubscribe_packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief send PUBLISH packet
     * @param packet PUBLISH packet
     * @param token the params are error_code, pubres_t
     *              When sending QoS0 packet, all members of pubres_t is std::nullopt.
     *              When sending QoS1 packet, only pubres_t::puback_opt is set.
     *              When sending QoS1 packet, only pubres_t::pubrec_opt pubres_t::pubcomp are set.
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, pubres_t)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    publish(
        publish_packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief send DISCONNECT packet
     * @param packet DISCONNECT packet
     * @param token the params is error_code
     * @return deduced by token
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
    disconnect(
        disconnect_packet packet,
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
     * @brief receive PUBLISH or DISCONNECT packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     * @param token the params are error_code, std::optional<publish_packet>, and std::optional<disconnect_packet>
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, std::optional<publish_packet>, std::optional<disconnect_packet>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    recv(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief executor getter
     * @return return endpoint's  executor.
     */
    as::any_io_executor get_executor() const;

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
     * @brief auto map (allocate) topic alias on send PUBLISH packet.
     * If all topic aliases are used, then overwrite by LRU algorithm.
     * \n This function should be called before send() call.
     * @note By default not automatically mapping.
     * @param val if true, enable auto mapping, otherwise disable.
     */
    void set_auto_map_topic_alias_send(bool val);

    /**
     * @brief auto replace topic with corresponding topic alias on send PUBLISH packet.
     * Registering topic alias need to do manually.
     * \n This function should be called before send() call.
     * @note By default not automatically replacing.
     * @param val if true, enable auto replacing, otherwise disable.
     */
    void set_auto_replace_topic_alias_send(bool val);

    /**
     * @brief Set timeout for receiving PINGRESP packet after PINGREQ packet is sent.
     * If the timer is fired, then the underlying layer is closed from the client side.
     * If the protocol_version is v5, then send DISCONNECT packet with the reason code
     * disconnect_reason_code::keep_alive_timeout automatically before underlying layer is closed.
     * \n This function should be called before send() call.
     * @note By default timeout is not set.
     * @param val if 0, timer is not set, otherwise set val milliseconds.
     */
    void set_pingresp_recv_timeout_ms(std::size_t ms);

    /**
     * @brief Set bulk write mode.
     * If true, then concatenate multiple packets' const buffer sequence
     * when send() is called before the previous send() is not completed.
     * Otherwise, send packet one by one.
     * \n This function should be called before send() call.
     * @note By default bulk write mode is false (disabled)
     * @param val if true, enable bulk write mode, otherwise disable it.
     */
    void set_bulk_write(bool val);

    /**
     * @brief acuire unique packet_id.
     * @param token the param is std::optional<packet_id_type>
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(std::optional<packet_id_type>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    acquire_unique_packet_id(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief acuire unique packet_id.
     * If packet_id is fully acquired, then wait until released.
     * @param token the param is packet_id_type
     * @return deduced by token
     */
    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(packet_id_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    acquire_unique_packet_id_wait_until(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief acuire unique packet_id.
     * If packet_id is fully acquired, then wait until released.
     * @param token the param is packet_id_type
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
        packet_id_type pid,
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
        void()
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    release_packet_id(
        packet_id_type pid,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief acuire unique packet_id.
     * @return std::optional<packet_id_type> if acquired return acquired packet id, otherwise std::nullopt
     * @note This function is SYNC function that thread unsafe without strand.
     */
    std::optional<packet_id_type> acquire_unique_packet_id();

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @return If true, success, otherwise the packet_id has already been used.
     * @note This function is SYNC function that thread unsafe without strand.
     */
    bool register_packet_id(packet_id_type pid);

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @note This function is SYNC function that thread unsafe without strand.
     */
    void release_packet_id(packet_id_type pid);

private:
    void recv_loop();

    // async operations
    struct start_op;
    struct subscribe_op;
    struct unsubscribe_op;
    struct publish_op;
    struct disconnect_op;
    struct recv_op;

    // internal types
    struct pid_tim_pv_res_col;
    struct recv_type;

    ep_type_sp ep_;
    pid_tim_pv_res_col pid_tim_pv_res_col_;
    std::deque<recv_type> recv_queue_;
    bool recv_queue_inserted_ = false;
    as::steady_timer tim_notify_publish_recv_;
};

} // namespace async_mqtt

#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/impl/client_start.hpp>
#include <async_mqtt/impl/client_subscribe.hpp>
#include <async_mqtt/impl/client_unsubscribe.hpp>
#include <async_mqtt/impl/client_publish.hpp>
#include <async_mqtt/impl/client_disconnect.hpp>
#include <async_mqtt/impl/client_close.hpp>
#include <async_mqtt/impl/client_recv.hpp>
#include <async_mqtt/impl/client_acquire_unique_packet_id.hpp>
#include <async_mqtt/impl/client_acquire_unique_packet_id_wait_until.hpp>
#include <async_mqtt/impl/client_register_packet_id.hpp>
#include <async_mqtt/impl/client_release_packet_id.hpp>

#endif // ASYNC_MQTT_CLIENT_HPP
