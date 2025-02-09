// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_IMPL_STREAM_IMPL_HPP)
#define ASYNC_MQTT_ASIO_BIND_IMPL_STREAM_IMPL_HPP

#include <utility>
#include <type_traits>
#include <deque>

#include <boost/asio/async_result.hpp>

#include <async_mqtt/asio_bind/detail/stream_layer.hpp>
#include <async_mqtt/asio_bind/stream_customize.hpp>
#include <async_mqtt/asio_bind/impl/stream_fwd.hpp>
#include <async_mqtt/protocol/error.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/ioc_queue.hpp>
#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt::detail {
namespace as = boost::asio;
namespace sys = boost::system;

template <typename NextLayer>
class stream_impl {
public:
    using this_type = stream_impl<NextLayer>;
    using this_type_sp = std::shared_ptr<this_type>;
    using next_layer_type = NextLayer;
    using lowest_layer_type = detail::lowest_layer_type<next_layer_type>;
    using executor_type = typename next_layer_type::executor_type;

    // constructor
    template <
        typename T,
        typename... Args,
        std::enable_if_t<!std::is_same_v<std::decay_t<T>, this_type>>* = nullptr
    >
    explicit
    stream_impl(T&& t, Args&&... args)
        :nl_{std::forward<T>(t), std::forward<Args>(args)...}
    {
        initialize(nl_);
    }

    template <typename Other>
    explicit
    stream_impl(
        stream_impl<Other>&& other
    )
        :nl_{force_move(other.nl_)}
    {
        initialize(nl_);
    }

    ~stream_impl() {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "destroy";
    }

    stream_impl(this_type&&) = delete;
    stream_impl(this_type const&) = delete;
    this_type& operator=(this_type&&) = delete;
    this_type& operator=(this_type const&) = delete;

    next_layer_type const& next_layer() const {
        return nl_;
    }
    next_layer_type& next_layer() {
        return nl_;
    }

    lowest_layer_type const& lowest_layer() const {
        return get_lowest_layer(nl_);
    }
    lowest_layer_type& lowest_layer() {
        return get_lowest_layer(nl_);
    }

    as::any_io_executor get_executor() {
        return nl_.get_executor();
    };

    void set_bulk_write(bool val) {
        bulk_write_ = val;
    }

    template <typename Executor1>
    struct rebind_executor {
        using other = stream_impl<
            typename NextLayer::template rebind_executor<Executor1>::other
        >;
    };

private:
    template <typename Layer>
    static void initialize(Layer& layer) {
        if constexpr (has_next_layer<Layer>::value) {
            initialize(layer.next_layer());
        }
        if constexpr(has_initialize<Layer>::value) {
            layer_customize<Layer>::initialize(layer);
        }
    }

    void init_read();

    void parse_packet();

    template <
        typename... Args
    >
    static auto
    async_underlying_handshake(
        this_type_sp impl,
        Args&&... args
    );

    // async operations

    template <typename Packet>
    struct stream_write_packet_op;
    template <typename MutableBufferSequence>
    struct stream_read_some_op;
    struct stream_close_op;

private:
    friend class stream<NextLayer>;
    struct error_packet {
        error_packet(error_code ec)
            :ec{ec} {}
        error_packet(buffer packet)
            :packet{force_move(packet)} {}

        error_code ec;
        buffer packet;
    };

    next_layer_type nl_;
    ioc_queue read_queue_;
    ioc_queue write_queue_;
    struct stream_read_op;
    std::vector<as::const_buffer> storing_cbs_;
    std::vector<as::const_buffer> sending_cbs_;
    bool bulk_write_ = false;
};

} // namespace async_mqtt::detail

#endif // ASYNC_MQTT_ASIO_BIND_IMPL_STREAM_IMPL_HPP
