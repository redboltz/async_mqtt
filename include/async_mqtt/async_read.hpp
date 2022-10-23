// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASYNC_READ_HPP)
#define ASYNC_MQTT_ASYNC_READ_HPP

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/static_vector.hpp>

namespace async_mqtt {

namespace as = boost::asio;


template <typename Stream>
struct async_read_packet_impl {
    async_read_packet_impl(Stream& stream):stream{stream}
    {}
    ~async_read_packet_impl() {
        std::cout << "dest" << std::endl;
    }
    Stream& stream;
    std::size_t received = 0;
    shared_ptr_array hrl = make_shared_ptr_array(5);
    std::uint32_t mul = 1;
    std::uint32_t rl = 0;
    shared_ptr_array spa;
    enum { header, remaining_length, complete } state = header;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        state = header;
        // read fixed_header
        auto& a_hrl{hrl};
        async_read(
            stream,
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
                auto& a_hrl{hrl};
                async_read(
                    stream,
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
                    stream,
                    as::buffer(&a_hrl[received], 1),
                    force_move(self)
                );
            }
            else {
                // remaining_length end
                rl += (hrl[received - 1] & 0b01111111) * mul;
                spa = make_shared_ptr_array(received + rl);
                std::copy(hrl.get(), hrl.get() + received, spa.get());
                if (rl == 0) {
                    self.complete(ec, buffer{spa.get(), spa.get() + received, spa});
                }
                else {
                    state = complete;
                    auto& a_spa{spa};
                    async_read(
                        stream,
                        as::buffer(&a_spa[received], rl),
                        force_move(self)
                    );
                }
            }
            break;
        case complete:
            self.complete(ec, buffer{spa.get(), spa.get() + received + rl, spa});
            break;
        }
    }
};

template <
    typename Stream,
    typename CompletionToken,
    typename std::enable_if_t<
        std::is_invocable<CompletionToken, boost::system::error_code, buffer>::value
    >* = nullptr
>
auto async_read_packet(
    Stream& stream,
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(boost::system::error_code const&, buffer)
        >(
            async_read_packet_impl<Stream>{
                stream
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_ASYNC_READ_HPP
