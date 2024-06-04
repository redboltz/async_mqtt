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
#include <stdexcept>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/asio/signal_set.hpp>

#include <async_mqtt/all.hpp>

#if defined(ASYNC_MQTT_USE_TLS)
#include <async_mqtt/predefined_layer/mqtts.hpp>
#endif // defined(ASYNC_MQTT_USE_TLS)

#if defined(ASYNC_MQTT_USE_WS)
#include <async_mqtt/predefined_layer/ws.hpp>
#endif // defined(ASYNC_MQTT_USE_WS)

#if defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)
#include <async_mqtt/predefined_layer/wss.hpp>
#endif // defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)

#include <broker/endpoint_variant.hpp>
#include <broker/broker.hpp>
#include <broker/constant.hpp>
#include <broker/fixed_core_map.hpp>

namespace am = async_mqtt;
namespace as = boost::asio;

#if defined(ASYNC_MQTT_USE_TLS)

inline
bool verify_certificate(
    std::string const& verify_field,
    bool preverified,
    as::ssl::verify_context& ctx,
    std::shared_ptr<std::optional<std::string>> const& username) {
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
std::shared_ptr<as::ssl::context> init_ctx(
    std::string const& certificate_filename,
    std::string const& key_filename,
    std::optional<std::string> const& verify_file
) {
    auto ctx = std::make_shared<as::ssl::context>(as::ssl::context::tlsv12);
    ctx->set_options(
        as::ssl::context::default_workarounds |
        as::ssl::context::single_dh_use);
    boost::system::error_code ec;
    ctx->use_certificate_chain_file(certificate_filename, ec);
    if (ec) {
        auto message = "Failed to load certificate file: " + ec.message();
        ASYNC_MQTT_LOG("mqtt_broker", error) << message;
        throw std::runtime_error(message);
    }

    ctx->use_private_key_file(key_filename, as::ssl::context::pem, ec);
    if (ec) {
        auto message = "Failed to load private key file: " + ec.message();
        ASYNC_MQTT_LOG("mqtt_broker", error) << message;
        throw std::runtime_error(message);
    }

    if (verify_file) {
        ctx->load_verify_file(*verify_file);
    }
    return ctx;
}

#endif // defined(ASYNC_MQTT_USE_TLS)

inline
void run_broker(boost::program_options::variables_map const& vm) {
    try {
        std::optional<as::thread_pool> thread_pool;
        auto threads = vm["threads"].as<std::size_t>();
        if (threads == 0) {
            ASYNC_MQTT_LOG("mqtt_broker", info)
                << "thread set to 0. Automatically set to " << std::thread::hardware_concurrency() << " (number of cores)";
            thread_pool.emplace();
        }
        else {
            thread_pool.emplace(threads);
        }

        using epv_type = am::basic_endpoint_variant<
            am::role::server,
            2,
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
            epv_type
        > brk{thread_pool->get_executor(), vm["recycling_allocator"].as<bool>()};

        auto set_auth =
            [&] {
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
                            ASYNC_MQTT_LOG("mqtt_broker", warning)
                                << "Authorization file '"
                                << auth_file
                                << "' not found,  broker doesn't use authorization file.";
                        }
                    }
                }
            };
        set_auth();

        // mqtt (MQTT on TCP)
        std::optional<as::ip::tcp::endpoint> mqtt_endpoint;
        std::optional<as::ip::tcp::acceptor> mqtt_ac;
        std::function<void()> mqtt_async_accept;
        auto apply_socket_opts =
            [&](auto& lowest_layer) {
                if (vm.count("tcp_no_delay")) {
                    lowest_layer.set_option(as::ip::tcp::no_delay(vm["tcp_no_delay"].as<bool>()));
                }
                if (vm.count("recv_buf_size")) {
                    lowest_layer.set_option(
                        as::socket_base::receive_buffer_size(
                            boost::numeric_cast<int>(vm["recv_buf_size"].as<std::size_t>())
                        )
                    );
                }
                if (vm.count("send_buf_size")) {
                    lowest_layer.set_option(
                        as::socket_base::send_buffer_size(
                            boost::numeric_cast<int>(vm["send_buf_size"].as<std::size_t>())
                        )
                    );
                }
            };

        if (vm.count("tcp.port")) {
            mqtt_endpoint.emplace(as::ip::tcp::v4(), vm["tcp.port"].as<std::uint16_t>());
            mqtt_ac.emplace(thread_pool->get_executor(), *mqtt_endpoint);
            mqtt_async_accept =
                [&] {
                    auto epsp =
                        am::basic_endpoint<
                            am::role::server,
                            2,
                            am::protocol::mqtt
                        >::create(
                            am::protocol_version::undetermined,
                            as::make_strand(thread_pool->get_executor())
                        );
                    epsp->set_bulk_write(vm["bulk_write"].as<bool>());
                    auto& lowest_layer = epsp->lowest_layer();
                    mqtt_ac->async_accept(
                        lowest_layer,
                        [&mqtt_async_accept, &apply_socket_opts, &lowest_layer, &brk, epsp]
                        (boost::system::error_code const& ec) mutable {
                            if (ec) {
                                ASYNC_MQTT_LOG("mqtt_broker", error)
                                    << "TCP accept error:" << ec.message();
                            }
                            else {
                                apply_socket_opts(lowest_layer);
                                brk.handle_accept(epv_type{force_move(epsp)});
                            }
                            mqtt_async_accept();
                        }
                    );
                };

            mqtt_async_accept();
        }

