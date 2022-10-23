// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/asio.hpp>

#include <async_mqtt/async_write.hpp>
#include <async_mqtt/async_read.hpp>

namespace as = boost::asio;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    as::ip::tcp::socket s{ioc};

    auto packet =
        async_mqtt::publish_packet{
            42,
            async_mqtt::allocate_buffer("topic1"),
            async_mqtt::allocate_buffer("payload"),
            async_mqtt::pub::opts{}
        };
    s.async_connect(
        endpoint,
        [&]
        (boost::system::error_code const& ec) {
            std::cout << "connect: " << ec.message() << std::endl;
            async_mqtt::async_write(
                s,
                async_mqtt::force_move(packet),
                [&]
                (boost::system::error_code const& ec, std::size_t bytes_transferred) mutable {
                    std::cout << "write: " << ec.message() << " " << bytes_transferred << std::endl;
                    async_mqtt::async_read_packet(
                        s,
                        [&]
                        (boost::system::error_code const& ec, async_mqtt::buffer packet) mutable {
                            std::cout << "read: " << ec.message() << " " << packet.size() << std::endl;
                        }
                    );
                }
            );
        }
    );

    ioc.run();
}
