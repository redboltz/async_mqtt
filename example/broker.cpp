// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <thread>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/asio/signal_set.hpp>

#include <async_mqtt/predefined_underlying_layer.hpp>
#include <async_mqtt/broker/endpoint_variant.hpp>
#include <async_mqtt/broker/broker.hpp>
#include <async_mqtt/broker/constant.hpp>
#include <async_mqtt/setup_log.hpp>

namespace am = async_mqtt;
namespace as = boost::asio;

#if defined(ASYNC_MQTT_USE_TLS)

inline
bool verify_certificate(
    std::string const& verify_field,
    bool preverified,
    as::ssl::verify_context& ctx,
    std::shared_ptr<am::optional<std::string>> const& username) {
    if (!preverified) return false;
    int error = X509_STORE_CTX_get_error(ctx.native_handle());
    if (error != X509_V_OK) {
        int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());
        ASYNC_MQTT_LOG("mqtt_broker", error)
            << "Certificate validation failed, depth: " << depth
            << ", message: " << X509_verify_cert_error_string(error);
        return false;
    }

    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME* name = X509_get_subject_name(cert);

    std::string verify_field_value;
    auto obj = std::unique_ptr<
        ASN1_OBJECT,
        decltype(&ASN1_OBJECT_free)
    >(
        OBJ_txt2obj(verify_field.c_str(), 0),
        &ASN1_OBJECT_free
    );
    if (obj) { // return nullptr if error
        verify_field_value.resize(am::max_cn_size);
        auto size = X509_NAME_get_text_by_OBJ(
            name,
            obj.get(),
            &verify_field_value[0],
            static_cast<int>(verify_field_value.size())
        );
        // Size equals -1 if field is not found, otherwise, length of value
        verify_field_value.resize(static_cast<std::size_t>(std::max(size, 0)));
        ASYNC_MQTT_LOG("mqtt_broker", info) << "[clicrt] " << verify_field << ":" << verify_field_value;
        *username = verify_field_value;
    }
    return true;
}

inline
as::ssl::context init_ctx() {
    as::ssl::context ctx(as::ssl::context::tlsv12);
    ctx.set_options(
        as::ssl::context::default_workarounds |
        as::ssl::context::single_dh_use);
    return ctx;
}

template<typename Context>
void reload_ctx(Context& ctx,
                as::steady_timer& reload_timer,
                std::string const& certificate_filename,
                std::string const& key_filename,
                am::optional<std::string> const& verify_file,
                unsigned int certificate_reload_interval,
                char const* name,
                bool first_load = true) {
    ASYNC_MQTT_LOG("mqtt_broker", info) << "Reloading certificates for server " << name;

    if (certificate_reload_interval > 0) {
        reload_timer.expires_after(std::chrono::hours(certificate_reload_interval));
        reload_timer.async_wait(
            [
                &ctx,
                &reload_timer,
                certificate_filename,
                key_filename,
                certificate_reload_interval,
                verify_file,
                name
            ]
            (boost::system::error_code const& e) {
                BOOST_ASSERT(!e || e == as::error::operation_aborted);
                if (!e) {
                    reload_ctx(
                        ctx,
                        reload_timer,
                        certificate_filename,
                        key_filename,
                        verify_file,
                        certificate_reload_interval,
                        name,
                        false
                    );
                }
            }
        );
    }

    auto context = init_ctx();

    boost::system::error_code ec;
    context.use_certificate_chain_file(certificate_filename, ec);
    if (ec) {
        auto message = "Failed to load certificate file: " + ec.message();
        if (first_load) {
            throw std::runtime_error(message);
        }

        ASYNC_MQTT_LOG("mqtt_broker", warning) << message;
        return;
    }

    context.use_private_key_file(key_filename, as::ssl::context::pem, ec);
    if (ec) {
        auto message = "Failed to load private key file: " + ec.message();
        if (first_load) {
            throw std::runtime_error(message);
        }

        ASYNC_MQTT_LOG("mqtt_broker", warning) << message;
        return;
    }

    if (verify_file) {
        context.load_verify_file(*verify_file);
    }

    ctx = am::force_move(context);
}

