// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_DETAIL_ENDPOINT_IMPL_HPP)
#define ASYNC_MQTT_ASIO_BIND_DETAIL_ENDPOINT_IMPL_HPP

#include <set>
#include <deque>

#include <async_mqtt/asio_bind/detail/endpoint_impl_fwd.hpp>
#include <async_mqtt/asio_bind/endpoint_fwd.hpp>
#include <async_mqtt/asio_bind/filter.hpp>
#include <async_mqtt/protocol/error.hpp>
#include <async_mqtt/protocol/rv_connection.hpp>
#include <async_mqtt/protocol/packet/packet_variant.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>
#include <async_mqtt/protocol/protocol_version.hpp>
#include <async_mqtt/protocol/role.hpp>
#include <async_mqtt/asio_bind/impl/stream.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt::detail {

template <role Role, std::size_t PacketIdBytes, typename NextLayer>
class basic_endpoint_impl {

    using this_type = basic_endpoint_impl<Role, PacketIdBytes, NextLayer>;
    using this_type_sp = std::shared_ptr<this_type>;
    using this_type_wp = std::weak_ptr<this_type>;
    using stream_type =
        stream<
            NextLayer
        >;

public:
    using next_layer_type = typename stream_type::next_layer_type;
    using lowest_layer_type = typename stream_type::lowest_layer_type;
    using executor_type = typename next_layer_type::executor_type;
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;

    template <typename... Args>
    explicit
    basic_endpoint_impl(
        protocol_version ver,
        Args&&... args
    );

    template <typename... Args>
    explicit
    basic_endpoint_impl(
        typename basic_packet_id_type<PacketIdBytes>::type packet_id_max,
        protocol_version ver,
        Args&&... args
    );

    ~basic_endpoint_impl() = default;
    basic_endpoint_impl(this_type const&) = delete;
    basic_endpoint_impl(this_type&&) = delete;

    this_type& operator=(this_type const&) = delete;
    this_type& operator=(this_type&&) = delete;

    as::any_io_executor get_executor();
    next_layer_type const& next_layer() const;
    next_layer_type& next_layer();
    lowest_layer_type const& lowest_layer() const;
    lowest_layer_type& lowest_layer();

    void underlying_accepted();
    void set_offline_publish(bool val);
    void set_auto_pub_response(bool val);
    void set_auto_ping_response(bool val);
    void set_auto_map_topic_alias_send(bool val);
    void set_auto_replace_topic_alias_send(bool val);
    void set_pingresp_recv_timeout(std::chrono::milliseconds duration);
    void set_close_delay_after_disconnect_sent(std::chrono::milliseconds duration);
    void set_bulk_write(bool val);
    void set_read_buffer_size(std::size_t val);

    // async funcs
    static void
    async_acquire_unique_packet_id(
        this_type_sp impl,
        as::any_completion_handler<
            void(error_code, typename basic_packet_id_type<PacketIdBytes>::type)
        > handler
    );

    static void
    async_acquire_unique_packet_id_wait_until(
        this_type_sp impl,
        as::any_completion_handler<
            void(error_code, typename basic_packet_id_type<PacketIdBytes>::type)
        > handler
    );

    static void
    async_register_packet_id(
        this_type_sp impl,
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        as::any_completion_handler<
            void(error_code)
        > handler
    );

    static void
    async_release_packet_id(
        this_type_sp impl,
        typename basic_packet_id_type<PacketIdBytes>::type packet_id,
        as::any_completion_handler<
            void()
        > handler
    );

    static void
    async_recv(
        this_type_sp impl,
        std::optional<filter> fil,
        std::set<control_packet_type> types,
        as::any_completion_handler<
            void(error_code, std::optional<packet_variant_type>)
        > handler
    );

    static void
    async_get_stored_packets(
        this_type_sp impl,
        as::any_completion_handler<
            void(error_code, std::vector<basic_store_packet_variant<PacketIdBytes>>)
        > handler
    );

    static void
    async_regulate_for_store(
        this_type_sp impl,
        v5::basic_publish_packet<PacketIdBytes> packet,
        as::any_completion_handler<
            void(error_code, v5::basic_publish_packet<PacketIdBytes>)
        > handler
    );

    static void
    async_restore_packets(
        this_type_sp impl,
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs,
        as::any_completion_handler<
            void()
        > handler
    );

