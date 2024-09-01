// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>

#include <boost/asio.hpp>

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

#include <boost/asio/yield.hpp>

template <typename Executor>
struct app {
    app(Executor exe,
        std::string_view host,
        std::string_view port
    ):host_{std::move(host)},
      port_{std::move(port)},
      amep_{am::protocol_version::v3_1_1, exe}
    {
        impl_();
    }

private:
    struct impl {
        impl(app& a):app_{a}
        {
        }
        // forwarding callbacks
        void operator()() const {
            proc({}, am::packet_variant{});
        }
        void operator()(am::error_code const& ec) const {
            proc(ec, am::packet_variant{});
        }
        void operator()(am::error_code const& ec, am::packet_variant pv) const {
            proc(ec, am::force_move(pv));
        }
    private:
        void proc(
            am::error_code const& ec,
            am::packet_variant pv
        ) const {

            reenter (coro_) {
                std::cout << "start" << std::endl;

                // Handshake undlerying layer (Name resolution and TCP handshaking)
                yield am::async_underlying_handshake(
                    app_.amep_.next_layer(),
                    app_.host_,
                    app_.port_,
                    *this
                );

                std::cout
                    << "Underlying layer connected ec:"
                    << ec.message()
                    << std::endl;

                if (ec) return;

                // Send MQTT CONNECT
                yield app_.amep_.async_send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0x1234, // keep_alive
                        "ClientIdentifier1",
                        app_.will,   // you can pass std::nullopt if you don't want to set the will message
                        "UserName1",
                        "Password1"
                    },
                    *this
                );
                if (ec) {
                    std::cout << "MQTT CONNECT send error:" << ec.message() << std::endl;
                    return;
                }

                // Recv MQTT CONNACK
                yield app_.amep_.async_recv(*this);
                if (ec) {
                    std::cout << "MQTT CONNACK recv error:" << ec.message() << std::endl;
                    return;
                }
                BOOST_ASSERT(pv);
                pv.visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            std::cout
                                << "MQTT CONNACK recv"
                                << " sp:" << p.session_present()
                                << std::endl;
                        },
                        [](auto const&) {}
                    }
                );

                // Send MQTT SUBSCRIBE
                yield app_.amep_.async_send(
                    am::v3_1_1::subscribe_packet{
                        *app_.amep_.acquire_unique_packet_id(), // sync version only works thread safe context
                        { {"topic1", am::qos::at_most_once} }
                    },
                    *this
                );
                if (ec) {
                    std::cout << "MQTT SUBSCRIBE send error:" << ec.message() << std::endl;
                    return;
                }
                // Recv MQTT SUBACK
                yield app_.amep_.async_recv(*this);
                if (ec) {
                    std::cout << "MQTT SUBACK recv error:" << ec.message() << std::endl;
                    return;
                }
                BOOST_ASSERT(pv);
                pv.visit(
                    am::overload {
                        [&](am::v3_1_1::suback_packet const& p) {
                            std::cout
                                << "MQTT SUBACK recv"
                                << " pid:" << p.packet_id()
                                << " entries:";
                            for (auto const& e : p.entries()) {
                                std::cout << e << " ";
                            }
                            std::cout << std::endl;
                        },
                        [](auto const&) {}
                    }
                );

                // Send MQTT PUBLISH
                yield app_.amep_.async_send(
                    am::v3_1_1::publish_packet{
                        *app_.amep_.acquire_unique_packet_id(), // sync version only works thread safe context
                        "topic1",
                        "payload1",
                        am::qos::at_least_once
                    },
                    *this
                );
                if (ec) {
                    std::cout << "MQTT PUBLISH send error:" << ec.message() << std::endl;
                    return;
                }
                // Recv MQTT PUBLISH and PUBACK (order depends on broker)
                for (app_.count_ = 0; app_.count_ != 2; ++app_.count_) {
                    yield app_.amep_.async_recv(*this);
                    if (ec) {
                        std::cout << "MQTT recv error:" << ec.message() << std::endl;
                        return;
                    }
                    BOOST_ASSERT(pv);
                    pv.visit(
                        am::overload {
                            [&](am::v3_1_1::publish_packet const& p) {
                                std::cout
                                    << "MQTT PUBLISH recv"
                                    << " pid:" << p.packet_id()
                                    << " topic:" << p.topic()
                                    << " payload:" << p.payload()
                                    << " qos:" << p.opts().get_qos()
                                    << " retain:" << p.opts().get_retain()
                                    << " dup:" << p.opts().get_dup()
                                    << std::endl;
                            },
                            [&](am::v3_1_1::puback_packet const& p) {
                                std::cout
                                    << "MQTT PUBACK recv"
                                    << " pid:" << p.packet_id()
                                    << std::endl;
                            },
                            [](auto const&) {}
                        }
                    );
                }
                std::cout << "close" << std::endl;
                yield app_.amep_.async_close(*this);
            }
        }

    private:
        app& app_;
        mutable as::coroutine coro_;
    };

    std::string_view host_;
    std::string_view port_;
    am::endpoint<am::role::client, am::protocol::mqtt> amep_;
    std::size_t count_ = 0;
    impl impl_{*this};
    // prepare will message if you need.
    am::will will{
        "WillTopic1",
        "WillMessage1",
        am::qos::at_most_once,
        { // properties
            am::property::user_property{"key1", "val1"},
            am::property::content_type{"text"},
        }
    };
};

#include <boost/asio/unyield.hpp>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    am::setup_log(am::severity_level::trace);
    as::io_context ioc;
    app a{ioc.get_executor(), argv[1], argv[2]};
    ioc.run();
}
