// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <functional>

#include <boost/asio.hpp>

#include <async_mqtt/async_write.hpp>

namespace as = boost::asio;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};
    as::ip::tcp::socket s{ioc};
    as::ip::tcp::acceptor ac{ioc, endpoint};

    std::string rstr;
    rstr.resize(1024);
    std::string wstr;
    std::function<void()> do_echo;
    do_echo =
        [&] {
            s.async_read_some(
                as::buffer(rstr),
                [&]
                (boost::system::error_code const& ec, std::size_t bytes_transferred) mutable {
                    std::cout << "read : " << ec.message() << " " << bytes_transferred << std::endl;
                    wstr = rstr.substr(0, bytes_transferred);
                    s.async_write_some(
                        as::buffer(wstr),
                        [&]
                        (boost::system::error_code const& ec, std::size_t bytes_transferred) mutable {
                            std::cout << "write: " << ec.message() << " " << bytes_transferred << std::endl;
                            do_echo();
                        }
                    );
                }
            );
        };

    ac.async_accept(
        s,
        [&](boost::system::error_code const& ec) {
            std::cout << "accept: " << ec.message() << std::endl;
            do_echo();
        }
    );
    ioc.run();
}