    // sync funcs

    std::optional<typename basic_packet_id_type<PacketIdBytes>::type> acquire_unique_packet_id();
    bool register_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id);
    void release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id);
    std::set<typename basic_packet_id_type<PacketIdBytes>::type> get_qos2_publish_handled_pids() const;
    void restore_qos2_publish_handled_pids(std::set<typename basic_packet_id_type<PacketIdBytes>::type> pids);
    void restore_packets(
        std::vector<basic_store_packet_variant<PacketIdBytes>> pvs
    );
    std::vector<basic_store_packet_variant<PacketIdBytes>> get_stored_packets() const;
    protocol_version get_protocol_version() const;
    bool is_publish_processing(typename basic_packet_id_type<PacketIdBytes>::type pid) const;
    void regulate_for_store(
        v5::basic_publish_packet<PacketIdBytes>& packet,
        error_code& ec
    ) const;
    static void set_pingreq_send_interval(
        this_type_sp ep,
        std::chrono::milliseconds duration
    );

    template <typename Executor1>
    struct rebind_executor {
        using other = basic_endpoint<
            Role,
            PacketIdBytes,
            typename NextLayer::template rebind_executor<Executor1>::other
        >;
    };

    template <
        typename ArgsTuple,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_underlying_handshake_impl(
        this_type_sp impl,
        ArgsTuple&& args_tuple,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

private: // compose operation impl

    template <typename Other>
    explicit
    basic_endpoint_impl(
        basic_endpoint_impl<Role, PacketIdBytes, Other>&& other
    );

    template <typename ArgsTuple>  struct underlying_handshake_op;
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
    static
    auto
    async_send(
        this_type_sp impl,
        Packet packet,
        bool from_queue,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename Packet,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_send(
        this_type_sp impl,
        Packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    static
    auto
    async_close(
        this_type_sp impl,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    static void
    async_close_impl(
        this_type_sp impl,
        as::any_completion_handler<
            void()
        > handler
    );

    static
    void
    async_add_retry(
        this_type_sp impl,
        as::any_completion_handler<
            void(error_code)
        > handler
    );

    bool enqueue_publish(v5::basic_publish_packet<PacketIdBytes>& packet);
    void initialize();

    static void reset_pingreq_send_timer(
        this_type_sp ep,
        std::optional<std::chrono::milliseconds> ms
    );
    static void cancel_pingreq_send_timer(
        this_type_sp ep
    );

    static void reset_pingreq_recv_timer(
        this_type_sp ep,
        std::optional<std::chrono::milliseconds> ms
    );
    static void cancel_pingreq_recv_timer(
        this_type_sp ep
    );

    static void reset_pingresp_recv_timer(
        this_type_sp ep,
        std::optional<std::chrono::milliseconds> ms
    );

    static void cancel_pingresp_recv_timer(
        this_type_sp ep
    );

    void notify_retry_one();
    void complete_retry_one();
    void notify_retry_all();
    bool has_retry() const;

    void clear_pid_man();
    void notify_release_pid(typename basic_packet_id_type<PacketIdBytes>::type pid);

private:

    friend class basic_endpoint<Role, PacketIdBytes, NextLayer>;
    stream_type stream_;
    std::size_t read_buffer_size_ = 65535; // TBD define constant
    as::streambuf read_buf_;
    std::istream is_{&read_buf_};
    basic_rv_connection<Role, PacketIdBytes> con_;

    std::deque<v5::basic_publish_packet<PacketIdBytes>> publish_queue_;
    ioc_queue close_queue_;

    as::steady_timer tim_pingreq_send_;
    as::steady_timer tim_pingreq_recv_;
    as::steady_timer tim_pingresp_recv_;
    as::steady_timer tim_close_by_disconnect_;
    std::chrono::milliseconds duration_close_by_disconnect_{std::chrono::milliseconds::zero()};
    struct tim_cancelled;
    std::deque<tim_cancelled> tim_retry_acq_pid_queue_;
    bool packet_id_released_ = false;

    enum class close_status {
        open,
        closing,
        closed
    };
    close_status status_{close_status::closed};
    std::deque<basic_event_variant<PacketIdBytes>> recv_events_;
};

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_ASIO_BIND_DETAIL_ENDPOINT_IMPL_HPP
