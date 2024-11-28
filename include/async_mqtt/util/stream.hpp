// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_STREAM_HPP)
#define ASYNC_MQTT_UTIL_STREAM_HPP

#include <utility>
#include <type_traits>
#include <deque>

#include <boost/asio/async_result.hpp>

#include <async_mqtt/util/stream_traits.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/ioc_queue.hpp>
#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/protocol/error.hpp>
#include <async_mqtt/util/log.hpp>

#include <async_mqtt/util/detail/stream_impl.hpp>

namespace async_mqtt {
namespace as = boost::asio;
namespace sys = boost::system;

template <typename NextLayer>
class stream {
public:
    using this_type = stream<NextLayer>;
    using impl_type = detail::stream_impl<NextLayer>;
    using next_layer_type = typename std::remove_reference<NextLayer>::type;
    using lowest_layer_type =
        typename std::remove_reference<
            decltype(get_lowest_layer(std::declval<next_layer_type&>()))
        >::type;
    using executor_type = async_mqtt::executor_type<next_layer_type>;

    template <
        typename T,
        typename... Args,
        std::enable_if_t<!std::is_same_v<std::decay_t<T>, this_type>>* = nullptr
    >
    explicit
    stream(T&& t, Args&&... args)
        :impl_{
            std::make_shared<impl_type>(
                std::forward<T>(t),
                std::forward<Args>(args)...
            )
        }
    {
    }

    ~stream() {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "destroy";
    }

    stream(this_type&&) = default;
    stream(this_type const&) = delete;
    this_type& operator=(this_type&&) = default;
    this_type& operator=(this_type const&) = delete;

    next_layer_type const& next_layer() const {
        return impl_->next_layer();
    }
    next_layer_type& next_layer() {
        return impl_->next_layer();
    }

    lowest_layer_type const& lowest_layer() const {
        return impl_->lowest_layer();
    }
    lowest_layer_type& lowest_layer() {
        return impl_->lowest_layer();
    }

    template <
        typename... Args
    >
    auto
    async_underlying_handshake(
        Args&&... args
    );

    template <
        typename MutableBufferSequence,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_read_some(
        MutableBufferSequence const& buffers,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename Packet,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_write_packet(
        Packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    as::any_io_executor get_executor() {
        return impl_->get_executor();
    };

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    auto
    async_close(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    void set_bulk_write(bool val) {
        impl_->set_bulk_write(val);
    }

    template <typename Executor1>
    struct rebind_executor {
        using other = stream<
            typename NextLayer::template rebind_executor<Executor1>::other
        >;
    };

    void set_read_buffer_size(std::size_t size) {
        impl_->set_read_buffer_size(size);
    }

private:
    // constructor
    template <typename Other>
    explicit
    stream(
        stream<Other>&& other
    )
        :impl_{force_move(other.impl_)}
    {
    }

private:
    std::shared_ptr<impl_type> impl_;
};

} // namespace async_mqtt

#include <async_mqtt/util/impl/stream_underlying_handshake.hpp>
#include <async_mqtt/util/impl/stream_read.hpp>
#include <async_mqtt/util/impl/stream_write_packet.hpp>
#include <async_mqtt/util/impl/stream_close.hpp>

#endif // ASYNC_MQTT_UTIL_STREAM_HPP
