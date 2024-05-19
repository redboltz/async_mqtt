// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_STREAM_READ_PACKET_HPP)
#define ASYNC_MQTT_IMPL_STREAM_READ_PACKET_HPP

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
    std::shared_ptr<char[]> spca = nullptr;
    stream_type_sp life_keeper = strm.shared_from_this();
    enum { header, remaining_length, complete } state = header;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        switch (state) {
        case header: {
            // read fixed_header
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
        (void)bytes_transferred; // Ignore unused argument in release build

        if (ec) {
            self.complete(ec, buffer{});
            return;
        }

        switch (state) {
        case header:
            BOOST_ASSERT(bytes_transferred == 1);
            state = remaining_length;
            ++received;
            // read the first remaining_length
            {
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
            break;
        case remaining_length:
            BOOST_ASSERT(bytes_transferred == 1);
            ++received;
            if (strm.header_remaining_length_buf_[received - 1] & 0b10000000) {
                // remaining_length continues
                if (received == 5) {
                    ASYNC_MQTT_LOG("mqtt_impl", warning)
                        << ASYNC_MQTT_ADD_VALUE(address, this)
                        << "out of size remaining length";
                    self.complete(
                        sys::errc::make_error_code(sys::errc::protocol_error),
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
        token
    );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_STREAM_READ_PACKET_HPP
