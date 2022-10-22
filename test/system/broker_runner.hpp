// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TEST_SYSTEM_BROKER_RUNNER_HPP)
#define ASYNC_MQTT_TEST_SYSTEM_BROKER_RUNNER_HPP

#include <thread>

#include <boost/process.hpp>
#include <boost/process/args.hpp>
#include <boost/asio.hpp>
#include <boost/predef.h>
namespace as = boost::asio;
namespace pr = boost::process;

inline void kill_broker() {
#if _WIN32
    std::system("taskkill /IM broker.exe /F");
#else  // _WIN32
    std::system("pkill broker");
#endif // _WIN32
}

struct broker_killer {
    broker_killer() {
        kill_broker();
    }
};

struct broker_runner : broker_killer {
    broker_runner(
        std::string const& config = "st_broker.conf",
        std::string const& auth = "st_auth.json"
    )
#if _WIN32
        :brk{pr::search_path("broker"), "--cfg", config, "--auth_file", auth}
#else  // _WIN32
         :brk{pr::args({"../../tool/broker", "--cfg", config, "--auth_file", auth})}
#endif // _WIN32
    {
        {
            as::io_context ioc;
            as::ip::address address = boost::asio::ip::make_address("127.0.0.1");
            as::ip::tcp::endpoint endpoint{address, 1883};
            as::ip::tcp::socket s{ioc};
            std::function<void(boost::system::error_code const&)> f =
                [&](boost::system::error_code const& ec) {
                    if (ec) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        s = as::ip::tcp::socket{ioc};
                        s.async_connect(
                            endpoint,
                            f
                        );
                    }
                };
            s.async_connect(
                endpoint,
                f
            );
            ioc.run();
        }
#if defined(ASYNC_MQTT_USE_TLS)
        {
            as::io_context ioc;
            as::ip::address address = boost::asio::ip::make_address("127.0.0.1");
            as::ip::tcp::endpoint endpoint{address, 8883};
            as::ip::tcp::socket s{ioc};
            std::function<void(boost::system::error_code const&)> f =
                [&](boost::system::error_code const& ec) {
                    if (ec) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        s = as::ip::tcp::socket{ioc};
                        s.async_connect(
                            endpoint,
                            f
                        );
                    }
                };
            s.async_connect(
                endpoint,
                f
            );
            ioc.run();
        }
#endif // defined(ASYNC_MQTT_USE_TLS)
#if defined(ASYNC_MQTT_USE_WS)
        {
            as::io_context ioc;
            as::ip::address address = boost::asio::ip::make_address("127.0.0.1");
            as::ip::tcp::endpoint endpoint{address, 10080};
            as::ip::tcp::socket s{ioc};
            std::function<void(boost::system::error_code const&)> f =
                [&](boost::system::error_code const& ec) {
                    if (ec) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        s = as::ip::tcp::socket{ioc};
                        s.async_connect(
                            endpoint,
                            f
                        );
                    }
                };
            s.async_connect(
                endpoint,
                f
            );
            ioc.run();
        }
#endif // defined(ASYNC_MQTT_USE_WS)
#if defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)
        {
            as::io_context ioc;
            as::ip::address address = boost::asio::ip::make_address("127.0.0.1");
            as::ip::tcp::socket s{ioc};
            as::ip::tcp::endpoint endpoint{address, 10443};
            std::function<void(boost::system::error_code const&)> f =
                [&](boost::system::error_code const& ec) {
                    if (ec) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        s = as::ip::tcp::socket{ioc};
                        s.async_connect(
                            endpoint,
                            f
                        );
                    }
                };
            s.async_connect(
                endpoint,
                f
            );
            ioc.run();
        }
#endif // defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)
    }
    ~broker_runner() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        kill_broker();
        brk.join();
    }
    pr::child brk;
};

#endif // ASYNC_MQTT_TEST_SYSTEM_BROKER_RUNNER_HPP
