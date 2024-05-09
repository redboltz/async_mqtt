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

namespace async_mqtt {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
struct basic_stub_socket {
    using executor_type = as::any_io_executor;
    using pv_queue_t = std::deque<basic_packet_variant<PacketIdBytes>>;
    using packet_iterator_t = packet_iterator<std::vector, as::const_buffer>;
    using packet_range = std::pair<packet_iterator_t, packet_iterator_t>;

    basic_stub_socket(
        protocol_version version,
        as::any_io_executor exe
    )
        :version_{version},
         exe_{force_move(exe)}
    {}

    basic_stub_socket(
        protocol_version version,
        as::io_context& ioc
    )
        :version_{version},
         exe_{ioc.get_executor()}
    {}

    void set_write_packet_checker(std::function<void(basic_packet_variant<PacketIdBytes> const& pv)> c) {
        open_ = true;
        write_packet_checker_ = force_move(c);
    }

    void set_recv_packets(std::deque<basic_packet_variant<PacketIdBytes>> recv_pvs) {
        recv_pvs_ = force_move(recv_pvs);
        recv_pvs_it_ = recv_pvs_.begin();
        pv_r_ = nullopt;

    }

    void set_close_checker(std::function<void()> c) {
        close_checker_ = force_move(c);
    }

#if 0
    auto const& lowest_layer() const {
        return exe_;
    }
    auto& lowest_layer() {
        return exe_;
    }
#endif
    auto get_executor() const {
        return exe_;
    }
    auto get_executor() {
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
    void async_write_some(
        ConstBufferSequence const& buffers,
        CompletionToken&& token
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
                auto pv = buffer_to_basic_packet_variant<PacketIdBytes>(buf, version_);
                if (write_packet_checker_) write_packet_checker_(pv);
                it = packet_end;
                packet_begin = packet_end;
            }
        }
        as::post(
            as::bind_executor(
                exe_,
                [token = std::forward<CompletionToken>(token), dis] () mutable {
                    auto exe = as::get_associated_executor(token);
                    as::dispatch(
                        as::bind_executor(
                            exe,
                            [token = force_move(token), dis] () mutable {
                                token(boost::system::error_code{}, std::size_t(dis));
                            }
                        )
                    );
                }
            )
        );
    }

    template <typename MutableBufferSequence, typename CompletionToken>
    void async_read_some(
        MutableBufferSequence const& mb,
        CompletionToken&& token
    ) {
        // empty
        if (recv_pvs_it_ == recv_pvs_.end()) {
            as::dispatch(
                as::bind_executor(
                    as::get_associated_executor(token),
                    [token = force_move(token)] () mutable {
                        token(errc::make_error_code(errc::no_message), 0);
                    }
                )
            );
            return;
        }
        if (auto* ec = recv_pvs_it_->template get_if<system_error>()) {
            ++recv_pvs_it_;
            as::dispatch(
                as::bind_executor(
                    as::get_associated_executor(token),
                    [token = force_move(token), code = ec->code()] () mutable {
                        token(code, 0);
                    }
                )
            );
            return;
        }

        if (!pv_r_) {
            cbs_ = recv_pvs_it_->const_buffer_sequence();
            pv_r_ = make_packet_range(cbs_);
        }

        while (pv_r_->first == pv_r_->second) {
            pv_r_ = nullopt;
            ++recv_pvs_it_;
            if (recv_pvs_it_ == recv_pvs_.end()) {
                as::dispatch(
                    as::bind_executor(
                        as::get_associated_executor(token),
                        [token = force_move(token)] () mutable {
                            token(errc::make_error_code(errc::no_message), 0);
                        }
                    )
                );
                return;
            }
            if (auto* ec = recv_pvs_it_->template get_if<system_error>()) {
                ++recv_pvs_it_;
                as::dispatch(
                    as::bind_executor(
                        as::get_associated_executor(token),
                        [token = force_move(token), code = ec->code()] () mutable {
                            token(code, 0);
                        }
                    )
                );
                return;
            }
            cbs_ = recv_pvs_it_->const_buffer_sequence();
            pv_r_ = make_packet_range(cbs_);
        }

        BOOST_ASSERT(static_cast<std::size_t>(std::distance(pv_r_->first, pv_r_->second)) >= mb.size());
        as::mutable_buffer mb_copy = mb;
        std::copy(pv_r_->first, std::next(pv_r_->first, std::ptrdiff_t(mb.size())), static_cast<char*>(mb_copy.data()));
        std::advance(pv_r_->first, mb.size());
        as::dispatch(
            as::bind_executor(
                as::get_associated_executor(token),
                [token = force_move(token), size = mb.size()] () mutable {
                    token(errc::make_error_code(errc::success), size);
                }
            )
        );
    }

private:

    protocol_version version_;
    as::any_io_executor exe_;
    pv_queue_t recv_pvs_;
    typename pv_queue_t::iterator recv_pvs_it_ = recv_pvs_.begin();
    std::vector<as::const_buffer> cbs_;
    optional<packet_range> pv_r_;
    std::function<void(basic_packet_variant<PacketIdBytes> const& pv)> write_packet_checker_;
    std::function<void()> close_checker_;
    bool open_ = true;
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
            token
        );
    }

    template <typename MutableBufferSequence>
    struct async_read_impl {
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
            token
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
            token
        );
    }

    template <typename MutableBufferSequence>
    struct async_read_impl {
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
            token
        );
    }
};


} // namespace async_mqtt

#endif // ASYNC_MQTT_STUB_SOCKET_HPP
