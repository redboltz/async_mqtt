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
#include <async_mqtt/packet/packet_variant.hpp>

#include "test_allocate_buffer.hpp"

namespace async_mqtt {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
struct cpp20coro_basic_stub_socket {
    using this_type = cpp20coro_basic_stub_socket<PacketIdBytes>;
    using executor_type = as::any_io_executor;
    using packet_iterator_t = packet_iterator<std::vector, as::const_buffer>;
    using packet_range = std::pair<packet_iterator_t, packet_iterator_t>;
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;

    struct error_pv {
        error_pv() = default;
        error_pv(
            packet_variant_type pv
        ):pv{force_move(pv)}
        {}

        error_pv(
            error_code ec
        ):ec{ec}
        {}

        error_code ec;
        packet_variant_type pv;
    };

    cpp20coro_basic_stub_socket(
        protocol_version version,
        as::any_io_executor exe
    )
        :version_{version},
         exe_{force_move(exe)}
    {}

    template <typename T, typename CompletionToken>
    auto emulate_recv(
        T&& t,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void()
        > (
            emulate_recv_impl{
                *this,
                error_pv{std::forward<T>(t)}
            },
            token,
            get_executor()
        );
    }

    template <typename CompletionToken>
    auto emulate_close(CompletionToken&& token) {
        return emulate_recv(
            errc::make_error_code(
                errc::connection_reset
            ),
            std::forward<CompletionToken>(token)
        );
    }

    template <typename CompletionToken>
    auto wait_response(CompletionToken&& token) {
        return as::async_compose<
            CompletionToken,
            void(error_code, packet_variant_type)
        > (
            wait_response_impl{
                *this
            },
            token,
            get_executor()
        );
    }

    struct emulate_recv_impl {
        this_type& socket;
        error_pv epv;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            socket.ch_recv_.async_send(
                force_move(epv),
                force_move(self)
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

    struct wait_response_impl {
        this_type& socket;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            socket.ch_send_.async_receive(
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_pv epv
        ) {
            self.complete(epv.ec, force_move(epv.pv));
        }
    };

    as::any_io_executor get_executor() const {
        return exe_;
    }

    bool is_open() const {
        return open_;
    }

    void close(error_code&) {
        open_ = false;
        ch_send_.async_send(
            errc::make_error_code(errc::connection_reset),
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
            token,
            get_executor()
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
                    error_code ec;
                    auto pv = buffer_to_basic_packet_variant<PacketIdBytes>(buf, socket.version_, ec);
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
            token,
            get_executor()
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
                    socket.ch_recv_.async_receive(
                        force_move(self)
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
                    socket.pv_r_ = std::nullopt;
                }
                self.complete(errc::make_error_code(errc::success), mb.size());
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_pv epv
        ) {
            if (epv.ec) {
                self.complete(epv.ec, 0);
            }
            else {
                socket.epv_ = epv;
                socket.cbs_ = socket.epv_.pv.const_buffer_sequence();
                socket.pv_r_ = make_packet_range(socket.cbs_);
                state = complete;
                as::dispatch(
                    force_move(self)
                );
            }
        }
    };

private:
    using channel_t = as::experimental::channel<void(error_pv)>;
    protocol_version version_;
    as::any_io_executor exe_;
    error_pv epv_;
    std::vector<as::const_buffer> cbs_;
    std::optional<packet_range> pv_r_;
    bool open_ = true;
    channel_t ch_recv_{exe_, 1};
    channel_t ch_send_{exe_, 1};
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
            token,
            stream
        );
    }

    template <typename MutableBufferSequence>
    struct async_read_impl {
        async_read_impl(
            cpp20coro_stub_socket& stream,
            MutableBufferSequence const& mbs
        ): stream{stream}, mbs{mbs}
        {}

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
            token,
            stream
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
            token,
            stream
        );
    }

    template <typename MutableBufferSequence>
    struct async_read_impl {
        async_read_impl(
            cpp20coro_basic_stub_socket<4>& stream,
            MutableBufferSequence const& mbs
        ): stream{stream}, mbs{mbs}
        {}

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
            token,
            stream
        );
    }
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_CPP20CORO_STUB_SOCKET_HPP
