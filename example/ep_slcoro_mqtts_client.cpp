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
    ):res_{exe},
      host_{std::move(host)},
      port_{std::move(port)},
      amep_{am::protocol_version::v3_1_1, exe, ctx_}
    {
        ctx_.set_verify_mode(am::tls::verify_none);
        impl_();
    }

private:
    struct impl {
        impl(app& a):app_{a}
        {
        }
        // forwarding callbacks
        void operator()() const {
            proc({}, {}, {}, {});
        }
        void operator()(boost::system::error_code const& ec) const {
            proc(ec, {}, {}, {});
        }
        void operator()(boost::system::error_code ec, as::ip::tcp::resolver::results_type eps) const {
            proc(ec, {}, {}, std::move(eps));
        }
        void operator()(boost::system::error_code ec, as::ip::tcp::endpoint /*unused*/) const {
            proc(ec, {}, {}, {});
        }
        void operator()(am::system_error const& se) const {
            proc({}, se, {}, {});
        }
        void operator()(am::packet_variant pv) const {
            proc({}, {}, am::force_move(pv), {});
        }
    private:
        void proc(
            boost::system::error_code const& ec,
            am::system_error const& se,
            am::packet_variant pv,
            std::optional<as::ip::tcp::resolver::results_type> eps
        ) const {
            reenter (coro_) {
                std::cout << "start" << std::endl;

                // Resolve hostname
                yield app_.res_.async_resolve(app_.host_, app_.port_, *this);
                std::cout << "async_resolve:" << ec.message() << std::endl;
                if (ec) return;

                // Layer
                // am::stream -> TCP

                // Underlying TCP connect
                yield as::async_connect(
                    app_.amep_.lowest_layer(),
                    *eps,
                    *this
                );
                std::cout
                    << "TCP connected ec:"
                    << ec.message()
                    << std::endl;

                if (ec) return;

                // Underlying TLS handshake
                yield app_.amep_.next_layer().async_handshake(
                    am::tls::stream_base::client,
                    *this
                );
                std::cout
                    << "TLS handshaked ec:"
                    << ec.message()
                    << std::endl;

                // Send MQTT CONNECT
                yield app_.amep_.send(
                    am::v3_1_1::connect_packet{
                        true,   // clean_session
                        0x1234, // keep_alive
                        am::allocate_buffer("cid1"),
                        am::nullopt, // will
                        am::nullopt, // username set like am::allocate_buffer("user1"),
                        am::nullopt  // password set like am::allocate_buffer("pass1")
                    },
                    *this
                );
                if (se) {
                    std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
                    return;
                }

                // Recv MQTT CONNACK
                yield app_.amep_.recv(*this);
                if (pv) {
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
                }
                else {
                    std::cout
                        << "MQTT CONNACK recv error:"
                        << pv.get<am::system_error>().what()
                        << std::endl;
                    return;
                }

                // Send MQTT SUBSCRIBE
                yield app_.amep_.send(
                    am::v3_1_1::subscribe_packet{
                        *app_.amep_.acquire_unique_packet_id(),
                        { {am::allocate_buffer("topic1"), am::qos::at_most_once} }
                    },
                    *this
                );
                if (se) {
                    std::cout << "MQTT SUBSCRIBE send error:" << se.what() << std::endl;
                    return;
                }
                // Recv MQTT SUBACK
                yield app_.amep_.recv(*this);
                if (pv) {
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
                }
                else {
                    std::cout
                        << "MQTT SUBACK recv error:"
                        << pv.get<am::system_error>().what()
                        << std::endl;
                    return;
                }
                // Send MQTT PUBLISH
                yield app_.amep_.send(
                    am::v3_1_1::publish_packet{
                        *app_.amep_.acquire_unique_packet_id(),
                        am::allocate_buffer("topic1"),
                        am::allocate_buffer("payload1"),
                        am::qos::at_least_once
                    },
                    *this
                );
                if (se) {
                    std::cout << "MQTT PUBLISH send error:" << se.what() << std::endl;
                    return;
                }
                // Recv MQTT PUBLISH and PUBACK (order depends on broker)
                for (app_.count_ = 0; app_.count_ != 2; ++app_.count_) {
                    yield app_.amep_.recv(*this);
                    if (pv) {
                        pv.visit(
                            am::overload {
                                [&](am::v3_1_1::publish_packet const& p) {
                                    std::cout
                                        << "MQTT PUBLISH recv"
                                        << " pid:" << p.packet_id()
                                        << " topic:" << p.topic()
                                        << " payload:" << am::to_string(p.payload())
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
                    else {
                        std::cout
                            << "MQTT recv error:"
                            << pv.get<am::system_error>().what()
                            << std::endl;
                        return;
                    }
                }
                std::cout << "close" << std::endl;
                yield app_.amep_.close(*this);
            }
        }

    private:
        app& app_;
        mutable as::coroutine coro_;
    };

    as::ip::tcp::resolver res_;
    std::string_view host_;
    std::string_view port_;
    am::tls::context ctx_{am::tls::context::tlsv12};
    am::endpoint<am::role::client, am::protocol::mqtts> amep_;
    std::size_t count_ = 0;
    impl impl_{*this};
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
