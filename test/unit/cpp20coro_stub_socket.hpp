// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CPP20CORO_STUB_SOCKET_HPP)
#define ASYNC_MQTT_CPP20CORO_STUB_SOCKET_HPP

#include <deque>

#include <boost/asio.hpp>
#include <boost/asio/experimental/channel.hpp>

#include <async_mqtt/buffer_to_packet_variant.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
struct cpp20coro_basic_stub_socket {
    using this_type = cpp20coro_basic_stub_socket<PacketIdBytes>;
    using executor_type = as::any_io_executor;
    using packet_iterator_t = packet_iterator<std::vector, as::const_buffer>;
    using packet_range = std::pair<packet_iterator_t, packet_iterator_t>;

    cpp20coro_basic_stub_socket(
        protocol_version version,
        as::any_io_executor exe
    )
        :version_{version},
         raw_exe_{force_move(exe)}
    {}

    void init(as::any_io_executor exe) {
        guarded_exe_.emplace(force_move(exe));
    }

    template <typename CompletionToken>
    auto emulate_recv(
        basic_packet_variant<PacketIdBytes> pv,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void()
        > (
            emulate_recv_impl{
                *this,
                force_move(pv)
            },
            token
        );
    }

    struct emulate_recv_impl {
        this_type& socket;
        basic_packet_variant<PacketIdBytes> pv;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            BOOST_ASSERT_MSG(socket.guarded_exe_, "You need to call call init(as::any_io_executor).");
            socket.ch_recv_.async_send(
                force_move(pv),
                as::bind_executor(
                    *socket.guarded_exe_,
                    force_move(self)
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& /* ec */
        ) {
            self.complete();
        }
    };

    template <typename CompletionToken>
    auto emulate_close(CompletionToken&& token) {
        return emulate_recv(
            errc::make_error_code(errc::connection_reset),
            std::forward<CompletionToken>(token)
        );
    }

    template <typename CompletionToken>
    auto wait_response(CompletionToken&& token) {
        return as::async_compose<
            CompletionToken,
            void(basic_packet_variant<PacketIdBytes>)
        > (
            wait_response_impl{
                *this
            },
            token
        );
    }

    struct wait_response_impl {
        this_type& socket;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            BOOST_ASSERT_MSG(socket.guarded_exe_, "You need to call call init(as::any_io_executor).");
            socket.ch_send_.async_receive(
                as::bind_executor(
                    *socket.guarded_exe_,
                    force_move(self)
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            basic_packet_variant<PacketIdBytes> pv
        ) {
            self.complete(force_move(pv));
        }
    };

    as::any_io_executor get_executor() const {
        return raw_exe_;
    }

    bool is_open() const {
        return open_;
    }

    void close(error_code&) {
        open_ = false;
        ch_send_.async_send(
            basic_packet_variant<PacketIdBytes>{errc::make_error_code(errc::connection_reset)},
            as::detached
        );
    }

    template <typename ConstBufferSequence, typename CompletionToken>
    auto async_write_some(
        ConstBufferSequence const& buffers,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec, std::size_t)
        > (
            async_write_some_impl<ConstBufferSequence>{
                *this,
                buffers
            },
            token
        );
    }

    template <typename ConstBufferSequence>
    struct async_write_some_impl {
        this_type& socket;
        ConstBufferSequence buffers;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            auto it = as::buffers_iterator<ConstBufferSequence>::begin(buffers);
            auto end = as::buffers_iterator<ConstBufferSequence>::end(buffers);
            auto dis = std::distance(it, end);
            auto packet_begin = it;

            while (it != end) {
                ++it; // it points to the first byte of remaining length
                if (auto remlen_opt = variable_bytes_to_val(it, end)) {
                    auto packet_end = std::next(it, *remlen_opt);
                    auto buf = allocate_buffer(packet_begin, packet_end);
                    auto pv = buffer_to_basic_packet_variant<PacketIdBytes>(buf, socket.version_);
                    socket.ch_send_.async_send(
                        pv,
                        as::detached
                    );
                    it = packet_end;
                    packet_begin = packet_end;
                }
            }
            self.complete(error_code{}, std::size_t(dis));
        }
    };

