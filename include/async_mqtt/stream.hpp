// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STREAM_HPP)
#define ASYNC_MQTT_STREAM_HPP

#include <utility>
#include <type_traits>

#include <boost/system/error_code.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/bind_executor.hpp>

#include <async_mqtt/core/stream_traits.hpp>
#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/ws_fixed_size_async_read.hpp>

namespace async_mqtt {

namespace as = boost::asio;
namespace sys = boost::system;

template <typename NextLayer, std::size_t PacketIdBytes>
class basic_stream {
public:
    using this_type = basic_stream<NextLayer, PacketIdBytes>;
    using next_layer_type = typename std::remove_reference<NextLayer>::type;
    using executor_type = async_mqtt::executor_type<next_layer_type>;

    auto const& next_layer() const {
        return nl_;
    }
    auto& next_layer() {
        return nl_;
    }

    auto const& lowest_layer() const {
        return nl_.lowest_layer();
    }
    auto& lowest_layer() {
        return nl_.lowest_layer();
    }

    template <typename... Args>
    explicit
    basic_stream(Args&&... args)
        :nl_{std::forward<Args>(args)...}
    {
        queue_.emplace();
        queue_->stop();
    }

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, sys::error_code, buffer>::value
        >* = nullptr
    >
    auto read_packet(
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(sys::error_code const&, buffer)
            >(
                read_packet_impl{
                    *this
                },
                token
            );

    }

    template <
        typename ConstBufferSequence,
        typename CompletionToken,
        typename std::enable_if_t<
            as::is_const_buffer_sequence<ConstBufferSequence>::value &&
            std::is_invocable<CompletionToken, sys::error_code, std::size_t>::value
        >* = nullptr
    >
    auto write_packet(
        ConstBufferSequence const& packet,
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(sys::error_code const&, std::size_t)
            >(
                write_packet_impl<ConstBufferSequence>{
                    *this,
                    packet
                },
                token
            );
    }

    as::strand<executor_type> const& get_strand() const {
        return strand_;
    }
    as::strand<executor_type>& get_strand() {
        return strand_;
    }

private:

    struct read_packet_impl {
        read_packet_impl(this_type& strm):strm{strm}
        {}

        this_type& strm;
        std::size_t received = 0;
        shared_ptr_array hrl = make_shared_ptr_array(5);
        std::uint32_t mul = 1;
        std::uint32_t rl = 0;
        shared_ptr_array spa;
        sys::error_code last_ec;

        enum { dispatch, header, remaining_length, bind, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            switch (state) {
            case dispatch: {
                state = header;
                auto& a_strm{strm};
                as::dispatch(
                    a_strm.strand_,
                    force_move(self)
                );
            } break;
            case header: {
                // read fixed_header
                auto& a_hrl{hrl};
                auto& a_strm{strm};
                async_read(
                    a_strm.nl_,
                    as::buffer(&a_hrl[received], 1),
                    force_move(self)
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
            sys::error_code const& ec,
            std::size_t bytes_transferred
        ) {
            if (ec) {
                auto exe = as::get_associated_executor(self);
                if (exe == as::system_executor()) {
                    self.complete(ec, buffer{});
                    return;
                }
                state = complete;
                last_ec = ec;
                as::dispatch(
                    exe,
                    force_move(self)
                );
                return;
            }

            switch (state) {
            case header:
                BOOST_ASSERT(bytes_transferred == 1);
                state = remaining_length;
                ++received;
                // read the first remaining_length
                {
                    auto& a_hrl{hrl};
                    auto& a_strm{strm};
                    async_read(
                        a_strm.nl_,
                        as::buffer(&a_hrl[received], 1),
                        force_move(self)
                    );
                }
                break;
            case remaining_length:
                BOOST_ASSERT(bytes_transferred == 1);
                ++received;
                if (hrl[received - 1] & 0b10000000) {
                    // remaining_length continues
                    if (received == 5) {
                        self.complete(
                            sys::errc::make_error_code(sys::errc::protocol_error),
                            buffer{}
                        );
                        return;
                    }
                    rl += (hrl[received - 1] & 0b01111111) * mul;
                    mul *= 128;
                    auto& a_hrl{hrl};
                    auto& a_strm{strm};
                    async_read(
                        a_strm.nl_,
                        as::buffer(&a_hrl[received], 1),
                        force_move(self)
                    );
                }
                else {
                    // remaining_length end
                    rl += (hrl[received - 1] & 0b01111111) * mul;
                    if (rl == 0) {
                        auto ptr = hrl.get();
                        self.complete(ec, buffer{ptr, ptr + received, force_move(hrl)});
                    }
                    else {
                        state = bind;

                        spa = make_shared_ptr_array(received + rl);
                        std::copy(hrl.get(), hrl.get() + received, spa.get());

                        auto& a_spa{spa};
                        auto& a_strm{strm};
                        async_read(
                            a_strm.nl_,
                            as::buffer(&a_spa[received], rl),
                            force_move(self)
                        );
                    }
                }
                break;
            case bind: {
                state = complete;
                auto exe = as::get_associated_executor(self);
                if (exe == as::system_executor()) {
                    auto ptr = spa.get();
                    self.complete(ec, buffer{ptr, ptr + received + rl, force_move(spa)});
                    return;
                }
                as::dispatch(exe, force_move(self));
            } break;
            case complete: {
                if (last_ec) {
                    self.complete(last_ec, buffer{});
                }
                else {
                    auto ptr = spa.get();
                    self.complete(ec, buffer{ptr, ptr + received + rl, force_move(spa)});
                }
            } break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }
    };

    template <typename ConstBufferSequence>
    struct write_packet_impl {
        this_type& strm;
        ConstBufferSequence packet;
        sys::error_code last_ec = sys::error_code{};
        enum { initiate, write, bind, complete } state = initiate;

        template <typename Self>
        void operator()(
            Self& self,
            sys::error_code const& ec = sys::error_code{},
            std::size_t bytes_transferred = 0
        ) {
            if (ec) {
                strm.queue_->poll_one();
                auto exe = as::get_associated_executor(self);
                if (exe == as::system_executor()) {
                    self.complete(ec, bytes_transferred);
                    return;
                }
                state = complete;
                last_ec = ec;
                as::dispatch(exe, force_move(self));
                return;
            }
            switch (state) {
            case initiate: {
                state = write;
                auto& a_strm{strm};
n                a_strm.queue_->post(
                    as::bind_executor(
                        a_strm.strand_,
                        force_move(self)
                    )
                );
                if (a_strm.queue_->stopped()) {
                    a_strm.queue_->restart();
                    as::dispatch(
                        a_strm.strand_,
                        [&a_strm] {
                            a_strm.queue_->poll_one();
                        }
                    );
                }
            } break;
            case write: {
                state = bind;
                auto& a_strm{strm};
                auto& a_packet{packet};
                async_write(
                    a_strm.nl_,
                    a_packet,
                    force_move(self)
                );
            } break;
            case bind: {
                state = complete;
                strm.queue_->poll_one();
                auto exe = as::get_associated_executor(self);
                if (exe == as::system_executor()) {
                    self.complete(ec, bytes_transferred);
                    return;
                }
                as::dispatch(exe, force_move(self));
            } break;
            case complete:
                if (last_ec) {
                    self.complete(last_ec, 0);
                }
                else {
                    self.complete(ec, bytes_transferred);
                }
                break;
            }
        }
    };

private:
    next_layer_type nl_;
    as::strand<executor_type> strand_{nl_.get_executor()};
    optional<as::io_context> queue_;
};

template <typename NextLayer>
using stream = basic_stream<NextLayer, 2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_STREAM_HPP
