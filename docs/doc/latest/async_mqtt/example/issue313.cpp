#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

void test() {
    as::io_context ioc;
    auto exe = ioc.get_executor();
    // auto amcl = am::client<am::protocol_version::v3_1_1, am::protocol::mqtt>(exe); // Works
    as::co_spawn(
        exe,
        [&] () -> as::awaitable<void> {
            auto amcl = am::client<am::protocol_version::v3_1_1, am::protocol::mqtt>::create(exe); // heap-use-after-free

            auto [ec_und] = co_await amcl.async_underlying_handshake(
                "127.0.0.1",
                "1883",
                as::as_tuple(as::use_awaitable)
            );
            (void)ec_und;
            auto [ec_con, connack_opt] = co_await amcl->async_start(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0,      // keep_alive
                    ""
                },
                as::as_tuple(as::use_awaitable)
            );

            auto [ec_disconnect] = co_await amcl->async_disconnect(
                am::v3_1_1::disconnect_packet{},
                as::as_tuple(as::use_awaitable)
            );
            (void)ec_disconnect;

            co_await amcl->async_close(
                as::as_tuple(as::use_awaitable)
            );

            co_return;
        },
        as::detached
    );
    ioc.run();
}

int main() {
    am::setup_log(
        am::severity_level::trace,
        true // log colored
    );
    test();
}
