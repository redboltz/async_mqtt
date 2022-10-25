// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>

#include <async_mqtt/stream.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>

namespace as = boost::asio;
namespace bs = boost::beast;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    as::ip::tcp::acceptor ac{ioc, endpoint};
    async_mqtt::stream<bs::websocket::stream<as::ip::tcp::socket>> ams{ioc};


    std::function <void()> do_echo;

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
        ams.next_layer().next_layer(),
        [&]
        (boost::system::error_code ec) {
            std::cout << "tcp accept : " << ec.message() << std::endl;
            if (ec) return;
            try {
                auto sb = std::make_shared<boost::asio::streambuf>();
                auto request = std::make_shared<bs::http::request<boost::beast::http::string_body>>();
                bs::http::async_read(
                    ams.next_layer().next_layer(),
                    *sb,
                    *request,
                    [&, sb, request]
                    (boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                        std::cout << "http accept: " << ec.message() << " " << bytes_transferred << std::endl;
                        if (ec) return;
                        if (bs::websocket::is_upgrade(*request)) {
                            std::cout << "upgrade request" << std::endl;
                            bs::websocket::permessage_deflate opt;
                            opt.client_enable = true;
                            opt.server_enable = true;
                            ams.next_layer().set_option(opt);
                            ams.next_layer().async_accept(
                                *request,
                                [&, request]
                                (boost::system::error_code ec) {
                                    std::cout << "ws accept  : " << ec.message() << std::endl;
                                    if (ec) return;
                                    do_echo();
                                }
                            );
                        }
                    }
                );
            }
            catch(std::exception const& e) {
                std::cout << e.what() << std::endl;;
            }
        }
    );
    ioc.run();
}