#if defined(ASYNC_MQTT_USE_WS)
        // ws (MQTT on WebSocket)
        std::optional<as::ip::tcp::endpoint> ws_endpoint;
        std::optional<as::ip::tcp::acceptor> ws_ac;
        std::function<void()> ws_async_accept;
        if (vm.count("ws.port")) {
            ws_endpoint.emplace(as::ip::tcp::v4(), vm["ws.port"].as<std::uint16_t>());
            ws_ac.emplace(thread_pool->get_executor(), *ws_endpoint);
            ws_async_accept =
                [&] {
                    auto epsp =
                        am::basic_endpoint<
                            am::role::server,
                            2,
                            am::protocol::ws
                        >::create(
                            am::protocol_version::undetermined,
                            as::make_strand(thread_pool->get_executor())
                        );
                    epsp->set_bulk_write(vm["bulk_write"].as<bool>());
                    auto& lowest_layer = epsp->lowest_layer();
                    ws_ac->async_accept(
                        lowest_layer,
                        [&ws_async_accept, &apply_socket_opts, &lowest_layer, &brk, epsp]
                        (boost::system::error_code const& ec) mutable {
                            if (ec) {
                                ASYNC_MQTT_LOG("mqtt_broker", error)
                                    << "TCP accept error:" << ec.message();
                            }
                            else {
                                apply_socket_opts(lowest_layer);
                                auto& ws_layer = epsp->next_layer();
                                ws_layer.async_accept(
                                    [&brk, epsp]
                                    (boost::system::error_code const& ec) mutable {
                                        if (ec) {
                                            ASYNC_MQTT_LOG("mqtt_broker", error)
                                                << "WS accept error:" << ec.message();
                                        }
                                        else {
                                            brk.handle_accept(epv_type{force_move(epsp)});
                                        }
                                    }
                                );
                            }
                            ws_async_accept();
                        }
                    );
                };

            ws_async_accept();
        }

#endif // defined(ASYNC_MQTT_USE_WS)

