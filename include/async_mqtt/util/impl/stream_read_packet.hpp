// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_IMPL_STREAM_READ_PACKET_HPP)
#define ASYNC_MQTT_UTIL_IMPL_STREAM_READ_PACKET_HPP

#include <async_mqtt/error.hpp>
#include <async_mqtt/util/stream.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>

namespace async_mqtt {

namespace detail {

template <typename NextLayer>
struct stream_impl<NextLayer>::stream_read_packet_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;
    using next_layer_type = stream_type::next_layer_type;

    std::shared_ptr<stream_type> strm;
    std::size_t received = 0;
    std::uint32_t mul = 1;
    std::uint32_t rl = 0;
    std::size_t rl_expected = 2;
    std::shared_ptr<char[]> spca = nullptr;
    enum { dispatch, post, work, remaining_length, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_strm{*strm};
        switch (state) {
        case dispatch: {
            state = post;
            as::dispatch(
                a_strm.get_executor(),
                force_move(self)
            );
        } break;
        case post: {
            state = work;
            a_strm.read_queue_.post(
                force_move(self)
            );
            a_strm.read_queue_.try_execute();
        } break;
        case work: {
            a_strm.read_queue_.start_work();
            if (a_strm.bulk_read_buffer_size_ == 0) {
                // start non bulk read
                state = remaining_length;
                // read fixed_header + first remaining_length
                a_strm.header_remaining_length_buf_.resize(5);
                auto address = &a_strm.header_remaining_length_buf_[received];
                if constexpr (
                    has_async_read<next_layer_type>::value) {
                        layer_customize<next_layer_type>::async_read(
                            a_strm.nl_,
                            as::buffer(address, 2),
                            force_move(self)
                        );
                    }
                else {
                    async_read(
                        a_strm.nl_,
                        as::buffer(address, 2),
                        as::transfer_all(),
                        force_move(self)
                    );
                }
            }
            else {
                // start bulk read
                auto strm_copy = strm;
                async_read_some(
                    strm_copy,
                    force_move(self)
                );
            }
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    // for non bulk read
    template <typename Self>
    void operator()(
        Self& self,
        error_code ec,
        std::size_t bytes_transferred
    ) {
        (void)bytes_transferred; // Ignore unused argument in release build
        auto& a_strm{*strm};

        if (ec) {
            next();
            self.complete(ec, buffer{});
            return;
        }

        switch (state) {
        case remaining_length:
            BOOST_ASSERT(bytes_transferred == rl_expected);
            rl_expected = 1;
            received += bytes_transferred;
            if (a_strm.header_remaining_length_buf_[received - 1] & 0b10000000) {
                // remaining_length continues
                if (received == 5) {
                    next();
                    ASYNC_MQTT_LOG("mqtt_impl", warning)
                        << ASYNC_MQTT_ADD_VALUE(address, this)
                        << "out of size remaining length";
                    self.complete(
                        make_error_code(disconnect_reason_code::packet_too_large),
                        buffer{}
                    );
                    return;
                }
                rl += (a_strm.header_remaining_length_buf_[received - 1] & 0b01111111) * mul;
                mul *= 128;
                auto address = &a_strm.header_remaining_length_buf_[received];
                if constexpr (
                    has_async_read<next_layer_type>::value) {
                        layer_customize<next_layer_type>::async_read(
                            a_strm.nl_,
                            as::buffer(address, 1),
                            force_move(self)
                        );
                    }
                else {
                    async_read(
                        a_strm.nl_,
                        as::buffer(address, 1),
                        as::transfer_all(),
                        force_move(self)
                    );
                }
            }
            else {
                // remaining_length end
                rl += (a_strm.header_remaining_length_buf_[received - 1] & 0b01111111) * mul;
                spca = make_shared_ptr_char_array(
                    received + rl
                );
                std::copy(
                    a_strm.header_remaining_length_buf_.data(),
                    a_strm.header_remaining_length_buf_.data() + received, spca.get()
                );

                if (rl == 0) {
                    next();
                    auto ptr = spca.get();
                    self.complete(ec, buffer{ptr, ptr + received + rl, force_move(spca)});
                    return;
                }
                else {
                    state = complete;
                    auto address = &spca[std::ptrdiff_t(received)];
                    if constexpr (
                        has_async_read<next_layer_type>::value) {
                            layer_customize<next_layer_type>::async_read(
                                a_strm.nl_,
                                as::buffer(address, rl),
                                force_move(self)
                            );
                        }
                    else {
                        async_read(
                            a_strm.nl_,
                            as::buffer(address, rl),
                            as::transfer_all(),
                            force_move(self)
                        );
                    }
                }
            }
            break;
        case complete: {
            next();
            auto ptr = spca.get();
            self.complete(ec, buffer{ptr, ptr + received + rl, force_move(spca)});
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    // finish bulk read
    template <typename Self>
    void operator()(
        Self& self,
        error_code const& ec,
        buffer packet
    ) {
        next();
        self.complete(ec, force_move(packet));
    }

    void next() {
        auto& a_strm{*strm};
        a_strm.read_queue_.stop_work();
        as::post(
            a_strm.get_executor(),
            [strm = force_move(strm)] {
                strm->read_queue_.poll_one();
            }
        );
    }
};

template <typename NextLayer>
inline
void
stream_impl<NextLayer>::init_read() {
    read_state_ = read_state::fixed_header;
    header_remaining_length_buf_.clear();
    remaining_length_ = 0;
    multiplier_ = 1;
}

template <typename NextLayer>
struct stream_impl<NextLayer>::stream_read_some_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;
    using next_layer_type = stream_type::next_layer_type;

    std::shared_ptr<stream_type> strm;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_strm{*strm};
        if (a_strm.read_packets_.empty()) {
            if constexpr (
                has_async_read_some<next_layer_type>::value) {
                    layer_customize<next_layer_type>::async_read_some(
                        a_strm.nl_,
                        a_strm.read_buf_.prepare(a_strm.bulk_read_buffer_size_),
                        force_move(self)
                    );
            }
            else {
                a_strm.nl_.async_read_some(
                    a_strm.read_buf_.prepare(a_strm.bulk_read_buffer_size_),
                    force_move(self)
                );
            }
        }
        else {
            auto [ec, packet] = force_move(a_strm.read_packets_.front());
            a_strm.read_packets_.pop_front();
            self.complete(ec, force_move(packet));
        }
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code const& ec,
        std::size_t bytes_transferred
    ) {
        auto& a_strm{*strm};
        if (ec) {
            a_strm.init_read();
            a_strm.read_packets_.emplace_back(ec);
        }
        else {
            a_strm.read_buf_.commit(bytes_transferred);
            a_strm.parse_packet();
            as::dispatch(
                a_strm.get_executor(),
                force_move(self)
            );
        }
    }
};

template <typename NextLayer>
template <typename CompletionToken>
auto
stream_impl<NextLayer>::async_read_some(
    this_type_sp impl,
    CompletionToken&& token
) {
    BOOST_ASSERT(impl);
    auto exe = impl->get_executor();
    return
        as::async_compose<
            CompletionToken,
            void(error_code, buffer)
        >(
            stream_read_some_op{
                force_move(impl)
            },
            token,
            exe
        );
}


template <typename NextLayer>
inline
void
stream_impl<NextLayer>::parse_packet() {
    while (read_buf_.size() != 0) {
        switch (read_state_) {
        case read_state::fixed_header: {
            if (read_buf_.size() > 0) {
                std::istream is{&read_buf_};
                char fixed_header;
                is.read(&fixed_header, 1);
                header_remaining_length_buf_.push_back(fixed_header);
                read_state_ = read_state::remaining_length;
            }
        } break;
        case read_state::remaining_length: {
            while (read_buf_.size() > 0) {
                std::istream is{&read_buf_};
                char encoded_byte;
                is.read(&encoded_byte, 1);
                header_remaining_length_buf_.push_back(encoded_byte);
                remaining_length_ += (std::uint8_t(encoded_byte) & 0b0111'1111) * multiplier_;
                multiplier_ *= 128;
                if ((encoded_byte & 0b1000'0000) == 0) {
                    read_state_ = read_state::payload;
                    break;
                }
                if (multiplier_ == 128 * 128 * 128 * 128) {
                    read_packets_.emplace_back(make_error_code(disconnect_reason_code::packet_too_large));
                    init_read();
                    return;
                }
            }
            if (read_state_ != read_state::payload) {
                return;
            }
        } break;
        case read_state::payload: {
            if (read_buf_.size() >= remaining_length_) {
                std::size_t total_size = header_remaining_length_buf_.size() + remaining_length_;
                auto spca = make_shared_ptr_char_array(total_size);
                std::copy_n(
                    header_remaining_length_buf_.data(),
                    header_remaining_length_buf_.size(),
                    spca.get()
                );
                std::istream is{&read_buf_};
                auto ptr = spca.get();
                is.read(ptr + header_remaining_length_buf_.size(), static_cast<std::streamsize>(remaining_length_));
                read_packets_.emplace_back(
                    buffer{ptr, total_size, force_move(spca)}
                );

                init_read();
            }
            else {
                return;
            }
        } break;
        }
    }
}

} // namespace detail

template <typename NextLayer>
template <typename CompletionToken>
auto
stream<NextLayer>::async_read_packet(
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return
        as::async_compose<
            CompletionToken,
            void(error_code, buffer)
        >(
            typename impl_type::stream_read_packet_op{
                impl_
            },
            token,
            get_executor()
        );
}


} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_IMPL_STREAM_READ_PACKET_HPP
