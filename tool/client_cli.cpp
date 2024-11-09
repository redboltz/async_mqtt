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

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // defined(__GNUC__)

#include <cli/boostasioscheduler.h>
#include <cli/cli.h>
#include <cli/clilocalsession.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // defined(__GNUC__)

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

namespace as = boost::asio;
namespace po = boost::program_options;
namespace am = async_mqtt;

constexpr auto color_red  = "\033[31m";
constexpr auto color_recv = "\033[36m";
constexpr auto color_none = "\033[0m";

#include <boost/asio/yield.hpp>

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

        root->Insert(
            "pub",
            {"topic", "payload", "qos[0-2]"},
            [this](std::ostream& out, std::string topic, std::string payload, std::size_t qos) {
                if (qos > 2) {
                    out << "Invalid QoS:" << qos << std::endl;
                    return;
                }
                publish(
                    topic,
                    payload,
                    static_cast<am::qos>(qos)
                );
            },
            "publish"
        );

        root->Insert(
            "sub",
            {"topic_filter", "qos[0-2]"},
            [this](std::ostream& out, std::string topic, std::size_t qos) {
                if (qos > 2) {
                    out << "Invalid QoS:" << qos << std::endl;
                    return;
                }
                subscribe(
                    topic,
                    static_cast<am::qos>(qos)
                );
            },
            "subscribe"
        );

        root->Insert(
            "unsub",
            {"topic_filter"},
            [this](std::ostream& /*out*/, std::string topic) {
                unsubscribe(
                    topic
                );
            },
            "unsubscribe"
        );

        // ---------------------------------------------------------------------
        auto pub_menu = std::make_unique<cli::Menu>(
            "bpub", "build publish packet and send ..."
        );
        pub_menu->Insert(
            "topic",
            {"TopicName"},
            [this](std::ostream& o, std::string topic) {
                pub_topic_ = topic;
                print_pub(o);
            }
        );
        pub_menu->Insert(
            "payload",
            {"Payload"},
            [this](std::ostream& o, std::string payload) {
                pub_payload_ = payload;
                print_pub(o);
            }
        );
        pub_menu->Insert(
            "retain",
            {"[0|1]"},
            [this](std::ostream& o, std::string retain_str) {
                if (retain_str == "1" ||
                    retain_str == "yes" ||
                    retain_str == "true"
                ) {
                    pub_retain_ = am::pub::retain::yes;
                }
                else if (retain_str == "0" ||
                    retain_str == "no" ||
                    retain_str == "false"
                ) {
                    pub_retain_ = am::pub::retain::no;
                }
                print_pub(o);
            }
        );
        pub_menu->Insert(
            "qos",
            {"[0-2]"},
            [this](std::ostream& o, std::string qos_str) {
                if (qos_str == "0" ||
                    qos_str == "at_most_once"
                ) {
                    pub_qos_ = am::qos::at_most_once;
                }
                else if (qos_str == "1" ||
                    qos_str == "at_least_once"
                ) {
                    pub_qos_ = am::qos::at_least_once;
                }
                else if (qos_str == "2" ||
                    qos_str == "exactly_once"
                ) {
                    pub_qos_ = am::qos::exactly_once;
                }
                print_pub(o);
            }
        );
        pub_menu->Insert(
            "pfi",
            {"[0|1]"},
            [this](std::ostream& o, std::string str) {
                auto val = boost::lexical_cast<std::uint32_t>(str);
                switch (val) {
                case 0:
                    pub_pfi_.emplace(am::payload_format::binary);
                    break;
                case 1:
                    pub_pfi_.emplace(am::payload_format::string);
                    break;
                default:
                    o << "Invalid value\n";
                    break;
                }
                print_pub(o);
            },
            "Payload Format Idenfitier Property (0:Binary, 1:String)"
        );
        pub_menu->Insert(
            "mei",
            {"[0-4294967295]"},
            [this](std::ostream& o, std::string str) {
                auto val = boost::lexical_cast<std::uint32_t>(str);
                pub_mei_.emplace(val);
                print_pub(o);
            },
            "Message Expiry Interval Property (32bit seconds)"
        );
        pub_menu->Insert(
            "ct",
            {"ContentTypeSrting"},
            [this](std::ostream& o, std::string str) {
                pub_ct_.emplace(str);
                print_pub(o);
            },
            "Content Type Property (ContentTypeString)"
        );
        pub_menu->Insert(
            "rt",
            {"topic"},
            [this](std::ostream& o, std::string str) {
                pub_rt_.emplace(str);
                print_pub(o);
            },
            "Response Topic Property (topic)"
        );
        pub_menu->Insert(
            "cd",
            {"data(only string supported on this tool)"},
            [this](std::ostream& o, std::string str) {
                pub_cd_.emplace(str);
                print_pub(o);
            },
            "Correlation Data Property (data binary)"
        );
        pub_menu->Insert(
            "ta",
            {"[1-65535]"},
            [this](std::ostream& o, std::string str) {
                auto val = boost::lexical_cast<std::uint16_t>(str);
                pub_ta_.emplace(val);
                print_pub(o);
            },
            "Topic Alias Property (16bit TopicAlias value)"
        );
        pub_menu->Insert(
            "add_up",
            {"key", "val"},
            [this](std::ostream& o, std::string key, std::string val) {
                pub_ups_.emplace_back(key, val);
                print_pub(o);
            },
            "Subscription Identifier Property"
        );
        pub_menu->Insert(
            "show",
            [this](std::ostream& o) {
                print_pub(o);
            },
            "show building packet"
        );
        pub_menu->Insert(
            "clear",
            [this](std::ostream& o) {
                pub_payload_ = std::string{};
                pub_topic_ = std::string{};
                pub_qos_ = am::qos::at_most_once;
                pub_retain_ = am::pub::retain::no;
                pub_pfi_ = std::nullopt;
                pub_mei_ = std::nullopt;
                pub_ct_ = std::nullopt;
                pub_rt_ = std::nullopt;
                pub_cd_ = std::nullopt;
                pub_ta_ = std::nullopt;
                pub_ups_.clear();
                print_pub(o);
            },
            "clear packet"
        );
        pub_menu->Insert(
            "send",
            [this](std::ostream& /*out*/) {
                publish(
                    pub_topic_,
                    pub_payload_,
                    pub_qos_ | pub_retain_
                );
            },
            "send packet"
        );
        root->Insert(am::force_move(pub_menu));

        // ---------------------------------------------------------------------
        auto sub_menu = std::make_unique<cli::Menu>(
            "bsub", "build publish packet and send ..."
        );
        sub_menu->Insert(
            "topic",
            {"TopicFilter"},
            [this](std::ostream& o, std::string topic) {
                sub_topic_ = topic,
                print_sub(o);
            }
        );
        sub_menu->Insert(
            "qos",
            {"[0-2]"},
            [this](std::ostream& o, std::string qos_str) {
                if (qos_str == "0" ||
                    qos_str == "at_most_once"
                ) {
                    sub_qos_ = am::qos::at_most_once;
                }
                else if (qos_str == "1" ||
                    qos_str == "at_least_once"
                ) {
                    sub_qos_ = am::qos::at_least_once;
                }
                else if (qos_str == "2" ||
                    qos_str == "exactly_once"
                ) {
                    sub_qos_ = am::qos::exactly_once;
                }
                print_sub(o);
            }
        );
        sub_menu->Insert(
            "nl",
            {"[0|1]"},
            [this](std::ostream& o, std::string nl_str) {
                if (nl_str == "0" ||
                    nl_str == "false"
                ) {
                    sub_nl_ = am::sub::nl::no;
                }
                else if (nl_str == "1" ||
                    nl_str == "true"
                ) {
                    sub_nl_ = am::sub::nl::yes;
                }
                print_sub(o);
            },
            "No Local"
        );
        sub_menu->Insert(
            "rap",
            {"[0|1]"},
            [this](std::ostream& o, std::string rap_str) {
                if (rap_str == "0" ||
                    rap_str == "false"
                ) {
                    sub_rap_ = am::sub::rap::dont;
                }
                else if (rap_str == "1" ||
                    rap_str == "true"
                ) {
                    sub_rap_ = am::sub::rap::retain;
                }
                print_sub(o);
            },
            "Retain as Published"
        );
        sub_menu->Insert(
            "rh",
            {"[0(send) | 1(new sub only) | 2(not send)]"},
            [this](std::ostream& o, std::string rh_str) {
                if (rh_str == "0") {
                    sub_rh_ = am::sub::retain_handling::send;
                }
                else if (rh_str == "1") {
                    sub_rh_ = am::sub::retain_handling::send_only_new_subscription;
                }
                else if (rh_str == "2") {
                    sub_rh_ = am::sub::retain_handling::not_send;
                }
                print_sub(o);
            },
            "Retain Handling"
        );
        sub_menu->Insert(
            "sid",
            {"[1-268435455] or 0 (clear)"},
            [this](std::ostream& o, std::string sid_str) {
                auto sid = boost::lexical_cast<std::uint32_t>(sid_str);
                if (sid == 0) {
                    sub_sid_ = std::nullopt;
                }
                else {
                    sub_sid_ = sid;
                }
                print_sub(o);
            },
            "Subscription Identifier Property"
        );
        sub_menu->Insert(
            "add_up",
            {"key", "val"},
            [this](std::ostream& o, std::string key, std::string val) {
                sub_ups_.emplace_back(key, val);
                print_sub(o);
            },
            "Subscription Identifier Property"
        );
        sub_menu->Insert(
            "show",
            [this](std::ostream& o) {
                print_sub(o);
            },
            "show building packet"
        );
        sub_menu->Insert(
            "clear",
            [this](std::ostream& o) {
                sub_topic_ = std::string{};
                sub_qos_ = am::qos::at_most_once;
                sub_nl_ = am::sub::nl::no;
                sub_rap_ = am::sub::rap::dont;
                sub_rh_ = am::sub::retain_handling::send;
                sub_sid_ = std::nullopt;
                sub_ups_.clear();
                print_sub(o);
            },
            "clear packet"
        );
        sub_menu->Insert(
            "send",
            [this](std::ostream& /*out*/) {
                subscribe(
                    sub_topic_,
                    sub_qos_ | sub_nl_ | sub_rap_ | sub_rh_
                );
            },
            "send packet"
        );
        root->Insert(am::force_move(sub_menu));

        cli_ = std::make_unique<cli::Cli>(std::move(root));
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
        o << "qos     : " << pub_qos_ << std::endl;
        o << "retain  : " << pub_retain_ << std::endl;
        o << "props   : " << std::endl;
        if (pub_pfi_) {
            o << "  Payload Format Indicator : " <<
                [this] {
                    if (*pub_pfi_ == am::payload_format::binary) return "binary";
                    return "string";
                } ()
              << std::endl;
        }
        if (pub_mei_) {
            o << "  Message Expiry Interval : " << *pub_mei_ << std::endl;
        }
        if (pub_ct_) {
            o << "  Content Type            : " << *pub_ct_ << std::endl;
        }
        if (pub_rt_) {
            o << "  Response Topic          : " << *pub_rt_ << std::endl;
        }
        if (pub_cd_) {
            o << "  Correlation Data        : " << *pub_cd_ << std::endl;
        }
        if (pub_ta_) {
            o << "  Topic Alias             : " << *pub_ta_ << std::endl;
        }
        for (auto const& p : pub_ups_) {
            o << "  User Property           : " << p.key() << ":" << p.val() << std::endl;
        }
    }

    void print_sub(std::ostream& o) const {
        o << "topic : " << sub_topic_ << std::endl;
        o << "qos   : " << sub_qos_ << std::endl;
        o << "nl    : " << sub_nl_ << std::endl;
        o << "rap   : " << sub_rap_ << std::endl;
        o << "rh    : " << sub_rh_ << std::endl;
        o << "props   : " << std::endl;
        if (sub_sid_) {
            o << "  Subscription Identifier : " << *sub_sid_ << std::endl;
        }
        for (auto const& p : sub_ups_) {
            o << "  User Property           : " << p.key() << ":" << p.val() << std::endl;
        }
    }

    void publish(
        am::packet_id_type pid,
        std::string topic,
        std::string payload,
        am::pub::opts opts
    ) {
        if (version_ == am::protocol_version::v3_1_1) {
            ep_.async_send(
                am::v3_1_1::publish_packet{
                    pid,
                    am::force_move(topic),
                    am::force_move(payload),
                    opts
                },
                [](am::error_code const& ec) {
                    std::cout << color_red << "\n";
                    std::cout << ec.message() << std::endl;
                    std::cout << color_none;
                }
            );
        }
        else {
            am::properties props;
            if (pub_pfi_) {
                props.push_back(
                    am::property::payload_format_indicator{*pub_pfi_}
                );
            }
            if (pub_mei_) {
                props.push_back(
                    am::property::message_expiry_interval{*pub_mei_}
                );
            }
            if (pub_ct_) {
                props.push_back(
                    am::property::content_type{*pub_ct_}
                );
            }
            if (pub_rt_) {
                props.push_back(
                    am::property::response_topic{*pub_rt_}
                );
            }
            if (pub_cd_) {
                props.push_back(
                    am::property::correlation_data{*pub_cd_}
                );
            }
            if (pub_ta_) {
                props.push_back(
                    am::property::topic_alias{*pub_ta_}
                );
            }
            for (auto& p : pub_ups_) {
                props.emplace_back(p);
            }
            ep_.async_send(
                am::v5::publish_packet{
                    pid,
                    am::force_move(topic),
                    am::force_move(payload),
                    opts,
                    am::force_move(props)
                },
                [](am::error_code const& ec) {
                    std::cout << color_red << "\n";
                    std::cout << ec.message() << std::endl;
                    std::cout << color_none;
                }
            );
        }
    }

    void publish(
        std::string topic,
        std::string payload,
        am::pub::opts opts
    ) {
        if (opts.get_qos() == am::qos::at_least_once ||
            opts.get_qos() == am::qos::exactly_once) {
            ep_.async_acquire_unique_packet_id(
                [
                    this,
                    topic = am::force_move(topic),
                    payload = am::force_move(payload),
                    opts
                ]
                (am::error_code const& ec, am::packet_id_type pid) mutable {
                    if (ec) {
                        std::cout << color_red;
                        std::cout << ec.message() << std::endl;
                        std::cout << color_none;
                    }
                    else {
                        publish(
                            pid,
                            am::force_move(topic),
                            am::force_move(payload),
                            opts
                        );
                    }
                }
            );
        }
        else {
            publish(
                0,
                am::force_move(topic),
                am::force_move(payload),
                opts
            );
        }
    }

    void subscribe(
        std::string topic,
        am::sub::opts opts
    ) {
        ep_.async_acquire_unique_packet_id(
            [
                this,
                topic = am::force_move(topic),
                opts
            ]
            (am::error_code const& ec, am::packet_id_type pid) mutable {
                if (ec) {
                    std::cout << color_red;
                    std::cout << ec.message() << std::endl;
                    std::cout << color_none;
                }
                else {
                    if (version_ == am::protocol_version::v3_1_1) {
                        ep_.async_send(
                            am::v3_1_1::subscribe_packet{
                                pid,
                                { {am::force_move(topic), opts} }
                            },
                            [](am::error_code const& ec) {
                                std::cout << color_red << "\n";
                                std::cout << ec.message() << std::endl;
                                std::cout << color_none;
                            }
                        );
                    }
                    else {
                        am::properties props;
                        if (sub_sid_) {
                            props.push_back(
                                am::property::subscription_identifier{*sub_sid_}
                            );
                        }
                        for (auto& p : sub_ups_) {
                            props.emplace_back(p);
                        }
                        ep_.async_send(
                            am::v5::subscribe_packet{
                                pid,
                                { {am::force_move(topic), opts} },
                                force_move(props)
                            },
                            [](am::error_code const& ec) {
                                std::cout << color_red << "\n";
                                std::cout << ec.message() << std::endl;
                                std::cout << color_none;
                            }
                        );
                    }
                }
            }
        );
    }

    void unsubscribe(
        std::string topic
    ) {
        ep_.async_acquire_unique_packet_id(
            [
                this,
                topic = am::force_move(topic)
            ]
            (am::error_code const& ec, am::packet_id_type pid) mutable {
                if (ec) {
                    std::cout << color_red;
                    std::cout << ec.message() << std::endl;
                    std::cout << color_none;
                }
                else {
                    if (version_ == am::protocol_version::v3_1_1) {
                        ep_.async_send(
                            am::v3_1_1::unsubscribe_packet{
                                pid,
                                { am::force_move(topic) }
                            },
                            [](am::error_code const& ec) {
                                std::cout << color_red << "\n";
                                std::cout << ec.message() << std::endl;
                                std::cout << color_none;
                            }
                        );
                    }
                    else {
                        ep_.async_send(
                            am::v5::unsubscribe_packet{
                                pid,
                                { am::force_move(topic) }
                            },
                            [](am::error_code const& ec) {
                                std::cout << color_red << "\n";
                                std::cout << ec.message() << std::endl;
                                std::cout << color_none;
                            }
                        );
                    }
                }
            }
        );
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
    am::qos pub_qos_ = am::qos::at_most_once;
    am::pub::retain pub_retain_ = am::pub::retain::no;
    std::optional<am::payload_format> pub_pfi_;
    std::optional<std::uint32_t> pub_mei_;
    std::optional<std::string> pub_ct_;
    std::optional<std::string> pub_rt_;
    std::optional<std::string> pub_cd_;
    std::optional<std::uint16_t> pub_ta_;
    std::vector<am::property::user_property> pub_ups_;

    std::string sub_topic_;
    am::qos sub_qos_ = am::qos::at_most_once;
    am::sub::nl sub_nl_ = am::sub::nl::no;
    am::sub::rap sub_rap_ = am::sub::rap::dont;
    am::sub::retain_handling sub_rh_ = am::sub::retain_handling::send;
    std::optional<std::uint32_t> sub_sid_;
    std::vector<am::property::user_property> sub_ups_;

};

