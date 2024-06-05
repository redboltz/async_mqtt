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
    using packet_variant_type = basic_packet_variant<PacketIdBytes>;

    struct error_packet {
        error_packet(
            packet_variant_type pv
        ): packet{ to_string(pv.const_buffer_sequence()) }
        {}

        error_packet(
            std::string_view packet
        ):packet{packet}
        {}

        error_packet(
            error_code ec
        ):ec{ec}
        {}

        error_code ec;
        std::string packet;
    };

    cpp20coro_basic_stub_socket(
        protocol_version version,
        as::any_io_executor exe
    )
        :version_{version},
         exe_{force_move(exe)}
    {}

    void set_send_error_code(error_code ec) {
        send_ec_ = ec;
    }

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
                error_packet{std::forward<T>(t)}
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
        error_packet epk;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            socket.ch_recv_.async_send(
                force_move(epk),
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
            error_packet epk
        ) {
            if (epk.ec) {
                self.complete(epk.ec, packet_variant_type{});
            }
            else {
                buffer buf{epk.packet};
                error_code ec;
                auto pv = buffer_to_packet_variant(buf, socket.version_, ec);
                self.complete(ec, force_move(pv));
            }
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
            if (socket.send_ec_) {
                self.complete(socket.send_ec_, 0);
            }
            else {
                self.complete(error_code{}, std::size_t(dis));
            }
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
                if (socket.epk_opt_) {
                    state = complete;
                }
                else {
                    auto& a_socket{socket};
                    a_socket.ch_recv_.async_receive(
                        force_move(self)
                    );
                }
            }
            if (state == complete) {
                BOOST_ASSERT(socket.epk_opt_);
                BOOST_ASSERT(socket.packet_it_opt_);
                auto packet_it = *socket.packet_it_opt_;
                auto end = socket.epk_opt_->packet.end();
                BOOST_ASSERT(
                    static_cast<std::size_t>(std::distance(packet_it, end))
                    >=
                    mb.size()
                );
                std::copy_n(
                    packet_it,
                    mb.size(),
                    static_cast<char*>(mb.data())
                );
                std::advance(packet_it, static_cast<std::ptrdiff_t>(mb.size()));
                if (packet_it == end) {
                    // all conttents have read
                    socket.packet_it_opt_.reset();
                    socket.epk_opt_.reset();
                }
                else {
                    socket.packet_it_opt_.emplace(packet_it);
                }
                self.complete(errc::make_error_code(errc::success), mb.size());
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_packet epk
        ) {
            if (epk.ec) {
                self.complete(epk.ec, 0);
            }
            else {
                socket.epk_opt_.emplace(force_move(epk));
                socket.packet_it_opt_.emplace(socket.epk_opt_->packet.begin());
                state = complete;
                as::dispatch(
                    force_move(self)
                );
            }
        }
    };

private:
    using channel_t = as::experimental::channel<void(error_packet)>;
    protocol_version version_;
    as::any_io_executor exe_;
    std::optional<error_packet> epk_opt_;
    std::optional<std::string::iterator> packet_it_opt_;
    bool open_ = true;
    channel_t ch_recv_{exe_, 1};
    channel_t ch_send_{exe_, 1};
    error_code send_ec_;
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
            async_close_impl{stream},
            token,
            stream
        );
    }

    struct async_close_impl {
        async_close_impl(
            cpp20coro_stub_socket& stream
        ):stream{stream}
        {}

        cpp20coro_stub_socket& stream;
        enum {wait1, wait2, complete} state = wait1;
        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case wait1:
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << "stub close wait1";
                state = wait2;
                as::post(
                    stream.get_executor(),
                    force_move(self)
                );
                break;
            case wait2:
                ASYNC_MQTT_LOG("mqtt_impl", info)
                    << "stub close wait2";
                state = complete;
                as::post(
                    stream.get_executor(),
                    force_move(self)
                );
                break;
            case complete: {
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
            } break;
            }
        }
    };
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
