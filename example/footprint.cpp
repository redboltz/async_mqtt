// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

int main() {

    am::setup_log(am::severity_level::warning);
    as::io_context ioc;
    auto amcl = am::client<am::protocol_version::v5, am::protocol::mqtt>::create(ioc.get_executor());
    as::co_spawn(
        amcl->get_executor(),
        [&] () -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;

            // Resolve hostname
            auto [ec_und] = co_await am::async_underlying_handshake(
                amcl->next_layer(),
                "127.0.0.1",
                "1883",
                as::as_tuple(as::deferred)
            );
            if (ec_und) co_return;

            // MQTT connect and receive loop start
            auto [ec_con, connack_opt] = co_await amcl->async_start(
                true,   // clean_session
                std::uint16_t(0x1234), // keep_alive
                "cid1",
                std::nullopt, // will
                std::nullopt, // username set like allocate_buffer("user1"),
                std::nullopt, // password set like allocate_buffer("pass1")
                as::as_tuple(as::use_awaitable)
            );

            if (ec_con) co_return;
            if (!connack_opt) co_return;
            if (connack_opt->code() != am::connect_reason_code::success) co_return;

            // subscribe
            // MQTT send subscribe and wait suback
            std::vector<am::topic_subopts> sub_entry{
                {
                    "topic1",
                    am::qos::exactly_once |
                    am::sub::rap::retain |
                    am::sub::nl::no |
                    am::sub::retain_handling::send
                },
            };
            auto [ec_sub, suback_opt] = co_await amcl->async_subscribe(
                *amcl->acquire_unique_packet_id(), // sync version only in strand
                am::force_move(sub_entry), // sub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            if (ec_sub) co_return;
            if (!suback_opt) co_return;

            // publish
            // MQTT publish QoS0 and wait response (socket write complete)
            auto [ec_pub0, pubres0] = co_await amcl->async_publish(
                "topic1",
                "payload1",
                am::qos::at_most_once,
                as::as_tuple(as::use_awaitable)
            );
            if (ec_pub0) co_return;

            auto [ec_recv, pv] = co_await amcl->async_recv(
                as::as_tuple(as::deferred)
            );

            // unsubscribe
            auto [ec_unsub, unsuback_opt] = co_await amcl->async_unsubscribe(
                *amcl->acquire_unique_packet_id(), // sync version only in strand
                std::vector<am::topic_sharename>{"topic1"},
                as::as_tuple(as::use_awaitable)
            );
            if (ec_unsub) co_return;
            if (!unsuback_opt) co_return;

            // disconnect
            auto [ec_discon] = co_await amcl->async_disconnect(
                as::as_tuple(as::use_awaitable)
            );
            if (ec_discon) co_return;

            co_return;
        },
        as::detached
    );
    ioc.run();
}
