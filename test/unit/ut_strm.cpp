// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>

#include <boost/asio.hpp>

#include <async_mqtt/util/stream.hpp>
#include <async_mqtt/util/scope_guard.hpp>
#include <async_mqtt/impl/buffer_to_packet_variant.hpp>

#include "stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_strm)

namespace am = async_mqtt;
namespace as = boost::asio;

// packet_id is hard coded in this test case for just testing.
// but users need to get packet_id via ep->acquire_unique_packet_id(...)
// see other test cases.

// v3_1_1

BOOST_AUTO_TEST_CASE(write_cont) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard{as::make_work_guard(ioc.get_executor())};

    using strm_t = am::stream<async_mqtt::stub_socket>;
    {
        auto s = strm_t::create(
            // for stub_socket args
            version,
            ioc.get_executor()
        );
        auto sg =
            am::shared_scope_guard(
                [&, s] {
                    guard.reset();
                }
            );

        auto p = am::v3_1_1::pingreq_packet();
        s->async_write_packet(p, [sg](am::error_code const&, std::size_t){});
        s->async_write_packet(p, [sg](am::error_code const&, std::size_t){});
        s->async_write_packet(p, [sg](am::error_code const&, std::size_t){});
        s->async_write_packet(p, [sg](am::error_code const&, std::size_t){});
        s->async_write_packet(p, [sg](am::error_code const&, std::size_t){});
        s->async_write_packet(p, [sg](am::error_code const&, std::size_t){});
        s->async_write_packet(p, [sg](am::error_code const&, std::size_t){});
        s->async_write_packet(p, [sg](am::error_code const&, std::size_t){});
    }
    ioc.run();
}

BOOST_AUTO_TEST_SUITE_END()
