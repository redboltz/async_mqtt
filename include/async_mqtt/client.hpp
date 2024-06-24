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

#include <async_mqtt/error.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/endpoint_fwd.hpp>
#include <async_mqtt/detail/client_packet_type_getter.hpp>
#include <async_mqtt/util/make_shared_helper.hpp>

/**
 * @defgroup client client (High level MQTT client)
 * @ingroup connection
 */

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @ingroup client
 * @brief MQTT client for casual usecases
 * #### Thread Safety
 *    - Distinct objects: Safe
 *    - Shared objects: Unsafe
 *
 * #### Internal type aliases
 * - connect_packet
 *   - connect_packet of the Version
 *     - v3_1_1::connect_packet
 *     - v5::connect_packet
 *   - connack_packet of the Version
 *     - v3_1_1::connack_packet
 *     - v5::connack_packet
 *   - subscribe_packet of the Version
 *     - @ref v3_1_1::basic_subscribe_packet "v3_1_1::subscribe_packet"
 *     - @ref v5::basic_subscribe_packet "v5::subscribe_packet"
 *   - suback_packet of the Version
 *     - @ref v3_1_1::basic_suback_packet "v3_1_1::suback_packet"
 *     - @ref v5::basic_suback_packet "v5::suback_packet"
 *   - unsubscribe_packet of the Version
 *     - @ref v3_1_1::basic_unsubscribe_packet "v3_1_1::unsubscribe_packet"
 *     - @ref v5::basic_unsubscribe_packet "v5::unsubscribe_packet"
 *   - unsuback_packet of the Version
 *     - @ref v3_1_1::basic_unsuback_packet "v3_1_1::unsuback_packet"
 *     - @ref v5::basic_unsuback_packet "v5::unsuback_packet"
 *   - publish_packet of the Version
 *     - @ref v3_1_1::basic_publish_packet "v3_1_1::publish_packet"
 *     - @ref v5::basic_publish_packet "v5::publish_packet"
 *   - puback_packet of the Version
 *     - @ref v3_1_1::basic_puback_packet "v3_1_1::puback_packet"
 *     - @ref v5::basic_puback_packet "v5::puback_packet"
 *   - pubrec_packet of the Version
 *     - @ref v3_1_1::basic_pubrec_packet "v3_1_1::pubrec_packet"
 *     - @ref v5::basic_pubrec_packet "v5::pubrec_packet"
 *   - pubrel_packet of the Version
 *     - @ref v3_1_1::basic_pubrel_packet "v3_1_1::pubrel_packet"
 *     - @ref v5::basic_pubrel_packet "v5::pubrel_packet"
 *   - pubcomp_packet of the Version
 *     - @ref v3_1_1::basic_pubcomp_packet "v3_1_1::pubcomp_packet"
 *     - @ref v5::basic_pubcomp_packet "v5::pubcomp_packet"
 *   - disconnect_packet of the Version
 *     - v3_1_1::disconnect_packet
 *     - v5::disconnect_packet
 *
 * @tparam Version       MQTT protocol version.
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
template <protocol_version Version, typename NextLayer>
class client : public std::enable_shared_from_this<client<Version, NextLayer>> {
    using this_type = client<Version, NextLayer>;
    using this_type_sp = std::shared_ptr<this_type>;

    template <typename T>
    friend class make_shared_helper;

public:
    /// @brief type of endpoint
    using endpoint_type = basic_endpoint<role::client, 2, NextLayer>;

    /// @brief type of the given NextLayer
    using next_layer_type = typename endpoint_type::next_layer_type;

    /// @brief lowest_layer_type of the given NextLayer
    using lowest_layer_type = typename endpoint_type::lowest_layer_type;

    /// @brief executor_type of the given NextLayer
    using executor_type = typename endpoint_type::executor_type;

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

