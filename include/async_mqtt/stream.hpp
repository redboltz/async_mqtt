// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STREAM_HPP)
#define ASYNC_MQTT_STREAM_HPP

#include <utility>
#include <type_traits>

#include <async_mqtt/core/stream_traits.hpp>
#include <async_mqtt/optional.hpp>
#include <async_mqtt/buffer.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <typename NextLayer>
class stream {
public:
    using this_type = stream<NextLayer>;
    using next_layer_type = typename std::remove_reference<NextLayer>::type;
    using executor_type = async_mqtt::executor_type<next_layer_type>;

    stream(NextLayer& nl)
        :nl_{nl}
    {
        queue_.emplace();
        queue_->stop();
    }

    template <
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, boost::system::error_code, buffer>::value
        >* = nullptr
    >
    auto async_read_packet(
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(boost::system::error_code const&, buffer)
            >(
                async_read_packet_impl{
                    *this
                },
                token
            );

    }

    template <
        typename Packet, // add concept later TBD
        typename CompletionToken,
        typename std::enable_if_t<
            std::is_invocable<CompletionToken, boost::system::error_code, std::size_t>::value
        >* = nullptr
    >
    auto async_write_packet(
        Packet&& packet,
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
                void(boost::system::error_code const&, std::size_t)
            >(
                async_write_packet_impl<Packet>{
                    *this,
                    std::forward<Packet>(packet)
                },
                token
            );
    }

private:

    struct async_read_packet_impl {
        async_read_packet_impl(this_type& strm):strm{strm}
        {}

        this_type& strm;
        std::size_t received = 0;
        shared_ptr_array hrl = make_shared_ptr_array(5);
        std::uint32_t mul = 1;
        std::uint32_t rl = 0;
        shared_ptr_array spa;
        boost::system::error_code last_ec;

        enum { header, remaining_length, bind, complete } state = header;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            state = header;
            // read fixed_header
            auto& a_hrl{hrl};
            async_read(
                strm.nl_,
                as::buffer(&a_hrl[received], 1),
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            boost::system::error_code const& ec,
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
                    async_read(
                        strm.nl_,
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
                            boost::system::errc::make_error_code(boost::system::errc::protocol_error),
                            buffer{}
                        );
                        return;
                    }
                    rl += (hrl[received - 1] & 0b01111111) * mul;
                    mul *= 128;
                    auto& a_hrl{hrl};
                    async_read(
                        strm.nl_,
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
                        async_read(
                            strm.nl_,
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
            }
        }
    };

    template <typename Packet>
    struct async_write_packet_impl {
        this_type& strm;
        Packet packet;
        boost::system::error_code last_ec = boost::system::error_code{};
        enum { initiate, write, bind, complete } state = initiate;

        template <typename Self>
        void operator()(
            Self& self,
            boost::system::error_code const& ec = boost::system::error_code{},
            std::size_t bytes_transferred = 0
        ) {
            if (ec) {
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
            case initiate:
                state = write;
                strm.queue_->post(
                    as::bind_executor(
                        strm.strand_,
                        force_move(self)
                    )
                );
                if (strm.queue_->stopped()) {
                    strm.queue_->restart();
                    as::dispatch(
                        as::bind_executor(
                            strm.strand_,
                            [this] {
                                strm.queue_->poll_one();
                            }
                        )
                    );
                }
                break;
            case write: {
                state = bind;
                auto cbs = packet.const_buffer_sequence();
                async_write(
                    strm.nl_,
                    force_move(cbs),
                    force_move(self)
                );
            } break;
            case bind: {
                state = complete;
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

    next_layer_type& nl_;
    as::strand<executor_type> strand_{nl_.get_executor()};
    optional<as::io_context> queue_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_STREAM_HPP
