// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>
#include <fstream>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <cli/boostasioscheduler.h>
#include <cli/cli.h>
#include <cli/clilocalsession.h>

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace po = boost::program_options;
namespace am = async_mqtt;

constexpr auto color_red  = "\033[31m";
constexpr auto color_recv = "\033[36m";
constexpr auto color_none = "\033[0m";

#include <boost/asio/yield.hpp>

using packet_id_t = typename am::packet_id_type<2>::type;

template <typename Endpoint>
class client_cli {
public:
    client_cli(
        as::io_context& ioc,
        Endpoint& ep,
        am::protocol_version version
    )
        :sched_{ioc},
         wg_{ioc.get_executor()},
         ep_{ep},
         version_{version}
    {
        auto root = std::make_unique<cli::Menu>("cli", "Top menu");
        cli::SetColor();

        auto pub_menu = std::make_unique<cli::Menu>("pub", "publish detail");
        pub_menu->Insert(
            "topic",
            {"TopicName"},
            [this](std::ostream& o, std::string topic) {
                pub_topic_ = am::force_move(topic);
                print_pub(o);
            },
            "topic"
        );
        pub_menu->Insert(
            "payload",
            {"Payload"},
            [this](std::ostream& o, std::string payload) {
                pub_payload_ = am::force_move(payload);
                print_pub(o);
            },
            "payload"
        );
        pub_menu->Insert(
            "show",
            [this](std::ostream& o) {
                print_pub(o);
            },
            "show current packet"
        );
        pub_menu->Insert(
            "send",
            [this](std::ostream& /*out*/) {
                if (version_ == am::protocol_version::v3_1_1) {
                    ep_.send(
                        am::v3_1_1::publish_packet{
                            am::allocate_buffer(pub_topic_),
                            am::allocate_buffer(pub_payload_),
                            am::qos::at_most_once
                        },
                        [](am::system_error const& se) {
                            std::cout << color_red << "\n";
                            std::cout << se.message() << std::endl;
                            std::cout << color_none;
                        }
                    );
                }
                else {
                    ep_.send(
                        am::v5::publish_packet{
                            am::allocate_buffer(pub_topic_),
                            am::allocate_buffer(pub_payload_),
                            am::qos::at_most_once
                        },
                        [](am::system_error const& se) {
                            std::cout << color_red << "\n";
                            std::cout << se.message() << std::endl;
                            std::cout << color_none;
                        }
                    );
                }
            },
            "send packet"
        );

        root->Insert(
            "spub",
            {"topic", "payload", "qos[0-2]"},
            [this](std::ostream& /*out*/, std::string topic, std::string payload, std::size_t qos) {
                auto send_pub =
                    [this] (std::string const& topic, std::string const& payload, am::qos qos, packet_id_t pid) {
                        if (version_ == am::protocol_version::v3_1_1) {
                            ep_.send(
                                am::v3_1_1::publish_packet{
                                    pid,
                                    am::allocate_buffer(topic),
                                    am::allocate_buffer(payload),
                                    qos
                                },
                                [](am::system_error const& se) {
                                    std::cout << color_red << "\n";
                                    std::cout << se.message() << std::endl;
                                    std::cout << color_none;
                                }
                            );
                        }
                        else {
                            ep_.send(
                                am::v5::publish_packet{
                                    pid,
                                    am::allocate_buffer(topic),
                                    am::allocate_buffer(payload),
                                    qos
                                },
                                [](am::system_error const& se) {
                                    std::cout << color_red << "\n";
                                    std::cout << se.message() << std::endl;
                                    std::cout << color_none;
                                }
                            );
                        }
                    };

                switch (qos) {
                case 0:
                    send_pub(topic, payload, am::qos::at_most_once, 0);
                    break;
                case 1:
                    ep_.acquire_unique_packet_id(
                        [topic, payload, send_pub](am::optional<packet_id_t> pid_opt) {
                            if (pid_opt) {
                                send_pub(topic, payload, am::qos::at_least_once, *pid_opt);
                            }
                            else {
                                std::cout << color_red;
                                std::cout << "packet_id exhausted" << std::endl;
                                std::cout << color_none;
                            }
                        }
                    );
                    break;
                case 2:
                    ep_.acquire_unique_packet_id(
                        [topic, payload, send_pub](am::optional<packet_id_t> pid_opt) {
                            if (pid_opt) {
                                send_pub(topic, payload, am::qos::exactly_once, *pid_opt);
                            }
                            else {
                                std::cout << color_red;
                                std::cout << "packet_id exhausted" << std::endl;
                                std::cout << color_none;
                            }
                        }
                    );
                    break;
                default:
                    std::cout << "invalid qos" << std::endl;
                    break;
                }
            },
            "Simple publish"
        );

        root->Insert(
            "ssub",
            {"topic_filter", "qos[0-2]"},
            [this](std::ostream& /*out*/, std::string topic, std::size_t qos) {
                ep_.acquire_unique_packet_id(
                    [this, topic, qos](am::optional<packet_id_t> pid_opt) {
                        if (pid_opt) {
                            if (version_ == am::protocol_version::v3_1_1) {
                                ep_.send(
                                    am::v3_1_1::subscribe_packet{
                                        *pid_opt,
                                        { {am::allocate_buffer(topic), static_cast<am::qos>(qos)} }
                                    },
                                    [](am::system_error const& se) {
                                        std::cout << color_red << "\n";
                                        std::cout << se.message() << std::endl;
                                        std::cout << color_none;
                                    }
                                );
                            }
                            else {
                                ep_.send(
                                    am::v5::subscribe_packet{
                                        *pid_opt,
                                        { {am::allocate_buffer(topic), static_cast<am::qos>(qos)} }
                                    },
                                    [](am::system_error const& se) {
                                        std::cout << color_red << "\n";
                                        std::cout << se.message() << std::endl;
                                        std::cout << color_none;
                                    }
                                );
                            }
                        }
                        else {
                            std::cout << color_red;
                            std::cout << "packet_id exhausted" << std::endl;
                            std::cout << color_none;
                        }
                    }
                );
            },
            "Simple subscribe"
        );

        root->Insert(am::force_move(pub_menu));

        cli_ = std::make_unique<cli::Cli>(std::move(root));
        // global exit action
        cli_->ExitAction( [](auto& out){ out << "Goodbye and thanks for all the fish.\n"; } );
        // std exception custom handler
        cli_->StdExceptionHandler(
            [](std::ostream& out, const std::string& cmd, const std::exception& e)
            {
                out << "Exception caught in cli handler: "
                    << e.what()
                    << " handling command: "
                    << cmd
                    << ".\n";
            }
        );

        session_ = std::make_unique<cli::CliLocalTerminalSession>(*cli_, sched_, std::cout, 200);
        session_->ExitAction(
            [this](auto& out) // session exit action
            {
                out << "Closing App...\n";
                wg_.reset();
                sched_.Stop();
            }
        );
    }

private:
    void print_pub(std::ostream& o) const {
        o << "topic   : " << pub_topic_ << std::endl;
        o << "payload : " << pub_payload_ << std::endl;
    }

private:
    cli::BoostAsioScheduler sched_;
    as::executor_work_guard<as::io_context::executor_type> wg_;
    Endpoint& ep_;
    am::protocol_version version_;
    std::unique_ptr<cli::Cli> cli_;
    std::unique_ptr<cli::CliLocalTerminalSession> session_;
    std::string pub_topic_;
    std::string pub_payload_;
};

