// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TEST_SYSTEM_BROKER_RUNNER_HPP)
#define ASYNC_MQTT_TEST_SYSTEM_BROKER_RUNNER_HPP

#include <thread>
#include <optional>

#include <boost/process.hpp>

#if BOOST_VERSION < 108600
#include <boost/process/args.hpp>
#endif // BOOST_VERSION < 108600

#include <boost/asio.hpp>
#include <boost/predef.h>

#include <boost/test/unit_test.hpp>

namespace as = boost::asio;
namespace pr = boost::process;
namespace am = async_mqtt;

#if BOOST_VERSION < 108600
using pid_type = pr::pid_t;
using process = pr::child;
#else  // BOOST_VERSION < 108600
using pid_type = pr::pid_type;
using process = pr::process;
#endif // BOOST_VERSION < 108600

inline bool launch_broker_required() {
    auto argc = boost::unit_test::framework::master_test_suite().argc;
    if (argc >= 3) {
        auto argv = boost::unit_test::framework::master_test_suite().argv;
        auto launch = std::string_view(argv[2]);
        if (launch == "no" || launch == "no_launch") {
            return false;
        }
    }
    return true;
}

inline void kill_broker(pid_type pid) {
    if (!launch_broker_required()) return;
#if _WIN32
    {
        std::string cmd = "taskkill /F /pid ";
        cmd += std::to_string(pid);
        std::system(cmd.c_str());
    }
#else  // _WIN32
    {
        std::string cmd = "kill ";
        cmd += std::to_string(pid);
        std::system(cmd.c_str());
    }
#endif // _WIN32
}

struct broker_runner {
    broker_runner(
        std::string const& config = "st_broker.conf",
        std::string const& auth = "st_auth.json"
    ) {
        if (!launch_broker_required()) return;
        auto level_opt =
            [&] () -> std::optional<std::size_t> {
            auto argc = boost::unit_test::framework::master_test_suite().argc;
            if (argc >= 2) {
                auto argv = boost::unit_test::framework::master_test_suite().argv;
                auto sevstr = std::string_view(argv[1]);
                if (sevstr == "fatal") {
                    return 0;
                }
                else if (sevstr == "error") {
                    return 1;
                }
                else if (sevstr == "warning") {
                    return 2;
                }
                else if (sevstr == "info") {
                    return 3;
                }
                else if (sevstr == "debug") {
                    return 4;
                }
                else if (sevstr == "trace") {
                    return 5;
                }
            }
            return std::nullopt;
        } ();
#if BOOST_VERSION < 108600

#if _WIN32
        if (level_opt) {
            brk.emplace(
                pr::search_path("broker"),
                "--cfg", config,
                "--auth_file", auth,
                "--verbose", std::to_string(*level_opt)
            );
        }
        else {
            brk.emplace(
                pr::search_path("broker"),
                "--cfg", config,
                "--auth_file", auth
            );
        }
#else  // _WIN32
        std::vector<std::string> args;
        args.emplace_back("--cfg");
        args.emplace_back(config);
        args.emplace_back("--auth_file");
        args.emplace_back(auth);
        if (level_opt) {
            args.emplace_back("--verbose");
            args.emplace_back(std::to_string(*level_opt));
        }
        brk.emplace("../../tool/broker", pr::args(args));
#endif // _WIN32

#else  // BOOST_VERSION < 108600
        std::vector<std::string> args;
        args.emplace_back("--cfg");
        args.emplace_back(config);
        args.emplace_back("--auth_file");
        args.emplace_back(auth);
        if (level_opt) {
            args.emplace_back("--verbose");
            args.emplace_back(std::to_string(*level_opt));
        }
        brk.emplace(ioc, "../../tool/broker", args);
#endif // BOOST_VERSION < 108600

        // wait broker's socket ready
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
        if (brk) {
            kill_broker(brk->id());
#if BOOST_VERSION < 108600
            brk->join();
#endif // BOOST_VERSION < 108600
        }
    }
    as::io_context ioc;
    std::optional<process> brk;
};

#endif // ASYNC_MQTT_TEST_SYSTEM_BROKER_RUNNER_HPP
