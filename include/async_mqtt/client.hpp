// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CLIENT_HPP)
#define ASYNC_MQTT_CLIENT_HPP

#include <optional>
#include <boost/asio/async_result.hpp>
#include <boost/asio/any_io_executor.hpp>

#include <async_mqtt/detail/client_impl_fwd.hpp>
#include <async_mqtt/detail/client_packet_type_getter.hpp>
#include <async_mqtt/detail/stream_layer.hpp>
#include <async_mqtt/protocol/error.hpp>
#include <async_mqtt/protocol/role.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>
#include <async_mqtt/endpoint_fwd.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <protocol_version Version, typename NextLayer>
class client {
    using this_type = client<Version, NextLayer>;
    using impl_type = detail::client_impl<Version, NextLayer>;

public:
    /// @brief type of endpoint
    using endpoint_type = basic_endpoint<role::client, 2, NextLayer>;

    /// @brief type of the given NextLayer
    using next_layer_type = NextLayer;

    /// @brief lowest_layer_type of the given NextLayer
    using lowest_layer_type = detail::lowest_layer_type<next_layer_type>;

    /// @brief executor_type of the given NextLayer
    using executor_type = typename next_layer_type::executor_type;

    /// @brief connect packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::connect_packet.
    /// if Version is v5 the type is @ref v5::connect_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, connect)

    /// @brief connack packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::connack_packet.
    /// if Version is v5 the type is @ref v5::connack_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, connack)

    /// @brief subscribe packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::subscribe_packet.
    /// if Version is v5 the type is @ref v5::subscribe_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, subscribe)

    /// @brief suback packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::suback_packet.
    /// if Version is v5 the type is @ref v5::suback_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, suback)

    /// @brief unsubscribe packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::unsubscribe_packet.
    /// if Version is v5 the type is @ref v5::unsubscribe_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, unsubscribe)

    /// @brief unsuback packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::unsuback_packet.
    /// if Version is v5 the type is @ref v5::unsuback_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, unsuback)

    /// @brief publish packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::publish_packet.
    /// if Version is v5 the type is @ref v5::publish_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, publish)

    /// @brief puback packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::puback_packet.
    /// if Version is v5 the type is @ref v5::puback_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, puback)

    /// @brief pubrec packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::pubrec_packet.
    /// if Version is v5 the type is @ref v5::pubrec_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, pubrec)

    /// @brief pubrel packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::pubrel_packet.
    /// if Version is v5 the type is @ref v5::pubrel_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, pubrel)

    /// @brief pubcomp packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::pubcomp_packet.
    /// if Version is v5 the type is @ref v5::pubcomp_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, pubcomp)

    /// @brief pingreq packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::pingreq_packet.
    /// if Version is v5 the type is @ref v5::pingreq_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, pingreq)

    /// @brief pingresp packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::pingresp_packet.
    /// if Version is v5 the type is @ref v5::pingresp_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, pingresp)

    /// @brief disconnect packet type
    ///
    /// if Version is v3.1.1 the type is @ref v3_1_1::disconnect_packet.
    /// if Version is v5 the type is @ref v5::disconnect_packet.
    ASYNC_MQTT_PACKET_TYPE(Version, disconnect)

    /// @brief auth packet type
    ///
    /// type alias of v5::auth_packet
    using auth_packet = v5::auth_packet;

    /**
     * @brief publish completion handler parameter class
     */
    struct pubres_type {
        /// puback_packet as the response when you send QoS1 publish
        /// @li @ref v3_1_1::basic_puback_packet "v3_1_1::puback_packet"
        /// @li @ref v5::basic_puback_packet "v5::puback_packet"
        std::optional<puback_packet> puback_opt;
        /// pubrec_packet as the response when you send QoS2 publish
        /// @li @ref v3_1_1::basic_pubrec_packet "v3_1_1::pubrec_packet"
        /// @li @ref v5::basic_pubrec_packet "v5::pubrec_packet"
        std::optional<pubrec_packet> pubrec_opt;
        /// pubcomp_packet as the response when you send QoS2 publish
        /// @li @ref v3_1_1::basic_pubcomp_packet "v3_1_1::pubcomp_packet"
        /// @li @ref v5::basic_pubcomp_packet "v5::pubcomp_packet"
        std::optional<pubcomp_packet> pubcomp_opt;
    };

