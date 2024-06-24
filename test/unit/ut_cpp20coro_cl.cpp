// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>
#include <tuple>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#if !defined(_MSC_VER)
#include <boost/asio/experimental/promise.hpp>
#include <boost/asio/experimental/use_promise.hpp>
#endif // !defined(_MSC_VER)

#include <async_mqtt/client.hpp>

#include "cpp20coro_stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_cpp20coro_cl)

namespace am = async_mqtt;
namespace as = boost::asio;
using namespace as::experimental::awaitable_operators;

BOOST_AUTO_TEST_CASE(v311_start_success) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_start_error) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::unacceptable_protocol_version
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::connect_return_code::unacceptable_protocol_version);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_subscribe_single_success) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v3_1_1::suback_packet{
                    pid,
                    {am::suback_return_code::success_maximum_qos_2},
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::exactly_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_CHECK(suback_opt);
                BOOST_TEST(*suback_opt == suback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_subscribe_single_error) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v3_1_1::suback_packet{
                    pid,
                    {am::suback_return_code::failure},
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::suback_return_code::failure);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_subscribe_single_mismatch) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();

                // subscribe but unsuback response
                auto unsuback = am::v3_1_1::unsuback_packet{
                    pid
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(unsuback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::disconnect_reason_code::protocol_error);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_subscribe_multi_all_success) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v3_1_1::suback_packet{
                    pid,
                    {
                        am::suback_return_code::success_maximum_qos_0,
                        am::suback_return_code::success_maximum_qos_1,
                    },
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                    {"topic2", am::qos::at_least_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_CHECK(suback_opt);
                BOOST_TEST(*suback_opt == suback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_subscribe_multi_partial_error) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v3_1_1::suback_packet{
                    pid,
                    {
                        am::suback_return_code::success_maximum_qos_0,
                        am::suback_return_code::failure,
                    },
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                    {"topic2", am::qos::at_least_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_CHECK(suback_opt);
                BOOST_TEST(*suback_opt == suback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_subscribe_multi_all_error) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v3_1_1::suback_packet{
                    pid,
                    {
                        am::suback_return_code::failure,
                        am::suback_return_code::failure,
                    },
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                    {"topic2", am::qos::at_least_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::mqtt_error::all_error_detected);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_unsubscribe) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto unsuback = am::v3_1_1::unsuback_packet{
                    pid
                };
                std::vector<am::topic_sharename> unsub_entry{
                    "topic1",
                };

                auto unsuback_opt = co_await(
                    cl->async_unsubscribe(
                        pid,
                        unsub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(unsuback, as::use_awaitable)
                );
                BOOST_CHECK(unsuback_opt);
                BOOST_TEST(*unsuback_opt == unsuback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_unsubscribe_mismatch) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                // unsubscribe send but suback response
                auto suback = am::v3_1_1::suback_packet{
                    pid,
                    {am::suback_return_code::failure},
                };
                std::vector<am::topic_sharename> unsub_entry{
                    "topic1",
                };

                auto unsuback_opt = co_await(
                    cl->async_unsubscribe(
                        pid,
                        unsub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::disconnect_reason_code::protocol_error);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_publish_qos0) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pubres = co_await
                    cl->async_publish(
                        "topic1",
                        "payload1",
                        am::qos::at_most_once,
                        as::use_awaitable
                    );
                BOOST_CHECK(!pubres.puback_opt);
                BOOST_CHECK(!pubres.pubrec_opt);
                BOOST_CHECK(!pubres.pubcomp_opt);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_publish_qos1) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto puback = am::v3_1_1::puback_packet{
                    pid
                };

                auto pubres = co_await(
                    cl->async_publish(
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(puback, as::use_awaitable)
                );
                BOOST_CHECK(pubres.puback_opt);
                BOOST_CHECK(!pubres.pubrec_opt);
                BOOST_CHECK(!pubres.pubcomp_opt);
                BOOST_TEST(*pubres.puback_opt == puback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v311_publish_qos2) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto pubrec = am::v3_1_1::pubrec_packet{
                    pid
                };
                auto pubcomp = am::v3_1_1::pubcomp_packet{
                    pid
                };

                auto pubres = co_await(
                    cl->async_publish(
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(pubrec, as::use_awaitable)
                    &&
                    cl->next_layer().emulate_recv(pubcomp, as::use_awaitable)
                );
                BOOST_CHECK(!pubres.puback_opt);
                BOOST_CHECK(pubres.pubrec_opt);
                BOOST_CHECK(pubres.pubcomp_opt);
                BOOST_TEST(*pubres.pubrec_opt == pubrec);
                BOOST_TEST(*pubres.pubcomp_opt == pubcomp);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_start_success) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_start_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::unsupported_protocol_version
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::connect_reason_code::unsupported_protocol_version);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_subscribe_single_success) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v5::suback_packet{
                    pid,
                    {am::suback_reason_code::granted_qos_2},
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::exactly_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_CHECK(suback_opt);
                BOOST_TEST(*suback_opt == suback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_subscribe_single_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v5::suback_packet{
                    pid,
                    {am::suback_reason_code::not_authorized},
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::suback_reason_code::not_authorized);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_subscribe_multi_all_success) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v5::suback_packet{
                    pid,
                    {
                        am::suback_reason_code::granted_qos_0,
                        am::suback_reason_code::granted_qos_1,
                    },
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                    {"topic2", am::qos::at_least_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_CHECK(suback_opt);
                BOOST_TEST(*suback_opt == suback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_subscribe_multi_partial_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v5::suback_packet{
                    pid,
                    {
                        am::suback_reason_code::granted_qos_0,
                        am::suback_reason_code::not_authorized,
                    },
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                    {"topic2", am::qos::at_least_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_CHECK(suback_opt);
                BOOST_TEST(*suback_opt == suback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_subscribe_multi_all_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto suback = am::v5::suback_packet{
                    pid,
                    {
                        am::suback_reason_code::not_authorized,
                        am::suback_reason_code::not_authorized,
                    },
                };
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::at_most_once},
                    {"topic2", am::qos::at_least_once},
                };

                auto suback_opt = co_await(
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(suback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::mqtt_error::all_error_detected);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}


BOOST_AUTO_TEST_CASE(v5_unsubscribe_single_success) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto unsuback = am::v5::unsuback_packet{
                    pid,
                    {am::unsuback_reason_code::success},
                };
                std::vector<am::topic_sharename> unsub_entry{
                    {"topic1"},
                };

                auto unsuback_opt = co_await(
                    cl->async_unsubscribe(
                        pid,
                        unsub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(unsuback, as::use_awaitable)
                );
                BOOST_CHECK(unsuback_opt);
                BOOST_TEST(*unsuback_opt == unsuback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_unsubscribe_single_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto unsuback = am::v5::unsuback_packet{
                    pid,
                    {am::unsuback_reason_code::not_authorized},
                };
                std::vector<am::topic_sharename> unsub_entry{
                    {"topic1"},
                };

                auto unsuback_opt = co_await(
                    cl->async_unsubscribe(
                        pid,
                        unsub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(unsuback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::unsuback_reason_code::not_authorized);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_unsubscribe_multi_all_success) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto unsuback = am::v5::unsuback_packet{
                    pid,
                    {
                        am::unsuback_reason_code::success,
                        am::unsuback_reason_code::success,
                    },
                };
                std::vector<am::topic_sharename> unsub_entry{
                    {"topic1"},
                    {"topic2"},
                };

                auto unsuback_opt = co_await(
                    cl->async_unsubscribe(
                        pid,
                        unsub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(unsuback, as::use_awaitable)
                );
                BOOST_CHECK(unsuback_opt);
                BOOST_TEST(*unsuback_opt == unsuback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_unsubscribe_multi_partial_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto unsuback = am::v5::unsuback_packet{
                    pid,
                    {
                        am::unsuback_reason_code::success,
                        am::unsuback_reason_code::not_authorized,
                    },
                };
                std::vector<am::topic_sharename> unsub_entry{
                    {"topic1"},
                    {"topic2"},
                };

                auto unsuback_opt = co_await(
                    cl->async_unsubscribe(
                        pid,
                        unsub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(unsuback, as::use_awaitable)
                );
                BOOST_CHECK(unsuback_opt);
                BOOST_TEST(*unsuback_opt == unsuback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_unsubscribe_multi_all_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto unsuback = am::v5::unsuback_packet{
                    pid,
                    {
                        am::unsuback_reason_code::not_authorized,
                        am::unsuback_reason_code::not_authorized,
                    },
                };
                std::vector<am::topic_sharename> unsub_entry{
                    {"topic1"},
                    {"topic2"},
                };

                auto unsuback_opt = co_await(
                    cl->async_unsubscribe(
                        pid,
                        unsub_entry,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(unsuback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::mqtt_error::all_error_detected);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}


BOOST_AUTO_TEST_CASE(v5_publish_qos0) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pubres = co_await
                    cl->async_publish(
                        "topic1",
                        "payload1",
                        am::qos::at_most_once,
                        as::use_awaitable
                    );
                BOOST_CHECK(!pubres.puback_opt);
                BOOST_CHECK(!pubres.pubrec_opt);
                BOOST_CHECK(!pubres.pubcomp_opt);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_publish_qos1_success) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto puback = am::v5::puback_packet{
                    pid,
                    am::puback_reason_code::success
                };

                auto pubres = co_await(
                    cl->async_publish(
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(puback, as::use_awaitable)
                );
                BOOST_CHECK(pubres.puback_opt);
                BOOST_CHECK(!pubres.pubrec_opt);
                BOOST_CHECK(!pubres.pubcomp_opt);
                BOOST_TEST(*pubres.puback_opt == puback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_publish_qos1_success_no_match) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto puback = am::v5::puback_packet{
                    pid,
                    am::puback_reason_code::no_matching_subscribers
                };

                auto pubres = co_await(
                    cl->async_publish(
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(puback, as::use_awaitable)
                );
                BOOST_CHECK(pubres.puback_opt);
                BOOST_CHECK(!pubres.pubrec_opt);
                BOOST_CHECK(!pubres.pubcomp_opt);
                BOOST_TEST(*pubres.puback_opt == puback);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_publish_qos1_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto puback = am::v5::puback_packet{
                    pid,
                    am::puback_reason_code::not_authorized
                };

                auto pubres = co_await(
                    cl->async_publish(
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(puback, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::puback_reason_code::not_authorized);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_publish_qos2_success) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto pubrec = am::v5::pubrec_packet{
                    pid,
                    am::pubrec_reason_code::success
                };
                auto pubcomp = am::v5::pubcomp_packet{
                    pid,
                    am::pubcomp_reason_code::success
                };

                auto pubres = co_await(
                    cl->async_publish(
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(pubrec, as::use_awaitable)
                    &&
                    cl->next_layer().emulate_recv(pubcomp, as::use_awaitable)
                );
                BOOST_CHECK(!pubres.puback_opt);
                BOOST_CHECK(pubres.pubrec_opt);
                BOOST_CHECK(pubres.pubcomp_opt);
                BOOST_TEST(*pubres.pubrec_opt == pubrec);
                BOOST_TEST(*pubres.pubcomp_opt == pubcomp);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_publish_qos2_success_error_pubrec) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto pubrec = am::v5::pubrec_packet{
                    pid,
                    am::pubrec_reason_code::not_authorized
                };

                auto pubres = co_await(
                    cl->async_publish(
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(pubrec, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::pubrec_reason_code::not_authorized);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_publish_qos2_success_error_pubcomp) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                auto pid = *cl->acquire_unique_packet_id();
                auto pubrec = am::v5::pubrec_packet{
                    pid,
                    am::pubrec_reason_code::success
                };
                auto pubcomp = am::v5::pubcomp_packet{
                    pid,
                    am::pubcomp_reason_code::packet_identifier_not_found
                };

                auto pubres = co_await(
                    cl->async_publish(
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once,
                        as::use_awaitable
                    )
                    &&
                    cl->next_layer().emulate_recv(pubrec, as::use_awaitable)
                    &&
                    cl->next_layer().emulate_recv(pubcomp, as::use_awaitable)
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::pubcomp_reason_code::packet_identifier_not_found);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_recv_auth_disconnect_fast) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                // setup
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test case
                auto auth = am::v5::auth_packet{};
                auto disconnect = am::v5::disconnect_packet{};
                co_await cl->next_layer().emulate_recv(auth, as::use_awaitable);
                co_await cl->next_layer().emulate_recv(disconnect, as::use_awaitable);
                {
                    auto pv = co_await cl->async_recv(as::use_awaitable);
                    BOOST_TEST(pv == auth);
                }
                {
                    auto pv = co_await cl->async_recv(as::use_awaitable);
                    BOOST_TEST(pv == disconnect);
                }

                // tear down
                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_recv_disconnect_last) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                // setup
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test case
                auto disconnect = am::v5::disconnect_packet{};
                auto pv = co_await (
                    cl->next_layer().emulate_recv(disconnect, as::use_awaitable) &&
                    cl->async_recv(as::use_awaitable)
                );
                BOOST_TEST(pv == disconnect);

                // tear down
                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_publish_send_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                cl->next_layer().set_send_error_code(
                    make_error_code(
                        am::errc::connection_reset
                    )
                );
                auto [ec, pubres] = co_await
                    cl->async_publish(
                        "topic1",
                        "payload1",
                        am::qos::at_most_once,
                        as::as_tuple(as::use_awaitable)
                    );
                BOOST_TEST(ec == am::errc::connection_reset);

                BOOST_CHECK(!pubres.puback_opt);
                BOOST_CHECK(!pubres.pubrec_opt);
                BOOST_CHECK(!pubres.pubcomp_opt);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_connect_send_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                cl->next_layer().set_send_error_code(
                    make_error_code(
                        am::errc::connection_reset
                    )
                );
                co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::errc::connection_reset);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_subscribe_send_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                cl->next_layer().set_send_error_code(
                    make_error_code(
                        am::errc::connection_reset
                    )
                );

                auto pid = *cl->acquire_unique_packet_id();
                std::vector<am::topic_subopts> sub_entry{
                    {"topic1", am::qos::exactly_once},
                };

                auto [ec, suback_opt] = co_await
                    cl->async_subscribe(
                        pid,
                        sub_entry,
                        as::as_tuple(as::use_awaitable)
                    );
                BOOST_TEST(ec == am::errc::connection_reset);
                BOOST_CHECK(!suback_opt);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_unsubscribe_send_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                // test scenario
                cl->next_layer().set_send_error_code(
                    make_error_code(
                        am::errc::connection_reset
                    )
                );

                auto pid = *cl->acquire_unique_packet_id();
                std::vector<am::topic_sharename> unsub_entry{
                    {"topic1"},
                };

                auto [ec, unsuback_opt] = co_await
                    cl->async_unsubscribe(
                        pid,
                        unsub_entry,
                        as::as_tuple(as::use_awaitable)
                    );
                BOOST_TEST(ec == am::errc::connection_reset);
                BOOST_CHECK(!unsuback_opt);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_auth_success) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                co_await cl->async_auth(
                    as::use_awaitable
                );

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5_auth_error) {
    static constexpr am::protocol_version version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_start
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::use_awaitable
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                auto props = am::properties{
                    am::property::will_delay_interval{1}
                };

                co_await cl->async_auth(
                    am::auth_reason_code::success,
                    props,
                    as::use_awaitable
                );
                BOOST_TEST(false);
            }
            catch (am::system_error const& se) {
                BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
            }
            co_await cl->next_layer().emulate_close(as::use_awaitable);
            co_await cl->async_close(as::use_awaitable);
            co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            co_return;
        },
        as::detached
    );
    ioc.run();
}

#if !defined(_MSC_VER)

BOOST_AUTO_TEST_CASE(v311_start_success_promise) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::experimental::use_promise
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

#endif // !defined(_MSC_VER)

BOOST_AUTO_TEST_CASE(v311_start_success_deferred) {
    static constexpr am::protocol_version version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto cl = am::client<version, am::cpp20coro_stub_socket>::create(
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            try {
                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await cl->next_layer().emulate_recv(connack, as::use_awaitable);

                auto connack_opt = co_await cl->async_start(
                    true,             // clean_session
                    std::uint16_t(0), // keep_alive
                    "cid1",
                    as::deferred
                );
                BOOST_CHECK(connack_opt);
                BOOST_TEST(*connack_opt == connack);

                co_await cl->next_layer().emulate_close(as::use_awaitable);
                co_await cl->async_close(as::use_awaitable);
                co_await cl->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            catch (am::system_error const&) {
                BOOST_TEST(false);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_SUITE_END()
