// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STREAM_HPP)
#define ASYNC_MQTT_STREAM_HPP

#include <iostream>

#include <utility>
#include <type_traits>

#include <boost/system/error_code.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/append.hpp>
#include <boost/asio/consign.hpp>

#if defined(ASYNC_MQTT_USE_WS)
#include <boost/beast/websocket/stream.hpp>
#endif // defined(ASYNC_MQTT_USE_WS)

#include <async_mqtt/stream_traits.hpp>
#include <async_mqtt/util/make_shared_helper.hpp>
#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/ioc_queue.hpp>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/constant.hpp>
#include <async_mqtt/is_strand.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/tls.hpp>
#include <async_mqtt/log.hpp>

namespace async_mqtt {

namespace as = boost::asio;
namespace sys = boost::system;

template <typename Stream>
struct is_ws : public std::false_type {};

#if defined(ASYNC_MQTT_USE_WS)
namespace bs = boost::beast;

template <typename NextLayer>
struct is_ws<bs::websocket::stream<NextLayer>> : public std::true_type {};

template <
    typename NextLayer,
    typename ConstBufferSequence,
    typename CompletionToken,
    typename std::enable_if_t<
        as::is_const_buffer_sequence<ConstBufferSequence>::value
    >* = nullptr
>
auto
async_write(
    bs::websocket::stream<NextLayer>& stream,
    ConstBufferSequence const& cbs,
    CompletionToken&& token
) {
    return stream.async_write(cbs, std::forward<CompletionToken>(token));
}

#endif // defined(ASYNC_MQTT_USE_WS)

template <typename Stream>
struct is_tls : public std::false_type {};

#if defined(ASYNC_MQTT_USE_TLS)
template <typename NextLayer>
struct is_tls<tls::stream<NextLayer>> : public std::true_type {};
#endif // defined(ASYNC_MQTT_USE_TLS)

template <typename NextLayer, template <typename> typename Strand = as::strand>
class stream : public std::enable_shared_from_this<stream<NextLayer, Strand>> {
public:
    using this_type = stream<NextLayer, Strand>;
    using this_type_sp = std::shared_ptr<this_type>;
    using next_layer_type = typename std::remove_reference<NextLayer>::type;
    using executor_type = async_mqtt::executor_type<next_layer_type>;
    using raw_strand_type = as::strand<executor_type>;
    using strand_type = Strand<as::any_io_executor>;

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

    auto const& next_layer() const {
        return nl_;
    }
    auto& next_layer() {
        return nl_;
    }

    auto const& lowest_layer() const {
        return get_lowest_layer(nl_);
    }
    auto& lowest_layer() {
        return get_lowest_layer(nl_);
    }

    auto get_executor() const {
        return nl_.get_executor();
    }
    auto get_executor() {
        return nl_.get_executor();
    }

    template <typename CompletionToken>
    auto
    read_packet(
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(error_code const&, buffer)
            >(
                read_packet_impl{
                    *this
                },
                token
            );
    }

    template <typename Packet, typename CompletionToken>
    auto
    write_packet(
        Packet packet,
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(error_code const&, std::size_t)
            >(
                write_packet_impl<Packet>{
                    *this,
                    std::make_shared<Packet>(force_move(packet))
                },
                token
            );
    }

    strand_type const& strand() const {
        return strand_;
    }

    strand_type& strand() {
        return strand_;
    }

    raw_strand_type const& raw_strand() const {
        return raw_strand_;
    };

    raw_strand_type& raw_strand() {
        return raw_strand_;
    };

    bool in_strand() const {
        return raw_strand().running_in_this_thread();
    }

    template<typename CompletionToken>
    auto
    close(CompletionToken&& token) {
        return
            as::async_compose<
                CompletionToken,
                void(error_code const&)
            >(
                close_impl{
                    *this
                },
                token
            );
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
#if defined(ASYNC_MQTT_USE_WS)
        if constexpr(is_ws<next_layer_type>::value) {
            nl_.binary(true);
            nl_.set_option(
                bs::websocket::stream_base::decorator(
                    [](bs::websocket::request_type& req) {
                        req.set("Sec-WebSocket-Protocol", "mqtt");
                    }
                )
            );
        }
#endif // defined(ASYNC_MQTT_USE_WS)
    }