#if defined(ASYNC_MQTT_USE_TLS)
        // mqtts (MQTT on TLS TCP)
        std::optional<as::ip::tcp::endpoint> mqtts_endpoint;
        std::optional<as::ip::tcp::acceptor> mqtts_ac;
        std::function<void()> mqtts_async_accept;
        std::optional<as::steady_timer> mqtts_timer;
        mqtts_timer.emplace(thread_pool->get_executor());
        auto mqtts_verify_field_obj =
            std::unique_ptr<ASN1_OBJECT, decltype(&ASN1_OBJECT_free)>(
                OBJ_txt2obj(vm["verify_field"].as<std::string>().c_str(), 0),
                &ASN1_OBJECT_free
            );
        if (!mqtts_verify_field_obj) {
            throw std::runtime_error(
                "An invalid verify field was specified: " +
                vm["verify_field"].as<std::string>()
            );
        }
        if (vm.count("tls.port")) {
            mqtts_endpoint.emplace(as::ip::tcp::v4(), vm["tls.port"].as<std::uint16_t>());
            mqtts_ac.emplace(thread_pool->get_executor(), *mqtts_endpoint);
            mqtts_async_accept =
                [&] {
                    std::optional<std::string> verify_file;
                    if (vm.count("verify_file")) {
                        verify_file = vm["verify_file"].as<std::string>();
                    }
                    auto mqtts_ctx = init_ctx(
                        vm["certificate"].as<std::string>(),
                        vm["private_key"].as<std::string>(),
                        verify_file
                    );
                    // shared_ptr for username
                    auto username = std::make_shared<std::optional<std::string>>();
                    mqtts_ctx->set_verify_mode(as::ssl::verify_peer);
                    mqtts_ctx->set_verify_callback(
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
                    auto epsp =
                        am::basic_endpoint<
                            am::role::server,
                            2,
                            am::protocol::mqtts
                        >::create(
                            am::protocol_version::undetermined,
                            as::make_strand(thread_pool->get_executor()),
                            *mqtts_ctx
                        );
                    epsp->set_bulk_write(vm["bulk_write"].as<bool>());
                    auto& lowest_layer = epsp->lowest_layer();
                    mqtts_ac->async_accept(
                        lowest_layer,
                        [&mqtts_async_accept, &apply_socket_opts, &lowest_layer, &brk, epsp, username, mqtts_ctx]
                        (boost::system::error_code const& ec) mutable {
                            if (ec) {
                                ASYNC_MQTT_LOG("mqtt_broker", error)
                                    << "TCP accept error:" << ec.message();
                            }
                            else {
                                // TBD insert underlying timeout here
                                apply_socket_opts(lowest_layer);
                                epsp->next_layer().async_handshake(
                                    as::ssl::stream_base::server,
                                    [&brk, epsp, username, mqtts_ctx]
                                    (boost::system::error_code const& ec) mutable {
                                        if (ec) {
                                            ASYNC_MQTT_LOG("mqtt_broker", error)
                                                << "TLS handshake error:" << ec.message();
                                        }
                                        else {
                                            brk.handle_accept(epv_type{force_move(epsp)}, *username);
                                        }
                                    }
                                );
                            }
                            mqtts_async_accept();
                        }
                    );
                };

            mqtts_async_accept();
        }

#if defined(ASYNC_MQTT_USE_WS)
        // wss (MQTT on WebScoket TLS TCP)
        std::optional<as::ip::tcp::endpoint> wss_endpoint;
        std::optional<as::ip::tcp::acceptor> wss_ac;
        std::function<void()> wss_async_accept;
        std::optional<as::steady_timer> wss_timer;
        wss_timer.emplace(thread_pool->get_executor());
        auto wss_verify_field_obj =
            std::unique_ptr<ASN1_OBJECT, decltype(&ASN1_OBJECT_free)>(
                OBJ_txt2obj(vm["verify_field"].as<std::string>().c_str(), 0),
                &ASN1_OBJECT_free
            );
        if (!wss_verify_field_obj) {
            throw std::runtime_error(
                "An invalid verify field was specified: " +
                vm["verify_field"].as<std::string>()
            );
        }
        if (vm.count("wss.port")) {
            wss_endpoint.emplace(as::ip::tcp::v4(), vm["wss.port"].as<std::uint16_t>());
            wss_ac.emplace(thread_pool->get_executor(), *wss_endpoint);
            wss_async_accept =
                [&] {
                    std::optional<std::string> verify_file;
                    if (vm.count("verify_file")) {
                        verify_file = vm["verify_file"].as<std::string>();
                    }
                    auto wss_ctx = init_ctx(
                        vm["certificate"].as<std::string>(),
                        vm["private_key"].as<std::string>(),
                        verify_file
                    );
                    // shared_ptr for username
                    auto username = std::make_shared<std::optional<std::string>>();
                    wss_ctx->set_verify_mode(as::ssl::verify_peer);
                    wss_ctx->set_verify_callback(
                        [username, &vm]
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
                    auto epsp =
                        am::basic_endpoint<
                            am::role::server,
                            2,
                            am::protocol::wss
                        >::create(
                            am::protocol_version::undetermined,
                            as::make_strand(thread_pool->get_executor()),
                            *wss_ctx
                        );
                    epsp->set_bulk_write(vm["bulk_write"].as<bool>());
                    auto& lowest_layer = epsp->lowest_layer();
                    wss_ac->async_accept(
                        lowest_layer,
                        [&wss_async_accept, &apply_socket_opts, &lowest_layer, &brk, epsp, username, wss_ctx]
                        (boost::system::error_code const& ec) mutable {
                            if (ec) {
                                ASYNC_MQTT_LOG("mqtt_broker", error)
                                    << "TCP accept error:" << ec.message();
                            }
                            else {
                                // TBD insert underlying timeout here
                                apply_socket_opts(lowest_layer);
                                epsp->next_layer().next_layer().async_handshake(
                                    as::ssl::stream_base::server,
                                    [&brk, epsp, username, wss_ctx]
                                    (boost::system::error_code const& ec) mutable {
                                        if (ec) {
                                            ASYNC_MQTT_LOG("mqtt_broker", error)
                                                << "TLS handshake error:" << ec.message();
                                        }
                                        else {
                                            auto& ws_layer = epsp->next_layer();
                                            ws_layer.binary(true);
                                            ws_layer.async_accept(
                                                [&brk, epsp, username]
                                                (boost::system::error_code const& ec) mutable {
                                                    if (ec) {
                                                        ASYNC_MQTT_LOG("mqtt_broker", error)
                                                            << "WS accept error:" << ec.message();
                                                    }
                                                    else {
                                                        brk.handle_accept(epv_type{force_move(epsp)}, *username);
                                                    }
                                                }
                                            );
                                        }
                                    }
                                );
                            }
                            wss_async_accept();
                        }
                    );
                };

            wss_async_accept();
        }

#endif // defined(ASYNC_MQTT_USE_WS)
#endif // defined(ASYNC_MQTT_USE_TLS)

        as::io_context ioc_signal;
        as::signal_set signals{
            ioc_signal,
            SIGINT,
            SIGTERM
#if !defined(_WIN32)
            ,
            SIGUSR1
#endif // !defined(_WIN32)
        };
        std::function<void(boost::system::error_code const&, int num)> handle_signal
            = [&set_auth, &signals, &handle_signal] (
                boost::system::error_code const& ec,
                int num
            ) {
                if (!ec) {
                    if (num == SIGINT || num == SIGTERM) {
                        ASYNC_MQTT_LOG("mqtt_broker", trace)
                            << "Signal " << num << " received. exit program";
                        exit(-1);
                    }
#if !defined(_WIN32)
                    else if (num == SIGUSR1) {
                        ASYNC_MQTT_LOG("mqtt_broker", trace)
                            << "Signal " << num << " received. Update auth information";
                        set_auth();
                        signals.async_wait(handle_signal);
                    }
#endif // !defined(_WIN32)
                }
              };
        signals.async_wait(handle_signal);
        std::thread th_signal {
            [&] {
                try {
                    ioc_signal.run();
                }
                catch (std::exception const& e) {
                    ASYNC_MQTT_LOG("mqtt_broker", error)
                        << "th_signal exception:" << e.what();
                }
            }
        };

        thread_pool->join();
        ASYNC_MQTT_LOG("mqtt_broker", trace) << "ts joined";

        signals.cancel();
        th_signal.join();
        ASYNC_MQTT_LOG("mqtt_broker", trace) << "th_signal joined";
    }
    catch (std::exception const& e) {
        ASYNC_MQTT_LOG("mqtt_broker", error) << e.what();
    }
}