    /**
     * @brief publish completion handler parameter class
     */
    struct pubres_type {
        /// puback_packet as the response when you send QoS1 publish
        /// - @ref v3_1_1::basic_puback_packet "v3_1_1::puback_packet"
        /// - @ref v5::basic_puback_packet "v5::puback_packet"
        std::optional<puback_packet> puback_opt;
        /// pubrec_packet as the response when you send QoS2 publish
        /// - @ref v3_1_1::basic_pubrec_packet "v3_1_1::pubrec_packet"
        /// - @ref v5::basic_pubrec_packet "v5::pubrec_packet"
        std::optional<pubrec_packet> pubrec_opt;
        /// pubcomp_packet as the response when you send QoS2 publish
        /// - @ref v3_1_1::basic_pubcomp_packet "v3_1_1::pubcomp_packet"
        /// - @ref v5::basic_pubcomp_packet "v5::pubcomp_packet"
        std::optional<pubcomp_packet> pubcomp_opt;
    };

    /**
     * @brief create
     * @tparam Args Types for the next layer
     * @param  args args for the next layer.
     * - There are predefined next layer types:
     *    - protocol::mqtt
     *    - protocol::mqtts
     *    - protocol::ws
     *    - protocol::wss
     * @return shared_ptr of client.
     */
    template <typename... Args>
    static std::shared_ptr<this_type> create(
        Args&&... args
    ) {
        return make_shared_helper<this_type>::make_shared(std::forward<Args>(args)...);
    }

    /**
     * @brief copy constructor **deleted**
     */
    client(this_type const&) = delete;

    /**
     * @brief move constructor
     */
    client(this_type&&) = default;

    /**
     * @brief copy assign operator **deleted**
     */
    this_type& operator=(this_type const&) = delete;

    /**
     * @brief move assign operator
     */
    this_type& operator=(this_type&&) = default;