template<typename Context>
void load_ctx(
    Context& ctx,
    as::steady_timer& reload_timer,
    boost::program_options::variables_map const& vm,
    char const* name
) {
    if (vm.count("certificate") == 0 && vm.count("private_key") == 0) {
        throw std::runtime_error("TLS requested but certificate and/or private_key not specified");
    }

    am::optional<std::string> verify_file;
    if (vm.count("verify_file")) {
        verify_file = vm["verify_file"].as<std::string>();
    }
    reload_ctx(
        ctx,
        reload_timer,
        vm["certificate"].as<std::string>(),
        vm["private_key"].as<std::string>(),
        verify_file,
        vm["certificate_reload_interval"].as<unsigned int>(),
        name, true
    );
}

#endif // defined(ASYNC_MQTT_USE_TLS)

inline
void run_broker(boost::program_options::variables_map const& vm) {
    try {
        as::io_context timer_ioc;

        using epv_t = am::endpoint_variant<
            am::role::server,
            am::protocol::mqtt
#if defined(ASYNC_MQTT_USE_WS)
            ,
            am::protocol::ws
#endif // defined(ASYNC_MQTT_USE_WS)
#if defined(ASYNC_MQTT_USE_TLS)
            ,
            am::protocol::mqtts
#if defined(ASYNC_MQTT_USE_WS)
            ,
            am::protocol::wss
#endif // defined(ASYNC_MQTT_USE_WS)
#endif // defined(ASYNC_MQTT_USE_TLS)
        >;

        am::broker<
            epv_t::shared_type
        > brk{timer_ioc};

        auto num_of_iocs =
            [&] () -> std::size_t {
                if (vm.count("iocs")) {
                    return vm["iocs"].as<std::size_t>();
                }
                return 1;
            } ();
        if (num_of_iocs == 0) {
            num_of_iocs = std::thread::hardware_concurrency();
            ASYNC_MQTT_LOG("mqtt_broker", info)
                << "iocs set to auto decide (0). Automatically set to " << num_of_iocs;
        }

        auto threads_per_ioc =
            [&] () -> std::size_t {
                if (vm.count("threads_per_ioc")) {
                    return vm["threads_per_ioc"].as<std::size_t>();
                }
                return 1;
            } ();
        if (threads_per_ioc == 0) {
            threads_per_ioc = std::min(std::size_t(std::thread::hardware_concurrency()), std::size_t(4));
            ASYNC_MQTT_LOG("mqtt_broker", info)
                << "threads_per_ioc set to auto decide (0). Automatically set to "
                << threads_per_ioc;
        }

        ASYNC_MQTT_LOG("mqtt_broker", info)
            << "iocs:" << num_of_iocs
            << " threads_per_ioc:" << threads_per_ioc
            << " total threads:" << num_of_iocs * threads_per_ioc;

        if (vm.count("auth_file")) {
            std::string auth_file = vm["auth_file"].as<std::string>();
            if (!auth_file.empty()) {
                ASYNC_MQTT_LOG("mqtt_broker", info)
                    << "auth_file:" << auth_file;

                std::ifstream input(auth_file);

                if (input) {
                    am::security security;
                    security.load_json(input);
                    brk.set_security(am::force_move(security));
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_broker", error)
                        << "Authorization file '"
                        << auth_file
                        << "' not found,  broker doesn't use authorization file.";
                }
            }
        }

        as::io_context accept_ioc;

        std::mutex mtx_con_iocs;
        std::vector<as::io_context> con_iocs(num_of_iocs);
        BOOST_ASSERT(!con_iocs.empty());

        std::vector<
            as::executor_work_guard<
                as::io_context::executor_type
            >
        > guard_con_iocs;
        guard_con_iocs.reserve(con_iocs.size());
        for (auto& con_ioc : con_iocs) {
            guard_con_iocs.emplace_back(con_ioc.get_executor());
        }

        auto con_iocs_it = con_iocs.begin();

        auto con_ioc_getter =
            [&mtx_con_iocs, &con_iocs, &con_iocs_it]() -> as::io_context& {
                std::lock_guard<std::mutex> g{mtx_con_iocs};
                auto& ret = *con_iocs_it++;
                if (con_iocs_it == con_iocs.end()) con_iocs_it = con_iocs.begin();
                return ret;
            };

        // mqtt (MQTT on TCP)
        am::optional<as::ip::tcp::endpoint> mqtt_endpoint;
        am::optional<as::ip::tcp::acceptor> mqtt_ac;
        std::function<void()> mqtt_async_accept;
        if (vm.count("tcp.port")) {
            mqtt_endpoint.emplace(as::ip::tcp::v4(), vm["tcp.port"].as<std::uint16_t>());
            mqtt_ac.emplace(accept_ioc, *mqtt_endpoint);
            mqtt_async_accept =
                [&] {
                    auto epsp =
                        epv_t::make_shared<am::endpoint<am::role::server, am::protocol::mqtt>>(
                            am::protocol_version::undetermined,
                            am::protocol::mqtt{con_ioc_getter().get_executor()}
                        );

                    mqtt_ac->async_accept(
                        epsp->as<am::protocol::mqtt>().get_stream().lowest_layer(),
                        [&mqtt_async_accept, &brk, epsp]
                        (boost::system::error_code const& ec) mutable {
                            if (ec) return;
                            brk.handle_accept(epsp);
                            mqtt_async_accept();
                        }
                    );
                };

            mqtt_async_accept();
        }

#if defined(ASYNC_MQTT_USE_WS)
        // ws (MQTT on WebSocket)
        am::optional<as::ip::tcp::endpoint> ws_endpoint;
        am::optional<as::ip::tcp::acceptor> ws_ac;
        std::function<void()> ws_async_accept;
        if (vm.count("ws.port")) {
            ws_endpoint.emplace(as::ip::tcp::v4(), vm["tcp.port"].as<std::uint16_t>());
            ws_ac.emplace(accept_ioc, *ws_endpoint);
            ws_async_accept =
                [&] {
                    auto epsp =
                        epv_t::make_shared<am::endpoint<am::role::server, am::protocol::ws>>(
                            am::protocol_version::undetermined,
                            am::protocol::ws{con_ioc_getter().get_executor()}
                        );
                    ws_ac->async_accept(
                        epsp->as<am::protocol::ws>().get_stream().lowest_layer(),
                        [&ws_async_accept, &brk, epsp]
                        (boost::system::error_code const& ec) mutable {
                            if (ec) {
                                ws_async_accept();
                                return;
                            }
                            epsp->as<am::protocol::ws>().get_stream().next_layer().async_accept(
                                [&ws_async_accept, &brk, epsp]
                                (boost::system::error_code const& ec) mutable {
                                    if (ec) return;
                                    brk.handle_accept(force_move(epsp));
                                    ws_async_accept();
                                }
                            );
                        }
                    );
                };

            ws_async_accept();
        }

#endif // defined(ASYNC_MQTT_USE_WS)

#if defined(ASYNC_MQTT_USE_TLS)
        // mqtts (MQTT on TLS TCP
        am::optional<as::ip::tcp::endpoint> mqtts_endpoint;
        am::optional<as::ip::tcp::acceptor> mqtts_ac;
        std::function<void()> mqtts_async_accept;
        am::optional<as::steady_timer> mqtts_timer;
        auto ctx = init_ctx();
        mqtts_timer.emplace(accept_ioc);
        auto verify_field_obj =
            std::unique_ptr<ASN1_OBJECT, decltype(&ASN1_OBJECT_free)>(
                OBJ_txt2obj(vm["verify_field"].as<std::string>().c_str(), 0),
                &ASN1_OBJECT_free
            );
        if (!verify_field_obj) {
            throw std::runtime_error(
                "An invalid verify field was specified: " +
                vm["verify_field"].as<std::string>()
            );
        }
        load_ctx(ctx, mqtts_timer.value(), vm, "TLS");
        if (vm.count("tls.port")) {
            mqtts_endpoint.emplace(as::ip::tcp::v4(), vm["tls.port"].as<std::uint16_t>());
            mqtts_ac.emplace(accept_ioc, *mqtts_endpoint);
            mqtts_async_accept =
                [&] {
                    auto epsp =
                        epv_t::make_shared<am::endpoint<am::role::server, am::protocol::mqtts>>(
                            am::protocol_version::undetermined,
                            am::protocol::mqtts{con_ioc_getter().get_executor(), ctx}
                        );

                    mqtts_ac->async_accept(
                        epsp->as<am::protocol::mqtts>().get_stream().lowest_layer(),
                        [&mqtts_async_accept, &brk, epsp, &ctx, &vm]
                        (boost::system::error_code const& ec) mutable {
                            if (ec) return;
                            // TBD insert underlying timeout here
                            auto username = std::make_shared<am::optional<std::string>>(); // shared_ptr for username
                            ctx.set_verify_mode(as::ssl::verify_peer);
                            ctx.set_verify_callback(
                                [username, &vm] // copy capture socket shared_ptr
                                (bool preverified, boost::asio::ssl::verify_context& ctx) {
                                    // user can set username in the callback
                                    return
                                        verify_certificate(
                                            vm["verify_field"].as<std::string>(),
                                            preverified,
                                            ctx,
                                            username
                                        );
                                }
                            );
                            epsp->as<am::protocol::mqtts>().get_stream().next_layer().async_handshake(
                                as::ssl::stream_base::server,
                                [&brk, epsp]
                                (boost::system::error_code const& ec) {
                                    if (ec) return;
                                    brk.handle_accept(epsp);
                                }
                            );
                            mqtts_async_accept();
                        }
                    );
                };

            mqtts_async_accept();
        }

#if defined(ASYNC_MQTT_USE_WS)
        // wss (MQTT on WebScoket TLS TCP
#endif // defined(ASYNC_MQTT_USE_WS)
#endif // defined(ASYNC_MQTT_USE_TLS)

        std::thread th_accept {
            [&accept_ioc] {
                accept_ioc.run();
                ASYNC_MQTT_LOG("mqtt_broker", trace) << "accept_ioc.run() finished";
            }
        };

        as::executor_work_guard<
            as::io_context::executor_type
        > guard_timer_ioc(timer_ioc.get_executor());

        std::thread th_timer {
            [&timer_ioc] {
                timer_ioc.run();
                ASYNC_MQTT_LOG("mqtt_broker", trace) << "timer_ioc.run() finished";
            }
        };
        std::vector<std::thread> ts;
        ts.reserve(num_of_iocs * threads_per_ioc);
        for (auto& con_ioc : con_iocs) {
            for (std::size_t i = 0; i != threads_per_ioc; ++i) {
                ts.emplace_back(
                    [&con_ioc] {
                        con_ioc.run();
                        ASYNC_MQTT_LOG("mqtt_broker", trace) << "con_ioc.run() finished";
                    }
                );
            }
        }

        as::io_context ioc_signal;
        as::signal_set signals{ioc_signal, SIGINT, SIGTERM};
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
        std::thread th_signal {
            [&] {
                ioc_signal.run();
            }
        };

        th_accept.join();
        ASYNC_MQTT_LOG("mqtt_broker", trace) << "th_accept joined";

        for (auto& g : guard_con_iocs) g.reset();
        for (auto& t : ts) t.join();
        ASYNC_MQTT_LOG("mqtt_broker", trace) << "ts joined";

        guard_timer_ioc.reset();
        th_timer.join();
        ASYNC_MQTT_LOG("mqtt_broker", trace) << "th_timer joined";

        signals.cancel();
        th_signal.join();
        ASYNC_MQTT_LOG("mqtt_broker", trace) << "th_signal joined";
    } catch(std::exception &e) {
        ASYNC_MQTT_LOG("mqtt_broker", error) << e.what();
    }
}

