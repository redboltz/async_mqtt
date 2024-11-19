// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_DETAIL_CLIENT_IMPL_HPP)
#define ASYNC_MQTT_DETAIL_CLIENT_IMPL_HPP

#include <deque>
#include <optional>

#include <boost/asio/async_result.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>

#include <async_mqtt/error.hpp>
#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/endpoint_fwd.hpp>
#include <async_mqtt/client_fwd.hpp>
#include <async_mqtt/detail/client_packet_type_getter.hpp>

namespace async_mqtt::detail {

namespace as = boost::asio;

template <protocol_version Version, typename NextLayer>
class client_impl {
    using this_type = client_impl<Version, NextLayer>;
    using this_type_sp = std::shared_ptr<this_type>;
    using client_type = client<Version, NextLayer>;

public:
    using endpoint_type = basic_endpoint<role::client, 2, NextLayer>;
    using next_layer_type = typename endpoint_type::next_layer_type;
    using lowest_layer_type = typename endpoint_type::lowest_layer_type;
    using executor_type = typename endpoint_type::executor_type;

    template <typename... Args>
    explicit
    client_impl(
        Args&&... args
    );
    client_impl(this_type const&) = delete;
    client_impl(this_type&&) = delete;
    ~client_impl() = default;

    this_type& operator=(this_type const&) = delete;
    this_type& operator=(this_type&&) = delete;

    template <
        typename... Args
    >
    auto
    async_underlying_handshake(
        Args&&... args
    );

    template <typename... Args>
    auto async_start(Args&&... args);

    template <typename... Args>
    auto async_subscribe(Args&&... args);

    template <typename... Args>
    auto async_unsubscribe(Args&&... args);

    template <typename... Args>
    auto async_publish(Args&&... args);

    template <typename... Args>
    auto async_disconnect(Args&&... args);

    template <typename... Args>
    auto async_auth(Args&&... args);

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_close(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_recv(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    as::any_io_executor get_executor();
    next_layer_type const& next_layer() const;
    next_layer_type& next_layer();
    lowest_layer_type const& lowest_layer() const;
    lowest_layer_type& lowest_layer();
    endpoint_type const& get_endpoint() const;
    endpoint_type& get_endpoint();

    void set_auto_map_topic_alias_send(bool val);
    void set_auto_replace_topic_alias_send(bool val);
    void set_pingresp_recv_timeout(std::chrono::milliseconds duration);
    void set_close_delay_after_disconnect_sent(std::chrono::milliseconds duration);
    void set_bulk_write(bool val);
    void set_read_buffer_size(std::size_t val);

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_acquire_unique_packet_id(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_acquire_unique_packet_id_wait_until(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_register_packet_id(
        packet_id_type packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_release_packet_id(
        packet_id_type packet_id,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    std::optional<packet_id_type> acquire_unique_packet_id();
    bool register_packet_id(packet_id_type packet_id);
    void release_packet_id(packet_id_type packet_id);

    template <typename Executor1>
    struct rebind_executor {
        using other = client_impl<
            Version,
            typename NextLayer::template rebind_executor<Executor1>::other
        >;
    };

private:
    friend class client<Version, NextLayer>;

    template <typename Other>
    explicit
    client_impl(
        client_impl<Version, Other>&& other
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_start_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::connect_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_subscribe_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::subscribe_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_unsubscribe_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::unsubscribe_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_publish_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::publish_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_disconnect_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::disconnect_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_auth_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<v5::auth_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    static void recv_loop(this_type_sp impl);

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

    endpoint_type ep_;
    pid_tim_pv_res_col pid_tim_pv_res_col_;
    std::deque<recv_type> recv_queue_;
    bool recv_queue_inserted_ = false;
    as::steady_timer tim_notify_publish_recv_;
};

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_DETAIL_CLIENT_IMPL_HPP
