// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_STREAM_WRITE_PACKET_HPP)
#define ASYNC_MQTT_IMPL_STREAM_WRITE_PACKET_HPP

#include <async_mqtt/util/stream.hpp>
#include <iostream>

namespace async_mqtt {

namespace detail {

template <typename NextLayer>
template <typename Packet>
struct stream_impl<NextLayer>::stream_write_packet_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;
    using next_layer_type = stream_type::next_layer_type;

    std::shared_ptr<stream_type> strm;
    std::shared_ptr<Packet> packet;
    std::size_t size = packet->size();
    enum { dispatch, post, write, bulk_write, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_strm{*strm};
        switch (state) {
        case dispatch: {
            std::cout << __FILE__ << ":" << __LINE__ << ":" << *packet << std::endl;
            state = post;
            as::dispatch(
                a_strm.get_executor(),
                force_move(self)
            );
        } break;
        case post: {
            std::cout << __FILE__ << ":" << __LINE__ << ":" << *packet << std::endl;
            auto& a_packet{*packet};
            if (!a_strm.bulk_write_ || a_strm.write_queue_.immediate_executable()) {
                state = write;
            }
            else {
                state = bulk_write;
                auto cbs = a_packet.const_buffer_sequence();
                std::copy(cbs.begin(), cbs.end(), std::back_inserter(a_strm.storing_cbs_));
            }
            a_strm.write_queue_.post(
                force_move(self)
            );
            a_strm.write_queue_.try_execute();
        } break;
        case write: {
            std::cout << __FILE__ << ":" << __LINE__ << ":" << *packet << std::endl;
            a_strm.write_queue_.start_work();
            if (a_strm.lowest_layer().is_open()) {
                state = complete;
                auto& a_packet{*packet};
                if constexpr (
                    has_async_write<next_layer_type>::value) {
                    layer_customize<next_layer_type>::async_write(
                        a_strm.nl_,
                        a_packet.const_buffer_sequence(),
                        force_move(self)
                    );
                }
                else {
                    async_write(
                        a_strm.nl_,
                        a_packet.const_buffer_sequence(),
                        force_move(self)
                    );
                }
            }
            else {
                state = complete;
                as::dispatch(
                    a_strm.get_executor(),
                    as::append(
                        force_move(self),
                        errc::make_error_code(errc::connection_reset),
                        0
                    )
                );
            }
        } break;
        case bulk_write: {
            std::cout << __FILE__ << ":" << __LINE__ << ":" << *packet << std::endl;
            a_strm.write_queue_.start_work();
            if (a_strm.lowest_layer().is_open()) {
                state = complete;
                if (a_strm.storing_cbs_.empty()) {
                    auto& a_size{size};
                    as::dispatch(
                        a_strm.get_executor(),
                        as::append(
                            force_move(self),
                            errc::make_error_code(errc::success),
                            a_size
                        )
                    );
                }
                else {
                    a_strm.sending_cbs_ = force_move(a_strm.storing_cbs_);
                    if constexpr (
                        has_async_write<next_layer_type>::value) {
                        layer_customize<next_layer_type>::async_write(
                            a_strm.nl_,
                            a_strm.sending_cbs_,
                            force_move(self)
                        );
                    }
                    else {
                        async_write(
                            a_strm.nl_,
                            a_strm.sending_cbs_,
                            force_move(self)
                        );
                    }
                }
            }
            else {
                state = complete;
                as::dispatch(
                    a_strm.get_executor(),
                    as::append(
                        force_move(self),
                        errc::make_error_code(errc::connection_reset),
                        0
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
        error_code ec,
        std::size_t bytes_transferred
    ) {
        auto& a_strm{*strm};
        if (ec) {
            std::cout << __FILE__ << ":" << __LINE__ << ":" << *packet << std::endl;
            a_strm.write_queue_.stop_work();
            as::post(
                a_strm.get_executor(),
                [strm = force_move(strm)] {
                    strm->write_queue_.poll_one();
                }
            );
            self.complete(ec, bytes_transferred);
            return;
        }
        switch (state) {
        case complete: {
            std::cout << __FILE__ << ":" << __LINE__ << ":" << *packet << std::endl;
            a_strm.write_queue_.stop_work();
            a_strm.sending_cbs_.clear();
            as::post(
                a_strm.get_executor(),
                [strm = force_move(strm)] {
                    strm->write_queue_.poll_one();
                }
            );
            self.complete(ec, size);
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }
};

} // namespace detail

template <typename NextLayer>
template <typename Packet, typename CompletionToken>
auto
stream<NextLayer>::async_write_packet(
    Packet packet,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::size_t)
        >(
            typename impl_type::template stream_write_packet_op<Packet>{
                impl_,
                std::make_shared<Packet>(force_move(packet))
            },
            token,
            get_executor()
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_STREAM_WRITE_PACKET_HPP