int main(int argc, char **argv) {
    try {
        boost::program_options::options_description desc;

        boost::program_options::options_description general_desc("General options");
        general_desc.add_options()
            ("help", "produce help message")
            (
                "cfg",
                boost::program_options::value<std::string>()->default_value("broker.conf"),
                "Load configuration file"
            )
            (
                "iocs",
                boost::program_options::value<std::size_t>()->default_value(1),
                "Number of io_context. If set 0 then automatically decided by hardware_concurrency()."
            )
            (
                "threads_per_ioc",
                boost::program_options::value<std::size_t>()->default_value(1),
                "Number of worker threads for each io_context."
            )
            (
                "verbose",
                boost::program_options::value<unsigned int>()->default_value(1),
                "set verbose level, possible values:\n 0 - Fatal\n 1 - Error\n 2 - Warning\n 3 - Info\n 4 - Debug\n 5 - Trace"
            )
#if defined(ASYNC_MQTT_USE_TLS)
            (
                "certificate",
                boost::program_options::value<std::string>(),
                "Certificate file for TLS connections"
            )
            (
                "private_key",
                boost::program_options::value<std::string>(),
                "Private key file for TLS connections"
            )
            (
                "verify_file",
                boost::program_options::value<std::string>(),
                "Root certificate used for verification of the client"
            )
            (
                "verify_field",
                boost::program_options::value<std::string>()->default_value("CN"),
                "Field to be used from certificate for authenticating clients"
            )
            (
                "certificate_reload_interval",
                boost::program_options::value<unsigned int>()->default_value(0),
                "Reload interval for the certificate and private key files (hours)\n 0 - Disabled"
            )
#endif
            (
                "auth_file",
                boost::program_options::value<std::string>(),
                "Authentication file"
            )
            ;

        boost::program_options::options_description notls_desc("TCP Server options");
        notls_desc.add_options()
            ("tcp.port", boost::program_options::value<std::uint16_t>(), "default port (TCP)")
        ;
        desc.add(general_desc).add(notls_desc);

#if defined(ASYNC_MQTT_USE_WS)
        boost::program_options::options_description ws_desc("TCP websocket Server options");
        ws_desc.add_options()
            ("ws.port", boost::program_options::value<std::uint16_t>(), "default port (TCP)")
        ;

        desc.add(ws_desc);
#endif // defined(ASYNC_MQTT_USE_WS)

#if defined(ASYNC_MQTT_USE_TLS)
        boost::program_options::options_description tls_desc("TLS Server options");
        tls_desc.add_options()
            ("tls.port", boost::program_options::value<std::uint16_t>(), "default port (TLS)")
        ;

        desc.add(tls_desc);
#endif // defined(ASYNC_MQTT_USE_TLS)

#if defined(ASYNC_MQTT_USE_WS) && defined(ASYNC_MQTT_USE_TLS)
        boost::program_options::options_description tlsws_desc("TLS Websocket Server options");
        tlsws_desc.add_options()
            ("wss.port", boost::program_options::value<std::uint16_t>(), "default port (TLS)")
        ;
        desc.add(tlsws_desc);
#endif // defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

        std::string config_file = vm["cfg"].as<std::string>();
        if (!config_file.empty()) {
            std::ifstream input(vm["cfg"].as<std::string>());
            if (input) {
                boost::program_options::store(boost::program_options::parse_config_file(input, desc), vm);
            }
            else {
                std::cerr
                    << "Configuration file '"
                    << config_file
                    << "' not found,  broker doesn't use configuration file."
                    << std::endl;
            }
        }

        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        }

        std::cout << "Set options:" << std::endl;
        for (auto const& e : vm) {
            std::cout << boost::format("%-28s") % e.first.c_str() << " : ";
            if (auto p = boost::any_cast<std::string>(&e.second.value())) {
                std::cout << *p;
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

#if defined(ASYNC_MQTT_USE_LOG)
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

        run_broker(vm);
    }
    catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
