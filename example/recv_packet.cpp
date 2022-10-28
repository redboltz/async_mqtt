// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <functional>

#include <boost/asio.hpp>

#include <async_mqtt/stream.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/predefined_underlying_layer.hpp>

namespace as = boost::asio;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    async_mqtt::stream<async_mqtt::protocol::mqtt> ams{ioc.get_executor()};
    as::ip::tcp::acceptor ac{ioc, endpoint};

    std::string rstr;
    rstr.resize(1024);
    std::string wstr;
    std::function<void()> do_echo;
    do_echo =
        [&] {
            ams.read_packet(
                [&]
                (boost::system::error_code const& ec, async_mqtt::buffer buf) mutable {
                    std::cout << "read : " << ec.message() << " " << buf.size() << std::endl;
                    if (ec) return;
                    if (auto pv = async_mqtt::buffer_to_packet_variant(
                            force_move(buf),
                            async_mqtt::protocol_version::v3_1_1
                        )
                    ) {
                        auto const& cbs = pv.const_buffer_sequence();
                        ams.write_packet(
                            cbs,
                            [&, pv = async_mqtt::force_move(pv)]
                            (boost::system::error_code const& ec, std::size_t bytes_transferred) mutable {
                                std::cout << "write: " << ec.message() << " " << bytes_transferred << std::endl;
                                if (ec) return;
                                do_echo();
                            }
                        );
                    }
                    else {
                        std::cout << "protocol error" << std::endl;
                    }
                }
            );
        };

    ac.async_accept(
        ams.next_layer(),
        [&](boost::system::error_code const& ec) {
            std::cout << "accept: " << ec.message() << std::endl;
            if (ec) return;
            do_echo();
        }
    );
    ioc.run();
}