    /**
     * @brief constructor
     * @tparam Args Types for the next layer
     * @param  args args for the next layer.
     *
     */
    template <typename... Args>
    explicit
    client(
        Args&&... args
    );

    /**
     * @brief copy constructor **deleted**
     */
    client(this_type const&) = delete;

    /**
     * @brief move constructor
     */
    client(this_type&&) = default;

    /**
     * @brief destructor
     * This function destroys the client,
     * cancelling any outstanding asynchronous operations associated with the client.
     */
    ~client() = default;

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
     * @param args see <a href="#_parameter_detail">parameter detail</a>
     * @return deduced by token
     *
     * ## parameter detail
     *
     * ### 1st..N-1th parameters
     * CONNECT packet of the Version or its constructor arguments (like std::vector::emplace_back())
     * @li @ref v3_1_1::connect_packet
     * @li @ref v5::connect_packet
     *
     * ### Nth(the last) parameter
     * CompletionToken
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, std::optional<@ref connack_packet>)
     *
     * ##### error_code and optional<connack_packet>
     *
     * @li If an error occurs during packet construction,
     *     and if it is a CONNECT packet specific error,
     *     @ref connect_reason_code is set.
     *     Otherwise, @ref disconnect_reason_code is set.
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during packet construction,
     *     but an error occurs while checking the packet for sending,
     *     @ref disconnect_reason_code is set.
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during checking the packet for sending,
     *     but an error occurs at an underlying layer while sending a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during the packet for sending,
     *     and the corresponding CONNACK packet is received,
     *     @ref connect_return_code and optional<connack_packet> is set to @ref v3_1_1::connack_packet,
     *     if the protocol version is v3.1.1.
     *     @ref connect_reason_code and optional<connack_packet> is set to @ref v5::connack_packet
     *     if the protocol version is v5.
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
    template <typename... Args>
    auto async_start(Args&&... args);

    /**
     * @brief send SUBSCRIBE packet
     * @param args see <a href="#_parameter_detail">parameter detail</a>
     * @return deduced by token
     *
     * ## parameter detail
     *
     * ### 1st..N-1th parameters
     * SUBSCRIBE packet of the Version or its constructor arguments (like std::vector::emplace_back())
     * @li @ref v3_1_1::subscribe_packet
     * @li @ref v5::subscribe_packet
     *
     * ### Nth(the last) parameter
     * CompletionToken
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, std::optional<@ref suback_packet>)
     *
     * ##### error_code and optional<suback_packet>
     * @li If an error occurs during packet construction,
     *     @ref disconnect_reason_code is set.
     *     optional<suback_packet> is set to nullopt.
     * @li If no error occurs during packet construction,
     *     but an error occurs while checking the packet for sending,
     *     @ref disconnect_reason_code is set.
     *     optional<suback_packet> is set to nullopt.
     * @li If no error occurs during checking the packet for sending,
     *     but an error occurs at an underlying layer while sending a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     optional<suback_packet> is set to nullopt.
     * @li If no error occurs during the packet for sending,
     *     and the corresponding SUBACK packet is received,
     *     optional<suback_packet> is set to @ref v3_1_1::suback_packet,
     *     if the protocol version is v3.1.1.
     *     optional<suback_packet> is set to @ref v5::suback_packet,
     *     if the protocol version is v5.
     *     If sent SUBSCRIBE packet has single entry,
     *     @ref suback_return_code is set if the protocol version is v3.1.1.
     *     @ref suback_reason_code is set if the protocol version is v5.
     *     If sent SUBSCRIBE packet has multiple entries,
     *     and if the all entries of the SUBACK packet are errors,
     *     @ref mqtt_error::all_error_detected is set.
     *     If some of the entries in the SUBACK packet are errors,
     *     @ref mqtt_error::partial_error_detected is set.
     *     If there are no errors in the SUBACK packet,
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
    template <typename... Args>
    auto async_subscribe(Args&&... args);

    /**
     * @brief send UNSUBSCRIBE packet
     * @param args see <a href="#_parameter_detail">parameter detail</a>
     * @return deduced by token
     *
     * ## parameter detail
     *
     * ### 1st..N-1th parameters
     * UNSUBSCRIBE packet of the Version or its constructor arguments (like std::vector::emplace_back())
     * @li @ref v3_1_1::unsubscribe_packet
     * @li @ref v5::unsubscribe_packet
     *
     * ### Nth(the last) parameter
     * CompletionToken
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, std::optional<@ref unsuback_packet>)
     *
     * ##### error_code and optional<unsuback_packet>
     * @li If an error occurs during packet construction,
     *     @ref disconnect_reason_code is set.
     *     optional<unsuback_packet> is set to nullopt.
     * @li If no error occurs during packet construction,
     *     but an error occurs while checking the packet for sending,
     *     @ref disconnect_reason_code is set.
     *     optional<unsuback_packet> is set to nullopt.
     * @li If no error occurs during checking the packet for sending,
     *     but an error occurs at an underlying layer while sending a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     optional<unsuback_packet> is set to nullopt.
     * @li If no error occurs during the packet for sending,
     *     and the corresponding UNSUBACK packet is received,
     *     optional<unsuback_packet> is set to @ref v3_1_1::unsuback_packet,
     *     if the protocol version is v3.1.1.
     *     optional<unsuback_packet> is set to @ref v5::unsuback_packet,
     *     if the protocol version is v5.
     *     If sent UNSUBSCRIBE packet has single entry,
     *     @ref unsuback_return_code is set if the protocol version is v3.1.1.
     *     @ref unsuback_reason_code is set if the protocol version is v5.
     *     If sent UNSUBSCRIBE packet has multiple entries,
     *     and if the all entries of the UNSUBACK packet are errors,
     *     @ref mqtt_error::all_error_detected is set.
     *     If some of the entries in the UNSUBACK packet are errors,
     *     @ref mqtt_error::partial_error_detected is set.
     *     If there are no errors in the UNSUBACK packet,
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
    template <typename... Args>
    auto async_unsubscribe(Args&&... args);

    /**
     * @brief send PUBLISH packet
     * @param args see <a href="#_parameter_detail">parameter detail</a>
     * @return deduced by token
     *
     * ## parameter detail
     *
     * ### 1st..N-1th parameters
     * PUBLISH packet of the Version or its constructor arguments (like std::vector::emplace_back())
     * @li @ref v3_1_1::publish_packet
     * @li @ref v5::publish_packet
     *
     * ### Nth(the last) parameter
     * CompletionToken
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, @ref pubres_type)
     *
     * ##### pubres_type
     * @li When sending QoS0 packet, all members of pubres_type are nullopt.
     * @li When sending QoS1 packet, only @ref pubres_type::puback_opt is set.
     * @li When sending QoS2 packet, only @ref pubres_type::pubrec_opt and @ref pubres_type::pubcomp are set.
     *
     * ##### error_code
     *
     * @li If an error occurs during packet construction,
     *     @ref disconnect_reason_code is set.
     *     All members of pubres_type are set to nullopt.
     * @li If no error occurs during packet construction,
     *     but an error occurs while checking the packet for sending,
     *     @ref disconnect_reason_code is set.
     *     All members of pubres_type are set to nullopt.
     * @li If no error occurs during checking the packet for sending,
     *     but an error occurs at an underlying layer while sending a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     All members of pubres_type are set to nullopt.
     * @li If no error occurs during the packet for sending,
     *     and if sent PUBLISH packet is QoS0,
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set,
     *     if sent PUBLISH packet is QoS1 or QoS2,
     *     and corresponding response packet is received,
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set
     *     if the protocol version is v3.1.1.
     *     (On v3.1.1, PUBACK, PUBREC, PUBCOMP packet has no error field.)
     *     If sent PUBLISH packet is QoS1, @ref puback_reason_code if the protocol version is v5.
     *     If sent PUBLISH packet is QoS2, @ref pubrec_reason_code as error if the protocol version is v5.
     *     If sent PUBLISH packet is QoS2, @ref pubcompc_reason_code if the protocol version is v5.
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
    template <typename... Args>
    auto async_publish(Args&&... args);

    /**
     * @brief send DISCONNECT packet
     * @param args see <a href="#_parameter_detail">parameter detail</a>
     * @return deduced by token
     *
     * ## parameter detail
     *
     * ### 1st..N-1th parameters
     * DISCONNECT packet of the Version or its constructor arguments (like std::vector::emplace_back())
     * @li @ref v3_1_1::disconnect_packet
     * @li @ref v5::disconnect_packet
     *
     * ### Nth(the last) parameter
     * CompletionToken
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code)
     *
     * ##### error_code
     *
     * @li If an error occurs during packet construction,
     *     and if it is a DISCONNECT packet specific error,
     *     @ref disconnect_reason_code is set.
     *     Otherwise, @ref disconnect_reason_code is set.
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during packet construction,
     *     but an error occurs while checking the packet for sending,
     *     @ref disconnect_reason_code is set.
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during checking the packet for sending,
     *     but an error occurs at an underlying layer while sending a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during the packet for sending,
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
    template <typename... Args>
    auto async_disconnect(Args&&... args);

    /**
     * @brief send AUTH packet
     * @param args see <a href="#_parameter_detail">parameter detail</a>
     * @return deduced by token
     *
     * ## parameter detail
     *
     * ### 1st..N-1th parameters
     * AUTH packet of the Version or its constructor arguments (like std::vector::emplace_back())
     * @li @ref v5::auth_packet
     *
     * ### Nth(the last) parameter
     * CompletionToken
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code)
     *
     * ##### error_code
     *
     * @li If an error occurs during packet construction,
     *     and if it is a AUTH packet specific error,
     *     @ref disconnect_reason_code is set.
     *     Otherwise, @ref disconnect_reason_code is set.
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during packet construction,
     *     but an error occurs while checking the packet for sending,
     *     @ref disconnect_reason_code is set.
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during checking the packet for sending,
     *     but an error occurs at an underlying layer while sending a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     optional<connack_packet> is set to nullopt.
     * @li If no error occurs during the packet for sending,
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
    template <typename... Args>
    auto async_auth(Args&&... args);

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
    async_close(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    /**
     * @brief receive PUBLISH or DISCONNECT packet
     *        users CANNOT call recv() before the previous recv()'s CompletionToken is invoked
     * @param token see Signature
     * @return deduced by token
     *
     * ### Completion Token
     * @li <a href="https://www.boost.org/doc/html/boost_asio/overview/composition/token_adapters.html">Default Completion Token</a> is supported
     *
     * #### Signature
     * void(@ref error_code, std::optional<@ref packet_variant_type>)
     *
     * ##### error_code and packet_variant_type
     * @li If an error occurs at an underlying layer while receiving a packet,
     *     underlying error is set. e.g. system, asio, beast, ...
     *     std::optional<@ref packet_variant_type> is set to std::nullopt.
     * @li If there are no errors during receiving the packet,
     *     <a href="https://www.boost.org/libs/system/doc/html/system.html#ref_errc">errc::success</a> is set.
     *     std::optional<@ref packet_variant_type> is set to one of the following @ref basic_packet_variant:
     * @li @ref v3_1_1::publish_packet, if Version is v3_1_1 and PUBLISH packet is received.
     * @li @ref v5::publish_packet, if Version is v5 and PUBLISH packet is received.
     * @li @ref v5::disconnect_packet, if Version is v5 and DISCONNECT packet is received.
     * @li @ref v5::auth_packet, if Version is v5 and AUTH packet is received.
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
     * @brief executor getter
     * @return return endpoint's  executor.
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
     * \n This function should be called before async_start() call.
     * @note By default not automatically mapping.
     * @param val if true, enable auto mapping, otherwise disable.
     */
    void set_auto_map_topic_alias_send(bool val);

    /**
     * @brief auto replace topic with corresponding topic alias on send PUBLISH packet.
     * Registering topic alias need to do manually.
     * \n This function should be called before async_start() call.
     * @note By default not automatically replacing.
     * @param val if true, enable auto replacing, otherwise disable.
     */
    void set_auto_replace_topic_alias_send(bool val);

    /**
     * @brief Set timeout for receiving PINGRESP packet after PINGREQ packet is sent.
     * If the timer is fired, then the underlying layer is closed from the client side.
     * If the protocol_version is v5, then send DISCONNECT packet with the reason code
     * disconnect_reason_code::keep_alive_timeout automatically before underlying layer is closed.
     * \n This function should be called before async_start() call.
     * @note By default timeout is not set.
     * @param duration if zero, timer is not set; otherwise duration is set.
     *                 The minimum resolution is in milliseconds.
     */
    void set_pingresp_recv_timeout(std::chrono::milliseconds duration);

    /**
     * @brief Sets the delay duration for closing the stream after sending the DISCONNECT packet.
     * If the timer expires, the underlying layer stream will begin closing.
     * \n This function should be called before async_start() call.
     * @note By default, no delay is set.
     * @param duration If set to zero, the timer is not activated, and the close process starts immediately.
     *                 Otherwise, the close process begins after the specified duration has elapsed.
     *                 The minimum resolution is in milliseconds.
     */
    void set_close_delay_after_disconnect_sent(std::chrono::milliseconds duration);

    /**
     * @brief Set bulk write mode.
     * If true, then concatenate multiple packets' const buffer sequence
     * when send() is called before the previous send() is not completed.
     * Otherwise, send packet one by one.
     * \n This function should be called before async_start() call.
     * @note By default bulk write mode is false (disabled)
     * @param val if true, enable bulk write mode, otherwise disable it.
     */
    void set_bulk_write(bool val);

    /**
     * @brief Set read buffer size.
     * If bulk read is enabled, the `val` parameter specifies the size of the internal
     * prepared `async_read_some()` streambuf.
     * By default, read buffer size is 65535.
     *
     * @param val If set to 0, bulk read is disabled. Otherwise, it specifies the buffer size.
     */
    void set_read_buffer_size(std::size_t val);

    // TBD doc later
    template <
        typename... Args
    >
    auto
    async_underlying_handshake(
        Args&&... args
    );

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
     *
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
        packet_id_type packet_id,
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
    auto
    async_start_impl(
        error_code ec,
        std::optional<connect_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_subscribe_impl(
        error_code ec,
        std::optional<subscribe_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_unsubscribe_impl(
        error_code ec,
        std::optional<unsubscribe_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_publish_impl(
        error_code ec,
        std::optional<publish_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_disconnect_impl(
        error_code ec,
        std::optional<disconnect_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_auth_impl(
        error_code ec,
        std::optional<v5::auth_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    void recv_loop();

private:
    std::shared_ptr<impl_type> impl_;
};

} // namespace async_mqtt

#include <async_mqtt/impl/client_misc.hpp>
#include <async_mqtt/impl/client_underlying_handshake.hpp>
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