    template <typename MutableBufferSequence, typename CompletionToken>
    auto async_read_some(
        MutableBufferSequence const& mb,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec, std::size_t)
        > (
            async_read_some_impl<MutableBufferSequence>{
                *this,
                mb
            },
            token
        );
    }

    template <typename MutableBufferSequence>
    struct async_read_some_impl {
        this_type& socket;
        MutableBufferSequence mb;
        enum { read, complete } state = read;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            if (state == read) {
                if (socket.pv_r_) {
                    state = complete;
                }
                else {
                    BOOST_ASSERT_MSG(socket.guarded_exe_, "You need to call call init(as::any_io_executor).");
                    socket.ch_recv_.async_receive(
                        as::bind_executor(
                            *socket.guarded_exe_,
                            force_move(self)
                        )
                    );
                }
            }
            if (state == complete) {
                BOOST_ASSERT(socket.pv_r_);
                BOOST_ASSERT(
                    static_cast<std::size_t>(std::distance(socket.pv_r_->first, socket.pv_r_->second))
                    >=
                    mb.size()
                );
                as::mutable_buffer mb_copy = mb;
                std::copy(
                    socket.pv_r_->first,
                    std::next(socket.pv_r_->first, std::ptrdiff_t(mb.size())),
                    static_cast<char*>(mb_copy.data())
                );
                std::advance(socket.pv_r_->first, mb.size());
                if (socket.pv_r_->first == socket.pv_r_->second) {
                    socket.pv_r_ = nullopt;
                }
                self.complete(errc::make_error_code(errc::success), mb.size());
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            basic_packet_variant<PacketIdBytes> pv
        ) {
            if (pv) {
                socket.pv_ = pv;
                socket.cbs_ = socket.pv_.const_buffer_sequence();
                socket.pv_r_ = make_packet_range(socket.cbs_);
                state = complete;
                as::dispatch(
                    as::bind_executor(
                        *socket.guarded_exe_,
                        force_move(self)
                    )
                );
            }
            else {
                self.complete(pv.template get<system_error>().code(), 0);
            }
        }
    };

private:
    using channel_t = as::experimental::channel<void(basic_packet_variant<PacketIdBytes>)>;
    protocol_version version_;
    as::any_io_executor raw_exe_;
    optional<as::any_io_executor> guarded_exe_;
    basic_packet_variant<PacketIdBytes> pv_;
    std::vector<as::const_buffer> cbs_;
    optional<packet_range> pv_r_;
    bool open_ = true;
    channel_t ch_recv_{raw_exe_, 1};
    channel_t ch_send_{raw_exe_, 1};
};

using cpp20coro_stub_socket = cpp20coro_basic_stub_socket<2>;

template <>
struct layer_customize<cpp20coro_stub_socket> {
    template <
        typename MutableBufferSequence,
        typename CompletionToken
    >
    static auto
    async_read(
        cpp20coro_stub_socket& stream,
        MutableBufferSequence const& mbs,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec, std::size_t)
        > (
            async_read_impl{
                stream,
                mbs
            },
            token
        );
    }

    template <typename MutableBufferSequence>
    struct async_read_impl {
        cpp20coro_stub_socket& stream;
        MutableBufferSequence mbs;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            return stream.async_read_some(
                mbs,
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec,
            std::size_t size
        ) {
            self.complete(ec, size);
        }
    };


    template <
        typename CompletionToken
    >
    static auto
    async_close(
        cpp20coro_stub_socket& stream,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec)
        > (
            [&stream](auto& self) {
                error_code ec;
                if (stream.is_open()) {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "stub close";
                    stream.close(ec);
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "stub already closed";
                }
                self.complete(ec);
            },
            token
        );
    }
};

template <>
struct layer_customize<cpp20coro_basic_stub_socket<4>> {
    template <
        typename MutableBufferSequence,
        typename CompletionToken
    >
    static auto
    async_read(
        cpp20coro_basic_stub_socket<4>& stream,
        MutableBufferSequence const& mbs,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec, std::size_t)
        > (
            async_read_impl{
                stream,
                mbs
            },
            token
        );
    }

    template <typename MutableBufferSequence>
    struct async_read_impl {
        cpp20coro_basic_stub_socket<4>& stream;
        MutableBufferSequence mbs;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            return stream.async_read_some(
                mbs,
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec,
            std::size_t size
        ) {
            self.complete(ec, size);
        }
    };


    template <
        typename CompletionToken
    >
    static auto
    async_close(
        cpp20coro_basic_stub_socket<4>& stream,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec)
        > (
            [&stream](auto& self) {
                error_code ec;
                if (stream.is_open()) {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "stub close";
                    stream.close(ec);
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "stub already closed";
                }
                self.complete(ec);
            },
            token
        );
    }
};


inline bool is_close(packet_variant const& pv) {
    if (pv) {
        BOOST_TEST_MESSAGE("close expected but receive packet: " << pv);
        return false; // pv is packet
    }
    return pv.get<system_error>().code() == errc::make_error_code(errc::connection_reset);
}

inline bool is_close(basic_packet_variant<4> const& pv) {
    if (pv) {
        BOOST_TEST_MESSAGE("close expected but receive packet: " << pv);
        return false; // pv is packet
    }
    return pv.get<system_error>().code() == errc::make_error_code(errc::connection_reset);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_CPP20CORO_STUB_SOCKET_HPP
