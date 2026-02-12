// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_IMPL_HPP)
#define ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_IMPL_HPP

#include <deque>
#include <optional>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>

#include <async_mqtt/asio_bind/detail/client_impl_fwd.hpp>
#include <async_mqtt/asio_bind/detail/client_packet_type_getter.hpp>
#include <async_mqtt/asio_bind/client_fwd.hpp>
#include <async_mqtt/asio_bind/endpoint.hpp>
#include <async_mqtt/protocol/error.hpp>
#include <async_mqtt/protocol/packet/packet_id_type.hpp>
#include <async_mqtt/protocol/packet/packet_variant.hpp>

namespace async_mqtt::detail {

namespace as = boost::asio;
namespace mi = boost::multi_index;

template <protocol_version Version, typename NextLayer>
class client_impl {
    using this_type = client_impl<Version, NextLayer>;
    using this_type_sp = std::shared_ptr<this_type>;
    using client_type = client<Version, NextLayer>;

public:
    using endpoint_type = basic_endpoint<role::client, 2, NextLayer>;
    using next_layer_type = NextLayer;
    using lowest_layer_type = detail::lowest_layer_type<next_layer_type>;
    using executor_type = typename next_layer_type::executor_type;

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

    std::optional<packet_id_type> acquire_unique_packet_id();
    bool register_packet_id(packet_id_type packet_id);
    void release_packet_id(packet_id_type packet_id);

    std::set<packet_id_type> get_qos2_publish_handled_pids() const;
    void restore_qos2_publish_handled_pids(std::set<packet_id_type> pids);
    void restore_packets(std::vector<store_packet_variant> pvs);
    std::vector<store_packet_variant> get_stored_packets() const;

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

    static
    void
    async_acquire_unique_packet_id(
        this_type_sp impl,
        as::any_completion_handler<
            void(error_code, packet_id_type)
        > handler
    );

    static
    void
    async_acquire_unique_packet_id_wait_until(
        this_type_sp impl,
        as::any_completion_handler<
            void(error_code, packet_id_type)
        > handler
    );

    static
    void
    async_register_packet_id(
        this_type_sp impl,
        packet_id_type packet_id,
        as::any_completion_handler<
            void(error_code)
        > handler
    );

    static
    void
    async_release_packet_id(
        this_type_sp impl,
        packet_id_type packet_id,
        as::any_completion_handler<
            void()
        > handler
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_start(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::connect_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    static
    void
    async_start_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::connect_packet> packet,
        as::any_completion_handler<
            void(error_code, std::optional<typename client_type::connack_packet>)
        > handler
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_subscribe(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::subscribe_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    static
    void
    async_subscribe_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::subscribe_packet> packet,
        as::any_completion_handler<
            void(error_code, std::optional<typename client_type::suback_packet>)
        > handler
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_unsubscribe(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::unsubscribe_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    static
    void
    async_unsubscribe_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::unsubscribe_packet> packet,
        as::any_completion_handler<
            void(error_code, std::optional<typename client_type::unsuback_packet>)
        > handler
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_publish(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::publish_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    static
    void
    async_publish_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::publish_packet> packet,
        as::any_completion_handler<
            void(error_code, typename client_type::pubres_type)
        > handler
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_disconnect(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::disconnect_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    static
    void
    async_disconnect_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::disconnect_packet> packet,
        as::any_completion_handler<
            void(error_code)
        > handler
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_auth(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::auth_packet> packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    static
    void
    async_auth_impl(
        this_type_sp impl,
        error_code ec,
        std::optional<typename client_type::auth_packet> packet,
        as::any_completion_handler<
            void(error_code)
        > handler
    );

    static
    void
    async_recv(
        this_type_sp impl,
        as::any_completion_handler<
            void(error_code, std::optional<packet_variant>)
        > handler
    );

    static
    void
    async_close(
        this_type_sp impl,
        as::any_completion_handler<
            void()
        > handler
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
    struct pid_tim_pv_res_col {
        struct elem_type {
            explicit elem_type(
                packet_id_type pid,
                std::shared_ptr<as::steady_timer> tim
            ): pid{pid},
               tim{force_move(tim)}
            {
            }
            explicit elem_type(
                std::shared_ptr<as::steady_timer> tim
            ): tim{force_move(tim)}
            {
            }

            packet_id_type pid = 0;
            std::shared_ptr<as::steady_timer> tim;
            std::optional<packet_variant> pv;
            typename client_type::pubres_type res;
        };

        struct tag_pid {};
        struct tag_tim {};

        auto& get_pid_idx() {
            return elems.template get<tag_pid>();
        }
        auto& get_tim_idx() {
            return elems.template get<tag_tim>();
        }
        void clear() {
            for (auto& elem : elems) {
                const_cast<as::steady_timer&>(*elem.tim).cancel();
            }
            elems.clear();
        }
        using mi_elem_type = mi::multi_index_container<
            elem_type,
            mi::indexed_by<
                mi::ordered_unique<
                    mi::tag<tag_pid>,
                    mi::key<&elem_type::pid>
                >,
                mi::ordered_unique<
                    mi::tag<tag_tim>,
                    mi::key<&elem_type::tim>
                >
            >
        >;
        mi_elem_type elems;
    };

    struct recv_type {
        explicit recv_type(packet_variant packet)
            :pv{force_move(packet)}
        {
        }
        explicit recv_type(error_code ec)
            :ec{ec}
        {
        }
        error_code ec = error_code{};
        std::optional<packet_variant> pv;
    };

    endpoint_type ep_;
    pid_tim_pv_res_col pid_tim_pv_res_col_;
    std::deque<recv_type> recv_queue_;
    bool recv_queue_inserted_ = false;
    as::steady_timer tim_notify_publish_recv_;
};

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_IMPL_HPP
