// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>

#include <boost/asio.hpp>

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/util/hex_dump.hpp>

#include "stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_ep_topic_alias)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v5_ta1) {
    auto p = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{}
    };

    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        am::protocol_version::v5,
        ioc,
        std::deque<am::packet_variant> {
            p, p
        }
    };

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            auto w_cbs = wp.const_buffer_sequence();
            auto [wb, we] = am::make_packet_range(w_cbs);
            auto e_cbs = p.const_buffer_sequence();
            auto [eb, ee] = am::make_packet_range(e_cbs);
            BOOST_TEST(std::equal(wb, we, eb, ee));
            std::cout << am::hex_dump(wp) << std::endl;
        }
    );

    auto f_send = ep.send(
        p,
        as::use_future
    );


    auto ec = f_send.get();


    BOOST_TEST(!ec);
    std::cout << ec.what() << std::endl;

    {
        auto f_recv = ep.recv(as::use_future);
        auto pv = f_recv.get();
        pv.visit(
            am::overload {
                [&](am::v5::connect_packet const& rp) {
                    auto r_cbs = rp.const_buffer_sequence();
                    auto [rb, re] = am::make_packet_range(r_cbs);
                    auto e_cbs = p.const_buffer_sequence();
                    auto [eb, ee] = am::make_packet_range(e_cbs);
                    BOOST_TEST(std::equal(rb, re, eb, ee));
                    std::cout << am::hex_dump(rp) << std::endl;
                },
                [&](auto const&){}
                }
        );
    }
    {
        auto f_recv = ep.recv(as::use_future);
        auto pv = f_recv.get();
        pv.visit(
            am::overload {
                [&](am::v5::connect_packet const& rp) {
                    auto r_cbs = rp.const_buffer_sequence();
                    auto [rb, re] = am::make_packet_range(r_cbs);
                    auto e_cbs = p.const_buffer_sequence();
                    auto [eb, ee] = am::make_packet_range(e_cbs);
                    BOOST_TEST(std::equal(rb, re, eb, ee));
                    std::cout << am::hex_dump(rp) << std::endl;
                },
                [&](auto const&){}
                }
        );
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
