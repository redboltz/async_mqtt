// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_STREAM_HPP)
#define ASYNC_MQTT_UTIL_STREAM_HPP

#include <iostream>

#include <utility>
#include <type_traits>

#include <boost/asio/async_result.hpp>

#include <async_mqtt/util/stream_traits.hpp>
#include <async_mqtt/util/make_shared_helper.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/ioc_queue.hpp>
#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/constant.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {
namespace as = boost::asio;
namespace sys = boost::system;

template <typename NextLayer>
class stream : public std::enable_shared_from_this<stream<NextLayer>> {
public:
    using this_type = stream<NextLayer>;
    using this_type_sp = std::shared_ptr<this_type>;
    using next_layer_type = typename std::remove_reference<NextLayer>::type;
    using lowest_layer_type =
        typename std::remove_reference<
            decltype(get_lowest_layer(std::declval<next_layer_type&>()))
        >::type;
    using executor_type = async_mqtt::executor_type<next_layer_type>;

    template <typename T>
    friend class make_shared_helper;

    template <
        typename T,
        typename... Args,
        std::enable_if_t<!std::is_same_v<std::decay_t<T>, this_type>>* = nullptr
    >
    static std::shared_ptr<this_type> create(T&& t, Args&&... args) {
        return make_shared_helper<this_type>::make_shared(std::forward<T>(t), std::forward<Args>(args)...);
    }

    ~stream() {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "destroy";
    }

    stream(this_type&&) = delete;
    stream(this_type const&) = delete;
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

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(error_code, buffer)
    )
    read_packet(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    template <
        typename Packet,
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void(system_error)
    )
    write_packet(
        Packet packet,
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    as::any_io_executor get_executor() {
        return nl_.get_executor();
    };

    template <
        typename CompletionToken = as::default_completion_token_t<executor_type>
    >
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
        CompletionToken,
        void()
    )
    close(
        CompletionToken&& token = as::default_completion_token_t<executor_type>{}
    );

    void set_bulk_write(bool val) {
        bulk_write_ = val;
    }

private:

    // constructor
    template <
        typename T,
        typename... Args,
        std::enable_if_t<!std::is_same_v<std::decay_t<T>, this_type>>* = nullptr
    >
    explicit
    stream(T&& t, Args&&... args)
        :nl_{std::forward<T>(t), std::forward<Args>(args)...}
    {
        initialize(nl_);
    }

    template <typename Layer>
    static void initialize(Layer& layer) {
        if constexpr (has_next_layer<Layer>::value) {
            initialize(layer.next_layer());
        }
        if constexpr(has_initialize<Layer>::value) {
            layer_customize<Layer>::initialize(layer);
        }
    }

    // async operations

    template <typename Packet>     struct stream_write_packet_op;
    struct stream_read_packet_op;
    struct stream_close_op;

private:
    next_layer_type nl_;
    ioc_queue queue_;
    static_vector<char, 5> header_remaining_length_buf_ = static_vector<char, 5>(5);
    std::vector<as::const_buffer> storing_cbs_;
    std::vector<as::const_buffer> sending_cbs_;
    bool bulk_write_ = false;
};

} // namespace async_mqtt

#include <async_mqtt/util/impl/stream_read_packet.hpp>
#include <async_mqtt/util/impl/stream_write_packet.hpp>
#include <async_mqtt/util/impl/stream_close.hpp>

#endif // ASYNC_MQTT_UTIL_STREAM_HPP
