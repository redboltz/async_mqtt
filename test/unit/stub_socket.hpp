// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STUB_SOCKET_HPP)
#define ASYNC_MQTT_STUB_SOCKET_HPP

#include <deque>

#include <boost/asio.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/util/log.hpp>
#include <async_mqtt/util/stream_traits.hpp>
#include <async_mqtt/util/variable_bytes.hpp>

#include "test_allocate_buffer.hpp"

namespace async_mqtt {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
struct basic_stub_socket {
    using this_type = basic_stub_socket<PacketIdBytes>;
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

    using packet_queue_t = std::deque<error_packet>;


    basic_stub_socket(
        protocol_version version,
        as::any_io_executor exe
    )
        :version_{version},
         exe_{force_move(exe)}
    {}

    void set_write_packet_checker(std::function<void(basic_packet_variant<PacketIdBytes> const& pv)> c) {
        open_ = true;
        write_packet_checker_ = force_move(c);
    }

    void set_recv_packets(packet_queue_t recv_packets) {
        recv_packets_ = force_move(recv_packets);
        recv_packets_it_ = recv_packets_.begin();
        packet_it_opt_.reset();

    }

    void set_close_checker(std::function<void()> c) {
        close_checker_ = force_move(c);
    }

    void set_associated_cheker_for_read(std::size_t num) {
        associated_allocator_num_for_read_ = num;
    }

    void set_associated_cheker_for_write(std::size_t num) {
        associated_allocator_num_for_write_ = num;
    }

    auto get_executor() const {
        return exe_;
    }

    bool is_open() const {
        return open_;
    }
    void close(error_code&) {
        open_ = false;
        if (close_checker_) close_checker_();
    }

    template <typename ConstBufferSequence, typename CompletionToken>
    auto async_write_some(
        ConstBufferSequence const& buffers,
        CompletionToken&& token
    ) {
        if (associated_allocator_num_for_write_ != 0) {
            BOOST_ASIO_REBIND_ALLOC(
                typename as::associated_allocator<CompletionToken>::type,
                char
            ) alloc1(
                as::get_associated_allocator(token)
            );
            alloc1.deallocate(
                alloc1.allocate(associated_allocator_num_for_write_),
                associated_allocator_num_for_write_
            );
        }

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
                    if (socket.write_packet_checker_) socket.write_packet_checker_(*pv);
                    it = packet_end;
                    packet_begin = packet_end;
                }
            }
            self.complete(boost::system::error_code{}, std::size_t(dis));
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
            // empty
            if (socket.recv_packets_it_ == socket.recv_packets_.end()) {
                self.complete(errc::make_error_code(errc::no_message), 0);
                return;
            }
            auto ec = socket.recv_packets_it_->ec;
            if (ec) {
                ++socket.recv_packets_it_;
                self.complete(
                    ec,
                    0
                );
                return;
            }

            if (socket.associated_allocator_num_for_read_ != 0) {
                BOOST_ASIO_REBIND_ALLOC(
                    typename as::associated_allocator<Self>::type,
                    char
                ) alloc1(
                    as::get_associated_allocator(self)
                );
                alloc1.deallocate(
                    alloc1.allocate(socket.associated_allocator_num_for_read_),
                    socket.associated_allocator_num_for_read_
                );
            }

            auto begin = socket.recv_packets_it_->packet.begin();
            auto end = socket.recv_packets_it_->packet.end();
            if (!socket.packet_it_opt_) {
                socket.packet_it_opt_.emplace(begin);
            }
            auto packet_it = *socket.packet_it_opt_;
            auto rest_size = static_cast<std::size_t>(std::distance(packet_it, end));
            auto copy_size = std::min(rest_size, mb.size());
            std::copy_n(
                packet_it,
                copy_size,
                static_cast<char*>(mb.data())
            );
            std::advance(packet_it, static_cast<std::ptrdiff_t>(copy_size));
            if (packet_it == end) {
                // all conttents have read
                socket.packet_it_opt_.reset();
                ++socket.recv_packets_it_;
            }
            else {
                socket.packet_it_opt_.emplace(packet_it);
            }
            self.complete(errc::make_error_code(errc::success), copy_size);
        }
    };

private:

    template <typename MutableBufferSequence>
    friend struct async_read_some_impl;
    protocol_version version_;
    as::any_io_executor exe_;
    packet_queue_t recv_packets_;
    typename packet_queue_t::iterator recv_packets_it_ = recv_packets_.begin();
    std::optional<std::string::iterator> packet_it_opt_;
    std::function<void(basic_packet_variant<PacketIdBytes> const& pv)> write_packet_checker_;
    std::function<void()> close_checker_;
    bool open_ = true;
    std::size_t associated_allocator_num_for_read_ = 0;
    std::size_t associated_allocator_num_for_write_ = 0;
};

using stub_socket = basic_stub_socket<2>;

template <typename MutableBufferSequence, typename CompletionToken>
void async_read(
    basic_stub_socket<4>& socket,
    MutableBufferSequence const& mb,
    CompletionToken&& token
) {
    socket.async_read_some(mb, std::forward<CompletionToken>(token));
}

template <typename MutableBufferSequence, typename CompletionToken>
void async_read(
    stub_socket& socket,
    MutableBufferSequence const& mb,
    CompletionToken&& token
) {
    socket.async_read_some(mb, std::forward<CompletionToken>(token));
}

template <>
struct layer_customize<stub_socket> {
    template <
        typename MutableBufferSequence,
        typename CompletionToken
    >
    static auto
    async_read(
        stub_socket& stream,
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
            stub_socket& stream,
            MutableBufferSequence const& mbs
        ): stream{stream}, mbs{mbs}
        {}

        stub_socket& stream;
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
        stub_socket& stream,
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
struct layer_customize<basic_stub_socket<4>> {
    template <
        typename MutableBufferSequence,
        typename CompletionToken
    >
    static auto
    async_read(
        basic_stub_socket<4>& stream,
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
            basic_stub_socket<4>& stream,
            MutableBufferSequence const& mbs
        ): stream{stream}, mbs{mbs}
        {}

        basic_stub_socket<4>& stream;
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
        basic_stub_socket<4>& stream,
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

#endif // ASYNC_MQTT_STUB_SOCKET_HPP
