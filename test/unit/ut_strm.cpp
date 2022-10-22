// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>

#include <boost/asio.hpp>

#include <async_mqtt/stream.hpp>
#include <async_mqtt/packet/v3_1_1_pingreq.hpp>

#include "stub_socket.hpp"
#include "packet_compare.hpp"

BOOST_AUTO_TEST_SUITE(ut_strm)

namespace am = async_mqtt;
namespace as = boost::asio;

// packet_id is hard coded in this test case for just testing.
// but users need to get packet_id via ep.acquire_unique_packet_id(...)
// see other test cases.

// v3_1_1

BOOST_AUTO_TEST_CASE(write_cont) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;

    using strm_t = am::stream<async_mqtt::stub_socket>;
    auto s = std::make_shared<strm_t>(
        // for stub_socket args
        version,
        ioc
    );

    auto p = am::v3_1_1::pingreq_packet();
    s->write_packet(p, [](am::error_code const&, std::size_t){});
    s->write_packet(p, [](am::error_code const&, std::size_t){});
    s->write_packet(p, [](am::error_code const&, std::size_t){});
    s->write_packet(p, [](am::error_code const&, std::size_t){});
    s->write_packet(p, [](am::error_code const&, std::size_t){});
    s->write_packet(p, [](am::error_code const&, std::size_t){});
    s->write_packet(p, [](am::error_code const&, std::size_t){});
    s->write_packet(p, [](am::error_code const&, std::size_t){});
    ioc.run();
}

BOOST_AUTO_TEST_SUITE_END()