    struct read_packet_impl {
        this_type& strm;
        std::size_t received = 0;
        std::uint32_t mul = 1;
        std::uint32_t rl = 0;
        shared_ptr_array spa = nullptr;
        this_type_sp life_keeper = strm.shared_from_this();
        enum { dispatch, header, remaining_length, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = header;
                auto& a_strm{strm};
                as::dispatch(
                    as::bind_executor(
                        a_strm.raw_strand_,
                        force_move(self)
                    )
                );
            } break;
            case header: {
                BOOST_ASSERT(strm.in_strand());
                // read fixed_header
                auto address = &strm.header_remaining_length_buf_[received];
                auto& a_strm{strm};
                async_read(
                    a_strm.nl_,
                    as::buffer(address, 1),
                    as::bind_executor(
                        a_strm.raw_strand_,
                        force_move(self)
                    )
                );
            } break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec,
            std::size_t bytes_transferred
        ) {
            boost::ignore_unused(bytes_transferred);

            BOOST_ASSERT(strm.in_strand());
            if (ec) {
                self.complete(ec, buffer{});
                return;
            }

            switch (state) {
            case header:
                BOOST_ASSERT(bytes_transferred == 1);
                state = remaining_length;
                ++received;
                // read the first remaining_length
                {
                    auto address = &strm.header_remaining_length_buf_[received];
                    auto& a_strm{strm};
                    async_read(
                        a_strm.nl_,
                        as::buffer(address, 1),
                        as::bind_executor(
                            a_strm.raw_strand_,
                            force_move(self)
                        )
                    );
                }
                break;
            case remaining_length:
                BOOST_ASSERT(bytes_transferred == 1);
                ++received;
                if (strm.header_remaining_length_buf_[received - 1] & 0b10000000) {
                    // remaining_length continues
                    if (received == 5) {
                        ASYNC_MQTT_LOG("mqtt_impl", warning)
                            << ASYNC_MQTT_ADD_VALUE(address, this)
                            << "out of size remaining length";
                        self.complete(
                            sys::errc::make_error_code(sys::errc::protocol_error),
                            buffer{}
                        );
                        return;
                    }
                    rl += (strm.header_remaining_length_buf_[received - 1] & 0b01111111) * mul;
                    mul *= 128;
                    auto address = &strm.header_remaining_length_buf_[received];
                    auto& a_strm{strm};
                    async_read(
                        a_strm.nl_,
                        as::buffer(address, 1),
                        as::bind_executor(
                            a_strm.raw_strand_,
                            force_move(self)
                        )
                    );
                }
                else {
                    // remaining_length end
                    rl += (strm.header_remaining_length_buf_[received - 1] & 0b01111111) * mul;

                    spa = make_shared_ptr_array(received + rl);
                    std::copy(
                        strm.header_remaining_length_buf_.data(),
                        strm.header_remaining_length_buf_.data() + received, spa.get()
                    );

                    if (rl == 0) {
                        auto ptr = spa.get();
                        self.complete(ec, buffer{ptr, ptr + received + rl, force_move(spa)});
                        return;
                    }
                    else {
                        state = complete;
                        auto address = &spa[std::ptrdiff_t(received)];
                        auto& a_strm{strm};
                        async_read(
                            a_strm.nl_,
                            as::buffer(address, rl),
                            as::bind_executor(
                                a_strm.raw_strand_,
                                force_move(self)
                            )
                        );
                    }
                }
                break;
            case complete: {
                auto ptr = spa.get();
                self.complete(ec, buffer{ptr, ptr + received + rl, force_move(spa)});
            } break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }
    };

