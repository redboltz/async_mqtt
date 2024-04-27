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
    using executor_type = as::any_io_executor;
    using packet_iterator_t = packet_iterator<std::vector, as::const_buffer>;
    using packet_range = std::pair<packet_iterator_t, packet_iterator_t>;

    cpp20coro_basic_stub_socket(
        protocol_version version,
        as::any_io_executor exe
    )
        :version_{version},
         exe_{force_move(exe)}
    {}

    cpp20coro_basic_stub_socket(
        protocol_version version,
        as::io_context& ioc
    )
        :version_{version},
         exe_{ioc.get_executor()}
    {}

    template <typename CompletionToken>
    auto emulate_recv(
        basic_packet_variant<PacketIdBytes> pv,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void()
        >(
            [this](
                auto completion_handler,
                basic_packet_variant<PacketIdBytes> pv
            ) {
                ch_recv_.async_send(
                    pv,
                    [completion_handler = force_move(completion_handler)]
                    (auto) mutable {
                        force_move(completion_handler)();
                    }
                );
            },
            token,
            force_move(pv)
        );
    }

    template <typename CompletionToken>
    auto emulate_close(CompletionToken&& token) {
        return emulate_recv(
            errc::make_error_code(errc::connection_reset),
            std::forward<CompletionToken>(token)
        );
    }

    template <typename CompletionToken>
    auto wait_response(CompletionToken&& token) {
        return as::async_initiate<
            CompletionToken,
            void(basic_packet_variant<PacketIdBytes>)
        >(
            [this](
                auto completion_handler
            ) {
                ch_send_.async_receive(
                    [completion_handler = force_move(completion_handler)]
                    (basic_packet_variant<PacketIdBytes> pv) mutable {
                        force_move(completion_handler)(force_move(pv));
                    }
                );
            },
            token
        );
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
        ch_send_.async_send(
            basic_packet_variant<PacketIdBytes>{errc::make_error_code(errc::connection_reset)},
            [](auto) {
            }
        );
    }

    template <typename ConstBufferSequence, typename CompletionToken>
    auto async_write_some(
        ConstBufferSequence const& buffers,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(boost::system::error_code, std::size_t)
        >(
            [this](
                auto completion_handler,
                ConstBufferSequence const& buffers
            ) {
                auto exe = as::get_associated_executor(completion_handler);
                auto it = as::buffers_iterator<ConstBufferSequence>::begin(buffers);
                auto end = as::buffers_iterator<ConstBufferSequence>::end(buffers);
                auto dis = std::distance(it, end);
                auto packet_begin = it;
                auto guard = shared_scope_guard(
                    [completion_handler = force_move(completion_handler), dis] () mutable {
                        force_move(completion_handler)(
                            boost::system::error_code{},
                            std::size_t(dis)
                        );
                    }
                );

                while (it != end) {
                    ++it; // it points to the first byte of remaining length
                    if (auto remlen_opt = variable_bytes_to_val(it, end)) {
                        auto packet_end = std::next(it, *remlen_opt);
                        auto buf = allocate_buffer(packet_begin, packet_end);
                        auto pv = buffer_to_basic_packet_variant<PacketIdBytes>(buf, version_);
                        as::post(
                            as::bind_executor(
                                exe_,
                                [
                                    this,
                                    exe,
                                    pv,
                                    guard
                                ] () mutable {
                                    as::dispatch(
                                        as::bind_executor(
                                            exe,
                                            [
                                                this,
                                                pv,
                                                guard = force_move(guard)
                                            ] () mutable {
                                                ch_send_.async_send(
                                                    pv,
                                                    as::detached
                                                );
                                            }
                                        )
                                    );
                                }
                            )
                        );
                        it = packet_end;
                        packet_begin = packet_end;
                    }
                }
            },
            token,
            buffers
        );
    }

    template <typename MutableBufferSequence, typename CompletionToken>
    void async_read_some(
        MutableBufferSequence const& mb,
        CompletionToken&& token
    ) {
        auto partial_read =
            [
                this,
                mb = mb
            ]
            (CompletionToken&& token){
                BOOST_ASSERT(pv_r_);
                BOOST_ASSERT(static_cast<std::size_t>(std::distance(pv_r_->first, pv_r_->second)) >= mb.size());
                as::mutable_buffer mb_copy = mb;
                std::copy(
                    pv_r_->first,
                    std::next(pv_r_->first, std::ptrdiff_t(mb.size())),
                    static_cast<char*>(mb_copy.data())
                );
                std::advance(pv_r_->first, mb.size());
                as::post(
                    as::bind_executor(
                        as::get_associated_executor(token),
                        [token = force_move(token), size = mb.size()] () mutable {
                            token(errc::make_error_code(errc::success), size);
                        }
                    )
                );
                if (pv_r_->first == pv_r_->second) {
                    pv_r_ = nullopt;
                }
            };

        if (pv_r_) {
            partial_read(std::forward<CompletionToken>(token));
            return;
        }

        ch_recv_.async_receive(
            [
                this,
                token = std::forward<CompletionToken>(token),
                partial_read
            ]
            (basic_packet_variant<PacketIdBytes> pv) mutable {
                if (!pv) {
                    auto exe = as::get_associated_executor(token);
                    as::post(
                        as::bind_executor(
                            exe,
                            [
                                token = force_move(token),
                                code = pv.template get<system_error>().code()
                            ] () mutable {
                                token(code, 0);
                            }
                        )
                    );
                    return;
                }
                pv_ = pv;
                cbs_ = pv_.const_buffer_sequence();
                pv_r_ = make_packet_range(cbs_);
                partial_read(force_move(token));
            }
        );
    }

private:
    using channel_t = as::experimental::channel<void(basic_packet_variant<PacketIdBytes>)>;
    protocol_version version_;
    as::any_io_executor exe_;
    basic_packet_variant<PacketIdBytes> pv_;
    std::vector<as::const_buffer> cbs_;
    optional<packet_range> pv_r_;
    bool open_ = true;
    channel_t ch_recv_{exe_, 1};
    channel_t ch_send_{exe_, 1};
};

using cpp20coro_stub_socket = cpp20coro_basic_stub_socket<2>;

template <typename MutableBufferSequence, typename CompletionToken>
void async_read(
    cpp20coro_stub_socket& socket,
    MutableBufferSequence const& mb,
    CompletionToken&& token
) {
    socket.async_read_some(mb, std::forward<CompletionToken>(token));
}

template <typename MutableBufferSequence, typename CompletionToken>
void async_read(
    cpp20coro_basic_stub_socket<4>& socket,
    MutableBufferSequence const& mb,
    CompletionToken&& token
) {
    socket.async_read_some(mb, std::forward<CompletionToken>(token));
}

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