int main(int argc, char *argv[]) {
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
                "silent",
                boost::program_options::value<bool>()->default_value(false),
                "if true, then set options are not output"
            )
            (
                "threads",
                boost::program_options::value<std::size_t>()->default_value(0),
                "Number of worker threads. 0 means auto detected number of cores"
            )
            (
                "tcp_no_delay",
                boost::program_options::value<bool>()->default_value(true),
                "Set tcp no_delay option for the sockets"
            )
            (
                "recv_buf_size",
                boost::program_options::value<std::size_t>(),
                "Set receive buffer size of the underlying socket"
            )
            (
                "send_buf_size",
                boost::program_options::value<std::size_t>(),
                "Set send buffer size of the underlying socket"
            )
            (
                "bulk_write",
                boost::program_options::value<bool>()->default_value(false),
                "Set bulk write mode for all connections"
            )
            (
                "recycling_allocator",
                boost::program_options::value<bool>()->default_value(false),
                "Use recyclinc allocator"
            )
            (
                "verbose",
                boost::program_options::value<unsigned int>()->default_value(1),
                "set verbose level, possible values:\n 0 - Fatal\n 1 - Error\n 2 - Warning\n 3 - Info\n 4 - Debug\n 5 - Trace"
            )
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

        boost::program_options::options_description ws_desc("TCP websocket Server options");
        ws_desc.add_options()
            ("ws.port", boost::program_options::value<std::uint16_t>(), "default port (TCP)")
        ;

        desc.add(ws_desc);

        boost::program_options::options_description tls_desc("TLS Server options");
        tls_desc.add_options()
            ("tls.port", boost::program_options::value<std::uint16_t>(), "default port (TLS)")
        ;

        desc.add(tls_desc);

        boost::program_options::options_description tlsws_desc("TLS Websocket Server options");
        tlsws_desc.add_options()
            ("wss.port", boost::program_options::value<std::uint16_t>(), "default port (TLS)")
        ;
        desc.add(tlsws_desc);

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
        if (!vm["silent"].as<bool>()) {
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
    catch(std::exception const& e) {
        std::cerr << e.what() << std::endl;
    }
}