    /**
     * @brief send CONNECT packet and start packet receive loop
     * @param args
     *  - the preceding arguments
     *     - CONNECT packet of the Version or its constructor arguments (like std::vector::emplace_back())
     *        - v3_1_1::connect_packet
     *        - v5::connect_packet
     *  - the last argument
     *     - CompletionToken
     *        - Signature:  void(@ref error_reporting "error_code", std::optional<connack_packet>)
     *           - v3_1_1::connack_packet
     *           - v5::connack_packet
     *           - error_code
     *              - If an error occurs during packet construction
     *                 - If the is CONNECT packet specific error
     *                    - connect_reason_code is set
     *                 - Otherwise
     *                    - disconnect_reason_code is set
     *              - If an error occurs while checking the packet for sending
     *                 - disconnect_reason_code is set
     *              - If an error occurs at an underlying layer while sending a packet
     *                 - underlying error is set. e.g. system, asio, beast, ...
     *              - If the corresponding CONNACK packet is received, you can access connack_packet even if an error is set
     *                 - If the protocol version is v3.1.1
     *                    - connect_return_code is set
     *                 - If the protocol version is v5
     *                    - connect_reason_code is set
     *              - Underlying error
     *                 - TCP, TLS, Websocket related errors
     *              - mqtt_error, connect_reason_code, disconnect_reason_code
     *                 - send packet related errors
     *              - connect_return_code (v3.1.1), connect_reason_code (v5)
     *                 - CONNACK packet response
     *        - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <typename... Args>
    auto async_start(Args&&... args);

    /**
     * @brief send SUBSCRIBE packet
     * @param args
     *  - the preceding arguments
     *     - SUBSCRIBE packet of the Version or its constructor arguments (like std::vector::emplace_back())
     *        - @ref v3_1_1::basic_subscribe_packet "v3_1_1::subscribe_packet"
     *        - @ref v5::basic_subscribe_packet "v5::subscribe_packet"
     *  - the last argument
     *     - CompletionToken
     *        - Signature: void(@ref error_reporting "error_code", std::optional<suback_packet>)
     *           - @ref v3_1_1::basic_suback_packet "v3_1_1::suback_packet"
     *           - @ref v5::basic_suback_packet "v5::suback_packet"
     *           - error_code
     *              - If an error occurs during packet construction
     *                 - disconnect_reason_code is set
     *              - If an error occurs while checking the packet for sending
     *                 - disconnect_reason_code is set
     *              - If an error occurs at an underlying layer while sending a packet
     *                 - underlying error is set. e.g. system, asio, beast, ...
     *              - If the corresponding SUBACK packet is received, you can access suback_packet even if an error is set
     *                 - If the sent SUBSCRIBE packet has a single entry
     *                    - If the protocol version is v3.1.1
     *                       - suback_return_code is set
     *                    - If the protocol version is v5
     *                       - suback_reason_code is set
     *                 - If the sent SUBSCRIBE packet has multiple entries
     *                    - If all entries of the SUBACK packet are errors
     *                       - mqtt_error::all_error_detected is set
     *                    - If some of the entries in the SUBACK packet are errors
     *                       - mqtt_error::partial_error_detected is set
     *                    - If there are no errors in the SUBACK packet
     *                       - errc::success is set
     *
     *        - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <typename... Args>
    auto async_subscribe(Args&&... args);

    /**
     * @brief send UNSUBSCRIBE packet
     * @param args
     *  - the preceding arguments
     *     - UNSUBSCRIBE packet of the Version or its constructor arguments (like std::vector::emplace_back())
     *        - @ref v3_1_1::basic_unsubscribe_packet "v3_1_1::unsubscribe_packet"
     *        - @ref v5::basic_unsubscribe_packet "v5::unsubscribe_packet"
     *  - the last argument
     *     - CompletionToken
     *        - Signature: void(@ref error_reporting "error_code", std::optional<unsuback_packet>)
     *           - @ref v3_1_1::basic_unsuback_packet "v3_1_1::unsuback_packet"
     *           - @ref v5::basic_unsuback_packet "v5::unsuback_packet"
     *           - error_code
     *              - If an error occurs during packet construction
     *                 - disconnect_reason_code is set
     *              - If an error occurs while checking the packet for sending
     *                 - disconnect_reason_code is set
     *              - If an error occurs at an underlying layer while sending a packet
     *                 - underlying error is set. e.g. system, asio, beast, ...
     *              - If the corresponding UNSUBACK packet is received, you can access unsuback_packet even if an error is set
     *                 - If the protocol version is v3.1.1 (UNSUBACK packet has no error field)
     *                    - errc::success is set
     *                 - If the protocol version is v5
     *                    - If the sent UNSUBSCRIBE packet has a single entry
     *                       - unsuback_reason_code is set
     *                 - If the sent UNSUBSCRIBE packet has multiple entries
     *                    - If all entries of the UNSUBACK packet are errors
     *                       - mqtt_error::all_error_detected is set
     *                    - If some of the entries in the UNSUBACK packet are errors
     *                       - mqtt_error::partial_error_detected is set
     *                    - If there are no errors in the UNSUBACK packet
     *                       - errc::success is set
     *
     *        - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <typename... Args>
    auto async_unsubscribe(Args&&... args);

    /**
     * @brief send PUBLISH packet
     * @param args
     *  - the preceding arguments
     *     - PUBLISH packet of the Version or its constructor arguments (like std::vector::emplace_back())
     *        - @ref v3_1_1::basic_publish_packet "v3_1_1::publish_packet"
     *        - @ref v5::basic_publish_packet "v5::publish_packet"
     *  - the last argument
     *     - CompletionToken
     *        - Signature: void(@ref error_reporting "error_code", @link pubres_type @endlink)
     *        - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
     *        - When sending QoS0 packet, all members of pubres_type are std::nullopt.
     *        - When sending QoS1 packet, only pubres_type::puback_opt is set.
     *        - When sending QoS2 packet, only pubres_type::pubrec_opt and pubres_type::pubcomp are set.
     *        - error_code
     *           - If an error occurs during packet construction
     *              - disconnect_reason_code is set
     *           - If an error occurs while checking the packet for sending
     *              - disconnect_reason_code is set
     *           - If an error occurs at an underlying layer while sending a packet
     *              - underlying error is set. e.g. system, asio, beast, ...
     *           - If sent PUBLISH packet is QoS0
     *              - errc::success is set
     *           - If sent PUBLISH packet is QoS1 or QoS2
     *              - If the corresponding response packet is received, you can access the response packet even if an error is set
     *                 - If the protocol version is v3.1.1 (PUBACK, PUBREC, PUBCOMP packet has no error field)
     *                    - errc::success is set
     *                 - If the protocol version is v5
     *                    - If sent PUBLISH packet is QoS1
     *                       - puback_reason_code is set
     *                    - If sent PUBLISH packet is QoS2
     *                       - If PUBREC is error
     *                          - pubrec_reason_code is set
     *                       - Otherwise (after PUBCOMP packet is received)
     *                          - pubcomp_reason_code is set
     *
     * @return deduced by token
     * @par Per-Operation Cancellation
     *
     *  This asynchronous operation supports cancellation for the following
     *  [boost::asio::cancellation_type](https://www.boost.org/doc/html/boost_asio/reference/cancellation_type.html) values:
     *  - cancellation_type::terminal
     *  - cancellation_type::partial
     *
     * if they are also supported by the NextLayer type's async_read_some and async_write_some operation.
     */
    template <typename... Args>
    auto async_publish(Args&&... args);

