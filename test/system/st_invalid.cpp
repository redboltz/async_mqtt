// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"
#include "broker_runner.hpp"
#include "coro_base.hpp"

#include <async_mqtt/all.hpp>
#include <boost/asio/yield.hpp>

BOOST_AUTO_TEST_SUITE(st_invalid)

namespace am = async_mqtt;
namespace as = boost::asio;

char invalid_remaining_length_packet[] { 0x10, char(0xff), char(0xff), char(0xff), char(0xff)};

BOOST_AUTO_TEST_CASE(remaining_length) {
    broker_runner br;
    as::io_context ioc;
    using ep_t = am::endpoint<am::role::client, am::protocol::mqtt>;
    auto amep = ep_t(
        am::protocol_version::v3_1_1,
        ioc.get_executor()
    );

    struct tc : coro_base<ep_t> {
        using coro_base<ep_t>::coro_base;
    private:
        void proc(
            am::optional<am::error_code> ec,
            am::optional<am::system_error> /*se*/,
            am::optional<am::packet_variant> pv,
            am::optional<packet_id_t> /*pid*/
        ) override {
            reenter(this) {
                yield ep().next_layer().async_connect(
                    dest(),
                    *this
                );
                BOOST_TEST(*ec == am::error_code{});
                yield as::async_write(
                    ep().next_layer(),
                    as::buffer(invalid_remaining_length_packet),
                    *this
                );
                BOOST_TEST(!*ec);
                yield ep().recv(*this);
                BOOST_TEST(!*pv); // auto close if receive error
                yield set_finish();
            }
        }
    };

    tc t{amep, "127.0.0.1", 1883};
    t();
    ioc.run();
    BOOST_TEST(t.finish());
}


BOOST_AUTO_TEST_SUITE_END()

#include <boost/asio/unyield.hpp>