template <typename Endpoint>
class network_manager {
public:
    network_manager(
        Endpoint& ep,
        as::ip::tcp::resolver& res,
        po::variables_map& vm,
        am::protocol_version version
    )
        :ep_{ep},
         res_{res},
         vm_{vm},
         version_{version}
    {
        ep_.set_auto_pub_response(true);
    }

    // forwarding callbacks
    void operator()() {
        proc({}, {}, {}, {}, {});
    }
    void operator()(boost::system::error_code const& ec) {
        proc(ec, {}, {}, {}, {});
    }
    void operator()(boost::system::error_code ec, as::ip::tcp::resolver::results_type eps) {
        proc(ec, {}, {}, {}, std::move(eps));
    }
    void operator()(boost::system::error_code ec, as::ip::tcp::endpoint /*unused*/) {
        proc(ec, {}, {}, {}, {});
    }
    void operator()(am::system_error const& se) {
        proc({}, se, {}, {}, {});
    }
    void operator()(am::optional<packet_id_t> pid_opt) {
        proc({}, {}, pid_opt, {}, {});
    }
    void operator()(am::packet_variant pv) {
        proc({}, {}, {}, am::force_move(pv), {});
    }

private:
    void proc(
        am::error_code const& ec,
        am::system_error const& se,
        am::optional<packet_id_t> /*pid_opt*/,
        am::packet_variant pv,
        std::optional<as::ip::tcp::resolver::results_type> eps
    ) {
        reenter (coro_) {
            yield {
                username_ =
                    [&] () -> am::optional<am::buffer> {
                        if (vm_.count("username")) {
                            return am::allocate_buffer(vm_["username"].template as<std::string>());
                        }
                        return am::nullopt;
                    } ();
                password_ =
                    [&] () -> am::optional<am::buffer> {
                        if (vm_.count("password")) {
                            return am::allocate_buffer(vm_["password"].template as<std::string>());
                        }
                        return am::nullopt;
                    } ();
                auto client_id =
                    [&] () -> am::buffer {
                        if (vm_.count("client_id")) {
                            return am::allocate_buffer(vm_["client_id"].template as<std::string>());
                        }
                        return am::buffer();
                    } ();
                auto verify_file =
                    [&] () -> am::optional<std::string> {
                        if (vm_.count("verify_file")) {
                            return vm_["verify_file"].template as<std::string>();
                        }
                        return am::nullopt;
                    } ();
                auto certificate =
                    [&] () -> am::optional<std::string> {
                        if (vm_.count("certificate")) {
                            return vm_["certificate"].template as<std::string>();
                        }
                        return am::nullopt;
                    } ();
                auto private_key =
                    [&] () -> am::optional<std::string> {
                        if (vm_.count("private_key")) {
                            return vm_["private_key"].template as<std::string>();
                        }
                        return am::nullopt;
                    } ();
                auto ws_path =
                    [&] () -> am::optional<std::string> {
                        if (vm_.count("ws_path")) {
                            return vm_["ws_path"].template as<std::string>();
                        }
                        return am::nullopt;
                    } ();

                auto host = vm_["host"].template as<std::string>();
                auto port = vm_["port"].template as<std::uint16_t>();
                auto protocol = vm_["protocol"].template as<std::string>();
                clean_start_ = vm_["clean_start"].template as<bool>();
                sei_ = vm_["sei"].template as<std::uint32_t>();

                // Resolve hostname
                res_.async_resolve(host, boost::lexical_cast<std::string>(port), *this);
                std::cout << color_red << "\n";
                std::cout << "async_resolve:" << ec.message() << std::endl;
                if (ec) return;
            }
            // Underlying TCP connect
            yield as::async_connect(
                ep_.lowest_layer(),
                *eps,
                *this
            );
            std::cout
                << "TCP connected ec:"
                << ec.message()
                << std::endl;
            if (ec) return;

            // Send MQTT CONNECT
            yield {
                if (version_ == am::protocol_version::v3_1_1) {
                    ep_.send(
                        am::v3_1_1::connect_packet{
                            clean_start_,
                            0, // keep_alive
                            am::allocate_buffer("cid1"),
                            am::nullopt, // will
                            username_,
                            password_
                        },
                        *this
                    );
                }
                else {
                    auto props =
                        [this] () -> am::properties {
                            if (sei_ == 0) {
                                return am::properties{};
                            }
                            else {
                                return am::properties{am::property::session_expiry_interval{sei_}};
                            }
                        } ();
                    ep_.send(
                        am::v5::connect_packet{
                            clean_start_,
                            0, // keep_alive
                            am::allocate_buffer("cid1"),
                            am::nullopt, // will
                            username_,
                            password_,
                            am::force_move(props)
                        },
                        *this
                    );
                }
            }
            if (se) {
                std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
                return;
            }

            // Recv loop
            while (true) {
                yield ep_.recv(*this);
                if (pv) {
                    pv.visit(
                        am::overload {
                            [&](am::v3_1_1::publish_packet const& p) {
                                std::cout << color_recv;
                                std::cout << p << std::endl;
                                std::cout << "  payload:";
                                for (auto const& e : p.payload()) {
                                    std::cout << am::json_like_out(e);
                                }
                                std::cout << std::endl;
                                std::cout << color_none;
                            },
                            [&](am::v5::publish_packet const& p) {
                                std::cout << color_recv;
                                std::cout << p << std::endl;
                                std::cout << "  payload:";
                                for (auto const& e : p.payload()) {
                                    std::cout << am::json_like_out(e);
                                }
                                std::cout << std::endl;
                                std::cout << color_none;
                            },
                            [](auto const& p) {
                                std::cout << color_recv;
                                std::cout << p << std::endl;
                                std::cout << color_none;
                            }
                        }
                    );
                }
                else {
                    std::cout << color_recv;
                    std::cout
                        << "recv error:"
                        << pv.get<am::system_error>().what()
                        << std::endl;
                    std::cout << color_none;
                    return;
                }
            }
        }
    }

private:
    Endpoint& ep_;
    as::ip::tcp::resolver& res_;
    po::variables_map& vm_;
    am::protocol_version version_;
    am::optional<am::buffer> username_;
    am::optional<am::buffer> password_;
    bool clean_start_ = true;
    std::uint32_t sei_ = 0;
    as::coroutine coro_;
};