    /**
     * @brief send DISCONNECT packet
     * @param args
     *  - the preceding arguments
     *     - DISCONNECT packet of the Version or its constructor arguments (like std::vector::emplace_back())
     *        - v3_1_1::disconnect_packet
     *        - v5::disconnect_packet
     *  - the last argument
     *     - CompletionToken
     *        - Signature: void(@ref error_reporting "error_code")
     *        - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
     *        - error_code
     *           - If an error occurs during packet construction
     *              - disconnect_reason_code is set
     *           - If an error occurs while checking the packet for sending
     *              - disconnect_reason_code is set
     *           - If an error occurs at an underlying layer while sending a packet
     *              - underlying error is set. e.g. system, asio, beast, ...
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
    template <typename... Args>
    auto async_disconnect(Args&&... args);

    /**
     * @brief send AUTH packet
     * @param args
     *  - the preceding arguments
     *     - AUTH packet of the Version or its constructor arguments (like std::vector::emplace_back())
     *        - v5::auth_packet
     *  - the last argument
     *     - CompletionToken
     *        - Signature: void(@ref error_reporting "error_code")
     *        - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
     *        - error_code
     *           - If an error occurs during packet construction
     *              - auth_reason_code is set
     *           - If an error occurs while checking the packet for sending
     *              - auth_reason_code is set
     *           - If an error occurs at an underlying layer while sending a packet
     *              - underlying error is set. e.g. system, asio, beast, ...
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
    template <typename... Args>
    auto async_auth(Args&&... args);

    /**
     * @brief close the underlying connection
     * @param token  the param is void
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
     * @brief receive PUBLISH or DISCONNECT packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     * @param token the params are
     *     - CompletionToken
     *        - Signature: void(@ref error_reporting "error_code", packet_variant)
     *          he packet_variant can be one of the following:
     *           - publish_packet
     *              - @ref v3_1_1::basic_publish_packet "v3_1_1::publish_packet"
     *              - @ref v5::basic_publish_packet "v5::publish_packet"
     *           - disconnect_packet
     *              - v3_1_1::disconnect_packet
     *              - v5::disconnect_packet
     *           - v5::auth_packet
     * @return deduced by token
     *
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
        void(error_code, packet_variant)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_recv(
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
     * @brief get endpoint
     *        This is for detail operation
     * @return endpoint
     */
    endpoint_type const& get_endpoint() const;

    /**
     * @brief get endpoint
     *        This is for detail operation
     * @return endpoint
     */
    endpoint_type& get_endpoint();

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
     * @param ms if 0, timer is not set, otherwise set val milliseconds.
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
     * @brief Set the bulk read buffer size.
     * If bulk read is enabled, the `val` parameter specifies the size of the internal
     * `async_read_some()` buffer.
     * Enabling bulk read can improve throughput but may increase latency.
     * Disabling bulk read can reduce latency but may lower throughput.
     * By default, bulk read is disabled.
     *
     * @param val If set to 0, bulk read is disabled. Otherwise, it specifies the buffer size.
     */
    void set_bulk_read_buffer_size(std::size_t val);

