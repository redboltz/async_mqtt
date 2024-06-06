// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_STREAM_READ_PACKET_HPP)
#define ASYNC_MQTT_IMPL_STREAM_READ_PACKET_HPP

#include <async_mqtt/error.hpp>
#include <async_mqtt/util/stream.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>

namespace async_mqtt {

template <typename NextLayer>
struct stream<NextLayer>::stream_read_packet_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;
    using next_layer_type = stream_type::next_layer_type;

    stream_type& strm;
    std::size_t received = 0;
    std::uint32_t mul = 1;
    std::uint32_t rl = 0;
    std::size_t rl_expected = 2;
    std::shared_ptr<char[]> spca = nullptr;
    stream_type_sp life_keeper = strm.shared_from_this();
    enum { dispatch, post, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        switch (state) {
        case dispatch: {
            state = post;
            auto& a_strm{strm};
            as::dispatch(
                a_strm.get_executor(),
                force_move(self)
            );
        } break;
        case post: {
            state = complete;
            auto& a_strm{strm};
            a_strm.read_queue_.post(
                force_move(self)
            );
        } break;
        case complete: {
            strm.read_queue_.start_work();
            strm.read_some(self);
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

#if 0
    template <typename Self>
    void operator()(
        Self& self,
        error_code ec,
        std::size_t bytes_transferred
    ) {
        (void)bytes_transferred; // Ignore unused argument in release build

        if (ec) {
            self.complete(ec, buffer{});
            return;
        }

        switch (state) {
        case remaining_length:
            BOOST_ASSERT(bytes_transferred == rl_expected);
            rl_expected = 1;
            received += bytes_transferred;
            if (strm.header_remaining_length_buf_[received - 1] & 0b10000000) {
                // remaining_length continues
                if (received == 5) {
                    ASYNC_MQTT_LOG("mqtt_impl", warning)
                        << ASYNC_MQTT_ADD_VALUE(address, this)
                        << "out of size remaining length";
                    self.complete(
                        make_error_code(disconnect_reason_code::packet_too_large),
                        buffer{}
                    );
                    return;
                }
                rl += (strm.header_remaining_length_buf_[received - 1] & 0b01111111) * mul;
                mul *= 128;
                auto address = &strm.header_remaining_length_buf_[received];
                auto& a_strm{strm};
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
                rl += (strm.header_remaining_length_buf_[received - 1] & 0b01111111) * mul;

                BOOST_ASIO_REBIND_ALLOC(
                    typename as::associated_allocator<Self>::type,
                    char
                )
                alloc{
                    as::get_associated_allocator(self)
                };
                spca = allocate_shared_ptr_char_array(
                    alloc,
                    received + rl
                );
                std::copy(
                    strm.header_remaining_length_buf_.data(),
                    strm.header_remaining_length_buf_.data() + received, spca.get()
                );

                if (rl == 0) {
                    auto ptr = spca.get();
                    self.complete(ec, buffer{ptr, ptr + received + rl, force_move(spca)});
                    return;
                }
                else {
                    state = complete;
                    auto address = &spca[std::ptrdiff_t(received)];
                    auto& a_strm{strm};
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
            auto ptr = spca.get();
            self.complete(ec, buffer{ptr, ptr + received + rl, force_move(spca)});
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }
#endif

};

template <typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, buffer)
)
stream<NextLayer>::async_read_packet(
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
        void(error_code, buffer)
    >(
        stream_read_packet_op{
            *this
        },
        token,
        get_executor()
    );
}

template <typename NextLayer>
inline
void
stream<NextLayer>::init_read() {
    read_state_ = read_state::fixed_header;
    header_remaining_length_buf_.clear();
    remaining_length_ = 0;
    multiplier_ = 1;
}

template <typename NextLayer>
template <typename Self>
inline
void
stream<NextLayer>::read_some(Self& self) {
    if (read_packets_.empty()) {
        nl_.async_read_some(
            read_buf_.prepare(4096),
            [this, &self, life_keeper = this->shared_from_this()]
            (error_code const& ec, std::size_t bytes_transferred) {
                if (ec) {
                    init_read();
                    read_packets_.emplace_back(ec);
                }
                else {
                    read_buf_.commit(bytes_transferred);
                    parse_packet(self);
                    read_some(self);
                }
            }
        );
    }
    else {
        read_queue_.stop_work();
        auto [ec, packet] = force_move(read_packets_.front());
        read_packets_.pop_front();
        self.complete(ec, force_move(packet));
        read_queue_.poll_one();
    }
}


template <typename NextLayer>
template <typename Self>
inline
void
stream<NextLayer>::parse_packet(Self& self) {
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
                if (multiplier_ > 128 * 128 * 128) {
                    read_packets_.emplace_back(make_error_code(disconnect_reason_code::packet_too_large));
                    init_read();
                    return;
                }
                multiplier_ *= 128;
                if ((encoded_byte & 0b1000'0000) == 0) {
                    read_state_ = read_state::payload;
                    break;
                }
            }
            if (read_state_ != read_state::payload) {
                return;
            }
        } break;
        case read_state::payload: {
            if (read_buf_.size() >= remaining_length_) {
                BOOST_ASIO_REBIND_ALLOC(
                    typename as::associated_allocator<Self>::type,
                    char
                )
                alloc{
                    as::get_associated_allocator(self)
                };
                std::size_t total_size = header_remaining_length_buf_.size() + remaining_length_;
                auto spca = allocate_shared_ptr_char_array(alloc, total_size);
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
        } break;
        }
    }
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_STREAM_READ_PACKET_HPP