    template <typename Packet>
    struct write_packet_impl {
        this_type& strm;
        std::shared_ptr<Packet> packet;
        this_type_sp life_keeper = strm.shared_from_this();
        enum { dispatch, post, write, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = post;
                auto& a_strm{strm};
                as::dispatch(
                    as::bind_executor(
                        a_strm.raw_strand_,
                        force_move(self)
                    )
                );
            } break;
            case post: {
                BOOST_ASSERT(strm.in_strand());
                state = write;
                auto& a_strm{strm};
                a_strm.queue_.post(
                    as::bind_executor(
                        a_strm.raw_strand_,
                        force_move(self)
                    )
                );
            } break;
            case write: {
                BOOST_ASSERT(strm.in_strand());
                strm.queue_.start_work();
                if (strm.lowest_layer().is_open()) {
                    state = complete;
                    auto& a_strm{strm};
                    auto cbs = packet->const_buffer_sequence();
                    async_write(
                        a_strm.nl_,
                        cbs,
                        as::bind_executor(
                            a_strm.raw_strand_,
                            force_move(self)
                        )
                    );
                }
                else {
                    state = complete;
                    auto& a_strm{strm};
                    as::dispatch(
                        as::bind_executor(
                            a_strm.raw_strand_,
                            as::append(
                                force_move(self),
                                errc::make_error_code(errc::connection_reset),
                                0
                            )
                        )
                    );
                }
            } break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec,
            std::size_t bytes_transferred
        ) {
            BOOST_ASSERT(strm.in_strand());
            if (ec) {
                strm.queue_.stop_work();
                auto& a_strm{strm};
                as::post(
                    as::bind_executor(
                        a_strm.raw_strand_,
                        [&a_strm,wp = a_strm.weak_from_this()] {
                            if (auto sp = wp.lock()) {
                                a_strm.queue_.poll_one();
                            }
                        }
                    )
                );
                self.complete(ec, bytes_transferred);
                return;
            }
            switch (state) {
            case complete: {
                strm.queue_.stop_work();
                auto& a_strm{strm};
                as::post(
                    as::bind_executor(
                        a_strm.raw_strand_,
                        [&a_strm, wp = a_strm.weak_from_this()] {
                            if (auto sp = wp.lock()) {
                                a_strm.queue_.poll_one();
                            }
                        }
                    )
                );
                self.complete(ec, bytes_transferred);
            } break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }
    };

    struct close_impl {
        this_type& strm;
        enum {
            dispatch,
            close,
            drop1,
            complete
        } state = dispatch;
        this_type_sp life_keeper = strm.shared_from_this();

        template <typename Self>
        void operator()(
            Self& self
        ) {
            BOOST_ASSERT(state == dispatch);
            state = close;
            auto& a_strm{strm};
            as::dispatch(
                as::bind_executor(
                    a_strm.raw_strand_,
                    as::append(
                        force_move(self),
                        error_code{},
                        std::ref(a_strm.nl_)
                    )
                )
            );
        }

#if defined(ASYNC_MQTT_USE_WS)
        template <typename Self, typename Stream>
        void operator()(
            Self& self,
            error_code const& ec,
            std::size_t /*size*/,
            std::reference_wrapper<Stream> stream
        ) {
            BOOST_ASSERT(strm.in_strand());
            if constexpr(is_ws<Stream>::value) {
                BOOST_ASSERT(state == complete);
                if (ec) {
                    if (ec == bs::websocket::error::closed) {
                        ASYNC_MQTT_LOG("mqtt_impl", info)
                            << ASYNC_MQTT_ADD_VALUE(address, this)
                            << "ws async_read (for close)  success";
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", info)
                            << ASYNC_MQTT_ADD_VALUE(address, this)
                            << "ws async_read (for close):" << ec.message();
                    }
                    state = close;
                    auto& a_strm{strm};
                    as::dispatch(
                        as::bind_executor(
                            a_strm.raw_strand_,
                            as::append(
                                force_move(self),
                                error_code{},
                                std::ref(stream.get().next_layer())
                            )
                        )
                    );
                }
                else {
                    auto& a_strm{strm};
                    auto buffer = std::make_shared<bs::flat_buffer>();
                    stream.get().async_read(
                        *buffer,
                        as::bind_executor(
                            a_strm.raw_strand_,
                            as::append(
                                as::consign(
                                    force_move(self),
                                    buffer
                                ),
                                force_move(stream)
                            )
                        )
                    );
                }
            }
        }
#endif // defined(ASYNC_MQTT_USE_WS)