#include <boost/asio/unyield.hpp>

int main(int argc, char* argv[]) {
    try {
        boost::program_options::options_description desc;

        boost::program_options::options_description general_desc("General options");
        general_desc.add_options()
            ("help", "produce help message")
            (
                "cfg",
                boost::program_options::value<std::string>()->default_value("cli.conf"),
                "Load configuration file"
            )
            (
                "host",
                boost::program_options::value<std::string>(),
                "mqtt broker's hostname to connect"
            )
            (
                "port",
                boost::program_options::value<std::uint16_t>()->default_value(1883),
                "mqtt broker's port to connect"
            )
            (
                "protocol",
                boost::program_options::value<std::string>()->default_value("mqtt"),
                "mqtt mqtts ws wss"
            )
            (
                "mqtt_version",
                boost::program_options::value<std::string>()->default_value("v5"),
                "MQTT version v5 or v3.1.1"
            )
            (
                "clean_start",
                boost::program_options::value<bool>()->default_value(true),
                "set clean_start flag to client"
            )
            (
                "sei",
                boost::program_options::value<std::uint32_t>()->default_value(0),
                "set session expiry interval to client"
            )
            (
                "username",
                boost::program_options::value<std::string>(),
                "username for all clients"
            )
            (
                "password",
                boost::program_options::value<std::string>(),
                "password for all clients"
            )
            (
                 "client_id",
                 po::value<std::string>(),
                 "(optional) client_id"
            )
            (
                "verify_file",
                boost::program_options::value<std::string>(),
                "CA Certificate file to verify server certificate for mqtts and wss connections"
            )
            (
                "certificate",
                boost::program_options::value<std::string>(),
                "Client certificate (chain) file"
            )
            (
                "private_key",
                boost::program_options::value<std::string>(),
                "Client certificate key file"
            )
            (
                "ws_path",
                boost::program_options::value<std::string>(),
                "Web-Socket path for ws and wss connections"
            )
            (
                "verbose",
                boost::program_options::value<unsigned int>()->default_value(1),
                "set verbose level, possible values:\n 0 - Fatal\n 1 - Error\n 2 - Warning\n 3 - Info\n 4 - Debug\n 5 - Trace"
            )
            ;


        desc.add(general_desc);

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

        std::string config_file = vm["cfg"].as<std::string>();
        if (!config_file.empty()) {
            std::ifstream input(vm["cfg"].as<std::string>());
            if (input.good()) {
                boost::program_options::store(boost::program_options::parse_config_file(input, desc), vm);
            } else
            {
                std::cerr << "Configuration file '"
                          << config_file
                          << "' not found, not use configuration file." << std::endl;
            }
        }

        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        }

        std::cout << "Set options:" << std::endl;
        for (auto const& e : vm) {
            std::cout << boost::format("  %-16s") % e.first.c_str() << " : ";
            if (auto p = boost::any_cast<std::string>(&e.second.value())) {
                if (e.first.c_str() == std::string("password")) {
                    std::cout << "********";
                }
                else {
                    std::cout << *p;
                }
            }
            else if (auto p = boost::any_cast<std::size_t>(&e.second.value())) {
                std::cout << *p;
            }
            else if (auto p = boost::any_cast<std::uint32_t>(&e.second.value())) {
                std::cout << *p;
            }
            else if (auto p = boost::any_cast<std::uint16_t>(&e.second.value())) {
                std::cout << *p;
            }
            else if (auto p = boost::any_cast<unsigned int>(&e.second.value())) {
                std::cout << *p;
            }
            else if (auto p = boost::any_cast<bool>(&e.second.value())) {
                std::cout << std::boolalpha << *p;
            }
            std::cout << std::endl;
        }


#if defined(MQTT_USE_LOG)
        switch (vm["verbose"].as<unsigned int>()) {
        case 5:
            am::setup_log(am::severity_level::trace);
            break;
        case 4:
            am::setup_log(am::severity_level::debug);
            break;
        case 3:
            am::setup_log(am::severity_level::info);
            break;
        case 2:
            am::setup_log(am::severity_level::warning);
            break;
        default:
            am::setup_log(am::severity_level::error);
            break;
        case 0:
            am::setup_log(am::severity_level::fatal);
            break;
        }
#else
        am::setup_log();
#endif

        if (!vm.count("host")) {
            std::cerr << "host must be set" << std::endl;
            return -1;
        }

        boost::asio::io_context ioc;
        as::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait(
            [] (
                boost::system::error_code const& ec,
                int num
            ) {
                if (!ec) {
                    ASYNC_MQTT_LOG("mqtt_broker", trace)
                        << "Signal " << num << " received. exit program";
                    exit(-1);
                }
            }
        );

        auto mqtt_version = vm["mqtt_version"].as<std::string>();

        am::protocol_version version =
            [&] {
                if (mqtt_version == "v5" || mqtt_version == "5" || mqtt_version == "v5.0" || mqtt_version == "5.0") {
                    return am::protocol_version::v5;
                }
                else if (mqtt_version == "v3.1.1" || mqtt_version == "3.1.1") {
                    return am::protocol_version::v3_1_1;
                }
                else {
                    std::cerr << "invalid mqtt_version:" << mqtt_version << " it should be v5 or v3.1.1" << std::endl;
                    return am::protocol_version::undetermined;
                }
            } ();

        if (version != am::protocol_version::v5 &&
            version != am::protocol_version::v3_1_1) {
            return -1;
        }


        auto protocol = vm["protocol"].as<std::string>();
        if (protocol == "mqtt") {
            as::ip::tcp::socket resolve_sock{ioc};
            as::ip::tcp::resolver res{resolve_sock.get_executor()};
            am::endpoint<am::role::client, am::protocol::mqtt> amep {
                version,
                am::protocol::mqtt{ioc.get_executor()}
            };
            auto cc = client_cli{ioc, amep, version};
            auto nm = network_manager{amep, res, vm, version};
            nm();
            ioc.run();
            return 0;
        }
    }
    catch (std::exception const &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
