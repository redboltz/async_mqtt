// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>

namespace as = boost::asio;
namespace bs = boost::beast;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    as::ip::tcp::acceptor ac{ioc, endpoint};
    bs::websocket::stream<as::ip::tcp::socket> ws{ioc};


    std::function <void()> do_echo;

    do_echo =
        [&] {
            auto wsfb = std::make_shared<bs::flat_buffer>();
            ws.async_read(
                *wsfb,
                [&, wsfb]
                (boost::system::error_code ec, std::size_t bytes_transferred) {
                    std::cout << "read : " << ec.message() << " " << bytes_transferred << std::endl;
                    if (ec) return;
                    auto buf = std::make_shared<std::vector<char>>(wsfb->size());
                    as::buffer_copy(as::buffer(*buf), wsfb->data(), wsfb->size());
                    ws.async_write(
                        as::buffer(*buf),
                        [&, buf]
                        (boost::system::error_code ec, std::size_t bytes_transferred) {
                            std::cout << "write: " << ec.message() << " " << bytes_transferred << std::endl;
                            if (ec) return;
                            do_echo();
                        }
                    );
                    wsfb->consume(wsfb->size());
                }
            );
        };

    ac.async_accept(
        ws.next_layer(),
        [&]
        (boost::system::error_code ec) {
            std::cout << "tcp accept : " << ec.message() << std::endl;
            if (ec) return;
            try {
                auto sb = std::make_shared<boost::asio::streambuf>();
                auto request = std::make_shared<bs::http::request<boost::beast::http::string_body>>();
                bs::http::async_read(
                    ws.next_layer(),
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
                            ws.set_option(opt);
                            ws.async_accept(
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