        template <typename Self, typename Stream>
        void operator()(
            Self& self,
            error_code const& ec,
            std::reference_wrapper<Stream> stream
        ) {
            BOOST_ASSERT(strm.in_strand());
            switch (state) {
            case close: {
#if defined(ASYNC_MQTT_USE_WS)
                if constexpr(is_ws<Stream>::value) {
                    if (stream.get().is_open()) {
                        state = drop1;
                        auto& a_strm{strm};
                        stream.get().async_close(
                            bs::websocket::close_code::none,
                            as::bind_executor(
                                a_strm.raw_strand_,
                                as::append(
                                    force_move(self),
                                    force_move(stream)
                                )
                            )
                        );
                    }
                    else {
                        state = close;
                        auto& a_strm{strm};
                        as::dispatch(
                            as::bind_executor(
                                a_strm.raw_strand_,
                                as::append(
                                    force_move(self),
                                    error_code{},
                                    std::ref(stream.get().next_layer())
                                )
                            )
                        );
                    }
                }
                else
#endif // defined(ASYNC_MQTT_USE_WS)
                if constexpr(is_tls<Stream>::value) {
                    auto& a_strm{strm};
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << ASYNC_MQTT_ADD_VALUE(address, this)
                        << "TLS async_shutdown start with timeout";
                    auto tim = std::make_shared<as::steady_timer>(a_strm.raw_strand_, shutdown_timeout);
                    tim->async_wait(
                        as::bind_executor(
                            a_strm.raw_strand_,
                            [this, &next_layer = stream.get().next_layer()] (error_code const& ec) {
                                if (!ec) {
                                    ASYNC_MQTT_LOG("mqtt_impl", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, this)
                                        << "TLS async_shutdown timeout";
                                    error_code ec;
                                    next_layer.close(ec);
                                }
                            }
                        )
                    );
                    stream.get().async_shutdown(
                        as::bind_executor(
                            a_strm.raw_strand_,
                            as::append(
                                as::consign(
                                    force_move(self),
                                    tim
                                ),
                                std::ref(stream.get().next_layer())
                            )
                        )
                    );
                }
                else {
                    error_code ec;
                    if (stream.get().is_open()) {
                        ASYNC_MQTT_LOG("mqtt_impl", info)
                            << ASYNC_MQTT_ADD_VALUE(address, this)
                            << "TCP close";
                        stream.get().close(ec);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_impl", info)
                            << ASYNC_MQTT_ADD_VALUE(address, this)
                            << "TCP already closed";
                    }
                    self.complete(ec);
                }
            } break;
            case drop1: {
#if defined(ASYNC_MQTT_USE_WS)
                if constexpr(is_ws<Stream>::value) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_impl", info)
                            << ASYNC_MQTT_ADD_VALUE(address, this)
                            << "ws async_close:" << ec.message();
                        state = close;
                        auto& a_strm{strm};
                        as::dispatch(
                            as::bind_executor(
                                a_strm.raw_strand_,
                                as::append(
                                    force_move(self),
                                    error_code{},
                                    std::ref(stream.get().next_layer())
                                )
                            )
                        );
                        return;
                    }
                    state = complete;
                    auto& a_strm{strm};
                    auto buffer = std::make_shared<bs::flat_buffer>();
                    stream.get().async_read(
                        *buffer,
                        as::bind_executor(
                            a_strm.raw_strand_,
                            as::append(
                                as::consign(
                                    force_move(self),
                                    buffer
                                ),
                                force_move(stream)
                            )
                        )
                    );
                }
#else  // defined(ASYNC_MQTT_USE_WS)
                (void)ec;
#endif // defined(ASYNC_MQTT_USE_WS)
            } break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }
    };

private:
    next_layer_type nl_;
    raw_strand_type raw_strand_{nl_.get_executor()};
    strand_type strand_{as::any_io_executor{raw_strand_}};
    ioc_queue queue_;
    static_vector<char, 5> header_remaining_length_buf_ = static_vector<char, 5>(5);
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_STREAM_HPP