template <typename Endpoint>
class network_manager {
public:
    network_manager(
        Endpoint& ep,
        po::variables_map& vm,
        am::protocol_version version
    )
        :ep_{ep},
         vm_{vm},
         version_{version}
    {
        ep_.set_auto_pub_response(true);
    }

    // forwarding callbacks
    void operator()(am::error_code const& ec = am::error_code{}, am::packet_variant pv = am::packet_variant{}) {
        proc(ec, am::force_move(pv));
    }

private:
    void proc(
        am::error_code const& ec,
        am::packet_variant pv
    ) {
        reenter (coro_) {
            yield {
                username_ =
                    [&] () -> std::optional<std::string> {
                        if (vm_.count("username")) {
                            return vm_["username"].template as<std::string>();
                        }
                        return std::nullopt;
                    } ();
                password_ =
                    [&] () -> std::optional<std::string> {
                        if (vm_.count("password")) {
                            return vm_["password"].template as<std::string>();
                        }
                        return std::nullopt;
                    } ();
                client_id_ =
                    [&] () -> std::string {
                        if (vm_.count("client_id")) {
                            return vm_["client_id"].template as<std::string>();
                        }
                        return std::string();
                    } ();
                keep_alive_ = vm_["keep_alive"].template as<std::uint16_t>();
                ws_path_ = vm_["ws_path"].template as<std::string>();
                host_ = vm_["host"].template as<std::string>();
                auto port = vm_["port"].template as<std::uint16_t>();
                auto protocol = vm_["protocol"].template as<std::string>();
                clean_start_ = vm_["clean_start"].template as<bool>();
                sei_ = vm_["sei"].template as<std::uint32_t>();

                // Handshake underlying layer
                ep_.async_underlying_handshake(
                    host_,
                    boost::lexical_cast<std::string>(port),
                    *this
                );
            }
            std::cout << "underlying handshake:" << ec.message() << std::endl;
            if (ec) return;

            // Send MQTT CONNECT
            yield {
                if (version_ == am::protocol_version::v3_1_1) {
                    ep_.async_send(
                        am::v3_1_1::connect_packet{
                            clean_start_,
                            keep_alive_,
                            client_id_,
                            std::nullopt, // will
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
                    ep_.async_send(
                        am::v5::connect_packet{
                            clean_start_,
                            keep_alive_,
                            client_id_,
                            std::nullopt, // will
                            username_,
                            password_,
                            am::force_move(props)
                        },
                        *this
                    );
                }
            }
            if (ec) {
                std::cout << "MQTT CONNECT send error:" << ec.message() << std::endl;
                return;
            }

            // Recv loop
            while (true) {
                yield ep_.async_recv(*this);
                if (ec) {
                    std::cout << color_recv;
                    std::cout
                        << "recv error:"
                        << ec.message()
                        << std::endl;
                    std::cout << color_none;
                    return;
                }
                pv.visit(
                    am::overload {
                        [&](am::v3_1_1::publish_packet const& p) {
                            std::cout << color_recv;
                            std::cout << p << std::endl;
                            std::cout << "  payload:";
                            std::cout << am::json_like_out(p.payload());
                            std::cout << std::endl;
                            std::cout << color_none;
                        },
                        [&](am::v5::publish_packet const& p) {
                            std::cout << color_recv;
                            std::cout << p << std::endl;
                            std::cout << "  payload:";
                            std::cout << am::json_like_out(p.payload());
                            std::cout << std::endl;
                            std::cout << color_none;
                        },
                        [](auto const& p) {
                            std::cout << color_recv;
                            std::cout << p << std::endl;
                            std::cout << color_none;
                        },
                        [](std::monostate&) {
                        }
                    }
                );
            }
        }
    }

private:
    Endpoint& ep_;
    po::variables_map& vm_;
    am::protocol_version version_;
    std::string client_id_;
    std::optional<std::string> username_;
    std::optional<std::string> password_;
    std::uint16_t keep_alive_ = 0;
    bool clean_start_ = true;
    std::uint32_t sei_ = 0;
    std::string host_;
    std::string ws_path_;
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
                 "client_id"
            )
            (
                 "keep_alive",
                 po::value<std::uint16_t>()->default_value(0),
                 "keep_alive"
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
                boost::program_options::value<std::string>()->default_value("/"),
                "Web-Socket path for ws and wss connections"
            )
            (
                "verbose",
                boost::program_options::value<unsigned int>()->default_value(1),
                "set verbose level, possible values:\n 0 - Fatal\n 1 - Error\n 2 - Warning\n 3 - Info\n 4 - Debug\n 5 - Trace"
            )
            (
                "colored_log",
                boost::program_options::value<bool>()->default_value(true),
                "if set true, log output is colored corresponding to the severity, otherwise not colored"
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

        std::cout << "type 'help' to show menu" << std::endl;
#if defined(ASYNC_MQTT_USE_LOG)
        switch (vm["verbose"].as<unsigned int>()) {
        case 5:
            am::setup_log(am::severity_level::trace, vm["colored_log"].as<bool>());
            break;
        case 4:
            am::setup_log(am::severity_level::debug, vm["colored_log"].as<bool>());
            break;
        case 3:
            am::setup_log(am::severity_level::info, vm["colored_log"].as<bool>());
            break;
        case 2:
            am::setup_log(am::severity_level::warning, vm["colored_log"].as<bool>());
            break;
        default:
            am::setup_log(am::severity_level::error, vm["colored_log"].as<bool>());
            break;
        case 0:
            am::setup_log(am::severity_level::fatal, vm["colored_log"].as<bool>());
            break;
        }
#endif // defined(ASYNC_MQTT_USE_LOG)

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
            auto amep = am::endpoint<am::role::client, am::protocol::mqtt>{
                version,
                ioc.get_executor()
            };
            auto cc = client_cli{ioc, amep, version};
            auto nm = network_manager{amep, vm, version};
            nm();
            ioc.run();
            return 0;
        }
#if defined(ASYNC_MQTT_USE_TLS)
        else if (protocol == "mqtts") {
            as::ssl::context ctx{as::ssl::context::tlsv12};
            if (vm.count("verify_file")) {
                ctx.load_verify_file(vm["verify_file"].as<std::string>());
                ctx.set_verify_mode(as::ssl::verify_peer);
            }
            else {
                ctx.set_verify_mode(as::ssl::verify_none);
            }
            if (vm.count("certificate") || vm.count("private_key")) {
                if (!vm.count("certificate")) {
                    std::cout << "private_key is set but certificate is not set" << std::endl;
                    return -1;
                }
                if (!vm.count("private_key")) {
                    std::cout << "certificateis set but private_key is not set" << std::endl;
                    return -1;
                }
                ctx.use_certificate_chain_file(vm["certificate"].as<std::string>());
                ctx.use_private_key_file(vm["private_key"].as<std::string>(), boost::asio::ssl::context::pem);
            }
            auto amep = am::endpoint<am::role::client, am::protocol::mqtts>{
                version,
                ioc.get_executor(),
                ctx
            };
            auto cc = client_cli{ioc, amep, version};
            auto nm = network_manager{amep, vm, version};
            nm();
            ioc.run();
            return 0;
        }
#endif // defined(ASYNC_MQTT_USE_TLS)
#if defined(ASYNC_MQTT_USE_WS)
        else if (protocol == "ws") {
            auto amep = am::endpoint<am::role::client, am::protocol::ws>{
                version,
                ioc.get_executor()
            };
            auto cc = client_cli{ioc, amep, version};
            auto nm = network_manager{amep, vm, version};
            nm();
            ioc.run();
            return 0;
        }
#endif // defined(ASYNC_MQTT_USE_WS)
#if defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)
        else if (protocol == "wss") {
            as::ssl::context ctx{as::ssl::context::tlsv12};
            if (vm.count("verify_file")) {
                ctx.load_verify_file(vm["verify_file"].as<std::string>());
                ctx.set_verify_mode(as::ssl::verify_peer);
            }
            else {
                ctx.set_verify_mode(as::ssl::verify_none);
            }
            if (vm.count("certificate") || vm.count("private_key")) {
                if (!vm.count("certificate")) {
                    std::cout << "private_key is set but certificate is not set" << std::endl;
                    return -1;
                }
                if (!vm.count("private_key")) {
                    std::cout << "certificateis set but private_key is not set" << std::endl;
                    return -1;
                }
                ctx.use_certificate_chain_file(vm["certificate"].as<std::string>());
                ctx.use_private_key_file(vm["private_key"].as<std::string>(), boost::asio::ssl::context::pem);
            }
            auto amep = am::endpoint<am::role::client, am::protocol::wss>{
                version,
                ioc.get_executor(),
                ctx
            };
            auto cc = client_cli{ioc, amep, version};
            auto nm = network_manager{amep, vm, version};
            nm();
            ioc.run();
            return 0;
        }
#endif // defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)
    }
    catch (std::exception const &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