    /**
     * @brief acuire unique packet_id.
     * @param token
     *  - CompletionToken
     *     - Signature: void(error_code, packet_id_type)
     *     - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
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
        void(error_code, packet_id_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_acquire_unique_packet_id(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief acuire unique packet_id.
     * If packet_id is fully acquired, then wait until released.
     * @param token
     *  - CompletionToken
     *     - Signature: void(error_code, packet_id_type)
     *     - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
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
        void(error_code, packet_id_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_acquire_unique_packet_id_wait_until(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @param token
     *  - CompletionToken
     *     - Signature: void(error_code)
     *        - If true, success, otherwise the packet_id has already been used.
     *     - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
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
        packet_id_type packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     * @param token
     *  - CompletionToken
     *     - Signature: void()
     *     - [Default Completion Token](https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html) is supported
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
        packet_id_type packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief acuire unique packet_id.
     * @return std::optional<packet_id_type> if acquired return acquired packet id, otherwise std::nullopt
     */
    std::optional<packet_id_type> acquire_unique_packet_id();

    /**
     * @brief register packet_id.
     * @param packet_id packet_id to register
     * @return If true, success, otherwise the packet_id has already been used.
     */
    bool register_packet_id(packet_id_type packet_id);

    /**
     * @brief release packet_id.
     * @param packet_id packet_id to release
     */
    void release_packet_id(packet_id_type packet_id);

    /**
     * @brief rebinds the client type to another executor
     */
    template <typename Executor1>
    struct rebind_executor {
        using other = client<
            Version,
            typename NextLayer::template rebind_executor<Executor1>::other
        >;
    };

private:

    /**
     * @brief constructor
     * @tparam Args Types for the next layer
     * @param  args args for the next layer.
     * - There are predefined next layer types:
     *    - protocol::mqtt
     *    - protocol::mqtts
     *    - protocol::ws
     *    - protocol::wss
     */
    template <typename... Args>
    explicit
    client(
        Args&&... args
    );

    /**
     *  @brief Rebinding constructor
     *         This constructor creates a client from the client with a different executor.
     *  @param other The other client to construct from.
     */
    template <typename Other>
    explicit
    client(
        client<Version, Other>&& other
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, std::optional<connack_packet>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_start_impl(
        error_code ec,
        std::optional<connect_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, std::optional<suback_packet>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_subscribe_impl(
        error_code ec,
        std::optional<subscribe_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, std::optional<suback_packet>)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_unsubscribe_impl(
        error_code ec,
        std::optional<unsubscribe_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
#if !defined(GENERATING_DOCUMENTATION)
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, pubres_type)
    )
#endif // !defined(GENERATING_DOCUMENTATION)
    async_publish_impl(
        error_code ec,
        std::optional<publish_packet> packet,
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
    async_disconnect_impl(
        error_code ec,
        std::optional<disconnect_packet> packet,
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
    async_auth_impl(
        error_code ec,
        std::optional<v5::auth_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    void recv_loop();

    // async operations
    struct start_op;
    struct subscribe_op;
    struct unsubscribe_op;
    struct publish_op;
    struct disconnect_op;
    struct auth_op;
    struct recv_op;

    // internal types
    struct pid_tim_pv_res_col;
    struct recv_type;

    using endpoint_type_sp = std::shared_ptr<endpoint_type>;

    endpoint_type_sp ep_;
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
#include <async_mqtt/impl/client_auth.hpp>
#include <async_mqtt/impl/client_close.hpp>
#include <async_mqtt/impl/client_recv.hpp>
#include <async_mqtt/impl/client_acquire_unique_packet_id.hpp>
#include <async_mqtt/impl/client_acquire_unique_packet_id_wait_until.hpp>
#include <async_mqtt/impl/client_register_packet_id.hpp>
#include <async_mqtt/impl/client_release_packet_id.hpp>

#endif // ASYNC_MQTT_CLIENT_HPP
