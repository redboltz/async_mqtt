// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <thread>
#include <fstream>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>

#include <async_mqtt/host_port.hpp>
#include <async_mqtt/setup_log.hpp>
#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/packet/pubopts.hpp>
#include <async_mqtt/predefined_underlying_layer.hpp>

#include "locked_cout.hpp"

namespace as = boost::asio;
namespace am = async_mqtt;

static constexpr std::size_t ts_size = 32;

// moved to global to avoid MSVC error
enum class phase {
    connect,
    sub_delay,
    subscribe,
    pub_delay,
    idle,
    pub_after_idle_delay,
    publish
};

enum class mode {
    single,
    send,
    recv
};

using packet_id_t = am::packet_id_type<2>::type;

#include <boost/asio/yield.hpp>
struct bench_context {
    bench_context(
        as::ip::tcp::resolver& res,
        am::optional<std::string> const& ws_path,
        am::protocol_version version,
        std::uint32_t sei,
        bool clean_start,
        am::optional<std::string> const& username,
        am::optional<std::string> const& password,
        std::string const& topic_prefix,
        am::qos qos,
        am::pub::retain retain,
        std::atomic<phase>& ph,
        as::io_context& ioc_timer,
        std::shared_ptr<as::steady_timer>& tim_progress,
        as::executor_work_guard<as::io_context::executor_type>& guard_ioc_timer,
        as::steady_timer& tim_delay,
        as::system_timer& tim_sync,
        std::size_t pub_idle_count,
        std::atomic<std::size_t>& rest_connect,
        std::atomic<std::size_t>& rest_sub,
        std::atomic<std::size_t>& rest_idle,
        std::atomic<std::uint64_t>& rest_times,
        std::size_t con_interval_ms,
        std::size_t sub_delay_ms,
        std::size_t sub_interval_ms,
        std::size_t pub_delay_ms,
        std::size_t pub_after_idle_delay_ms,
        std::size_t pub_interval_ms,
        std::size_t all_interval_ns,
        std::size_t limit_ms,
        bool compare,
        std::chrono::steady_clock::time_point& tp_start,
        std::chrono::steady_clock::time_point& tp_sub_delay,
        std::chrono::steady_clock::time_point& tp_subscribe,
        std::chrono::steady_clock::time_point& tp_pub_delay,
        std::chrono::steady_clock::time_point& tp_idle,
        std::chrono::steady_clock::time_point& tp_pub_after_idle_delay,
        std::chrono::steady_clock::time_point& tp_publish,
        bool detail_report,
        std::vector<
            as::executor_work_guard<
                as::io_context::executor_type
            >
        >& guard_iocs,
        mode md,
        std::vector<std::shared_ptr<as::ip::tcp::socket>> const& workers,
        am::optional<am::host_port> const& manager_hp,
        bool close_after_report
    )
    :res{res},
     ws_path{ws_path},
     version{version},
     sei{sei},
     clean_start{clean_start},
     username{username},
     password{password},
     topic_prefix{topic_prefix},
     qos{qos},
     retain{retain},
     ph{ph},
     ioc_timer{ioc_timer},
     tim_progress{tim_progress},
     guard_ioc_timer{guard_ioc_timer},
     tim_delay{tim_delay},
     tim_sync{tim_sync},
     pub_idle_count{pub_idle_count},
     rest_connect{rest_connect},
     rest_sub{rest_sub},
     rest_idle{rest_idle},
     rest_times{rest_times},
     con_interval_ms{con_interval_ms},
     sub_delay_ms{sub_delay_ms},
     sub_interval_ms{sub_interval_ms},
     pub_delay_ms{pub_delay_ms},
     pub_after_idle_delay_ms{pub_after_idle_delay_ms},
     pub_interval_ms{pub_interval_ms},
     all_interval_ns{all_interval_ns},
     limit_ms{limit_ms},
     compare{compare},
     tp_start{tp_start},
     tp_sub_delay{tp_sub_delay},
     tp_subscribe{tp_subscribe},
     tp_pub_delay{tp_pub_delay},
     tp_idle{tp_idle},
     tp_pub_after_idle_delay{tp_pub_after_idle_delay},
     tp_publish{tp_publish},
     detail_report{detail_report},
     guard_iocs{guard_iocs},
     md{md},
     workers{workers},
     manager_hp{manager_hp},
     close_after_report{close_after_report}
    {
    }

    as::ip::tcp::resolver& res;
    am::optional<std::string> const& ws_path;
    am::protocol_version version;
    std::uint32_t sei;
    bool clean_start;
    am::optional<std::string> const& username;
    am::optional<std::string> const& password;
    std::string const& topic_prefix;
    am::qos qos;
    am::pub::retain retain;
    std::atomic<phase>& ph;
    as::io_context& ioc_timer;
    std::shared_ptr<as::steady_timer>& tim_progress;
    as::executor_work_guard<as::io_context::executor_type>& guard_ioc_timer;
    as::steady_timer& tim_delay;
    as::system_timer& tim_sync;
    std::size_t pub_idle_count;
    std::atomic<std::size_t>& rest_connect;
    std::atomic<std::size_t>& rest_sub;
    std::atomic<std::size_t>& rest_idle;
    std::atomic<std::uint64_t>& rest_times;
    std::size_t con_interval_ms;
    std::size_t sub_delay_ms;
    std::size_t sub_interval_ms;
    std::size_t pub_delay_ms;
    std::size_t pub_after_idle_delay_ms;
    std::size_t pub_interval_ms;
    std::size_t all_interval_ns;
    std::size_t limit_ms;
    bool compare;
    std::chrono::steady_clock::time_point& tp_start;
    std::chrono::steady_clock::time_point& tp_sub_delay;
    std::chrono::steady_clock::time_point& tp_subscribe;
    std::chrono::steady_clock::time_point& tp_pub_delay;
    std::chrono::steady_clock::time_point& tp_idle;
    std::chrono::steady_clock::time_point& tp_pub_after_idle_delay;
    std::chrono::steady_clock::time_point& tp_publish;
    bool detail_report;
    std::vector<
        as::executor_work_guard<
            as::io_context::executor_type
        >
    >& guard_iocs;
    mode md;
    std::vector<std::shared_ptr<as::ip::tcp::socket>> const& workers;
    am::optional<am::host_port> const& manager_hp;
    bool close_after_report;
};

template <typename ClientInfo>
struct bench {
    using ep_t = typename ClientInfo::client_type;
    using next_layer_t = typename ep_t::next_layer_type;
    bench(
        std::vector<ClientInfo>& cis,
        bench_context& bc
    )
        :cis_{cis},
         bc_{bc}
    {
    }

    // forwarding callbacks
    void operator()(ClientInfo* pci = nullptr) {
        proc({}, {}, {}, {}, pci);
    }
    void operator()(boost::system::error_code const& ec, ClientInfo* pci = nullptr) {
        proc(ec, {}, {}, {}, pci);
    }
    void operator()(boost::system::error_code ec, as::ip::tcp::resolver::results_type eps, ClientInfo* pci = nullptr) {
        proc(ec, {}, {}, std::move(eps), pci);
    }
    void operator()(boost::system::error_code ec, as::ip::tcp::endpoint /*unused*/, ClientInfo* pci = nullptr) {
        proc(ec, {}, {}, {}, pci);
    }
    void operator()(am::system_error const& se, ClientInfo* pci = nullptr) {
        proc({}, se, {}, {}, pci);
    }
    void operator()(am::packet_variant pv, ClientInfo* pci = nullptr) {
        proc({}, {}, am::force_move(pv), {}, pci);
    }

private:
    void proc(
        am::optional<boost::system::error_code> ec,
        am::optional<am::system_error> se,
        am::packet_variant pv,
        am::optional<as::ip::tcp::resolver::results_type> eps,
        ClientInfo* pci
    ) {
        reenter (coro_) {
            // Setup
            for (auto& ci : cis_) {
                ci.init_timer(ci.c.strand());
                ci.c.set_auto_pub_response(true);
            }

            yield {
                // connect interval timer set
                std::size_t index = 0;
                for (auto& ci : cis_) {
                    ci.tim->expires_after(std::chrono::milliseconds(bc_.con_interval_ms) * ++index);
                    ci.tim->async_wait(
                        as::append(
                            *this,
                            &ci
                        )
                    );
                }
            }
            if (*ec) {
                locked_cout() << "async_connect interval timer error:" << ec->message() << std::endl;
                exit(-1);
            }

            // Resolve hostname
            yield bc_.res.async_resolve(
                pci->host,
                pci->port,
                as::append(
                    *this,
                    pci
                )
            );
            if (*ec) {
                locked_cout() << "async_resolve error:" << ec->message() << std::endl;
                exit(-1);
            }
            eps_ = am::force_move(*eps);

            // TCP connect
            yield as::async_connect(
                pci->c.lowest_layer(),
                eps_,
                as::append(
                    *this,
                    pci
                )
            );
            if (*ec) {
                locked_cout() << "async_connect error:" << ec->message() << std::endl;
                exit(-1);
            }

            // TLS handshake

#if defined(ASYNC_MQTT_USE_TLS)
            yield {
                if constexpr(std::is_same_v<ep_t, am::endpoint<am::role::client, am::protocol::mqtts>>) {
                    pci->c.next_layer().async_handshake(
                        am::tls::stream_base::client,
                        as::append(
                            *this,
                            pci
                        )
                    );
                    return;
                }
#if defined(ASYNC_MQTT_USE_WS)
                if constexpr(std::is_same_v<ep_t, am::endpoint<am::role::client, am::protocol::wss>>) {
                    pci->c.next_layer().next_layer().async_handshake(
                        am::tls::stream_base::client,
                        as::append(
                            *this,
                            pci
                        )
                    );
                    return;
                }
#endif // defined(ASYNC_MQTT_USE_WS)
                as::dispatch(
                    pci->c.strand(),
                    as::append(
                        *this,
                        am::error_code{},
                        pci
                    )
                );
            }
            if (*ec) {
                locked_cout() << "TLS async_handshake error:" << ec->message() << std::endl;
                exit(-1);
            }
#endif // defined(ASYNC_MQTT_USE_TLS)

            // WS handshake

#if defined(ASYNC_MQTT_USE_WS)
            yield {
                if constexpr(std::is_same_v<ep_t, am::endpoint<am::role::client, am::protocol::ws>>) {
                    pci->c.next_layer().async_handshake(
                        pci->host,
                        [&] () -> std::string{
                            if (bc_.ws_path) return *bc_.ws_path;
                            else return "/";
                        } (),
                        as::append(
                            *this,
                            pci
                        )
                    );
                    return;
                }
#if defined(ASYNC_MQTT_USE_TLS)
                if constexpr(std::is_same_v<ep_t, am::endpoint<am::role::client, am::protocol::wss>>) {
                    pci->c.next_layer().async_handshake(
                        pci->host,
                        [&] () -> std::string{
                            if (bc_.ws_path) return *bc_.ws_path;
                            else return "/";
                        } (),
                        as::append(
                            *this,
                            pci
                        )
                    );
                    return;
                }
#endif // defined(ASYNC_MQTT_USE_TLS)
                as::dispatch(
                    pci->c.strand(),
                    as::append(
                        *this,
                        am::error_code{},
                        pci
                    )
                );
            }
            if (*ec) {
                locked_cout() << "WebSocket async_handshake error:" << ec->message() << std::endl;
                exit(-1);
            }
#endif //defined(ASYNC_MQTT_USE_WS)

            // MQTT connect send
            yield {
                am::optional<am::buffer> un;
                am::optional<am::buffer> pw;
                if (bc_.username) un.emplace(am::allocate_buffer(*bc_.username));
                if (bc_.password) pw.emplace(am::allocate_buffer(*bc_.password));
                switch (bc_.version) {
                case am::protocol_version::v5: {
                    am::properties props;
                    if (bc_.sei != 0) {
                        props.emplace_back(
                            am::property::session_expiry_interval(bc_.sei)
                        );
                    }
                    pci->c.send(
                        am::v5::connect_packet{
                            bc_.clean_start,
                            0, // keep_alive
                            am::allocate_buffer(pci->get_client_id()),
                            am::nullopt, // will
                            am::force_move(un),
                            am::force_move(pw),
                            am::force_move(props)
                        },
                        as::append(
                            *this,
                            pci
                        )
                    );
                } break;
                case am::protocol_version::v3_1_1: {
                    pci->c.send(
                        am::v3_1_1::connect_packet{
                            bc_.clean_start,
                            0, // keep_alive
                            am::allocate_buffer(pci->get_client_id()),
                            am::nullopt, // will
                            am::force_move(un),
                            am::force_move(pw)
                        },
                        as::append(
                            *this,
                            pci
                        )
                    );
                } break;
                default:
                    locked_cout() << "invalid MQTT version" << std::endl;
                    exit(-1);
                }
                pci->init_timer(pci->c.strand());
            }
            if (*se) {
                locked_cout() << "connect send error:" << se->what() << std::endl;
                exit(-1);
            }
            // MQTT connack recv
            yield pci->c.recv(
                as::append(
                    *this,
                    pci
                )
            );
            pv.visit(
                am::overload {
                    [&](am::v5::connack_packet const& p) {
                        if (p.code() == am::connect_reason_code::success) {
                            --bc_.rest_connect;
                        }
                        else {
                            locked_cout() << "v5 connack:" << p.code() << std::endl;
                        }
                    },
                    [&](am::v3_1_1::connack_packet const& p) {
                        if (p.code() == am::connect_return_code::accepted) {
                            --bc_.rest_connect;
                        }
                        else {
                            locked_cout() << "v3.1.1 connack:" << p.code() << std::endl;
                        }
                    },
                    [](auto const&) {
                    }
                }
            );
            if (bc_.rest_connect != 0) return;

            if (bc_.md == mode::single || bc_.md == mode::recv) {
                // subscribe delay
                bc_.ph.store(phase::sub_delay);
                bc_.tp_sub_delay = std::chrono::steady_clock::now();
                bc_.tim_delay.expires_after(std::chrono::milliseconds(bc_.sub_delay_ms));
                yield bc_.tim_delay.async_wait(*this);
                if (*ec) {
                    locked_cout() << "sub delay timer error:" << ec->message() << std::endl;
                    exit(-1);
                }
                bc_.ph.store(phase::subscribe);
                bc_.tp_subscribe = std::chrono::steady_clock::now();
                yield {
                    // sub interval
                    std::size_t index = 0;
                    for (auto& ci : cis_) {
                        ci.tim->expires_after(std::chrono::milliseconds(bc_.sub_interval_ms) * ++index);
                        ci.tim->async_wait(
                            as::append(
                                *this,
                                &ci
                            )
                        );
                    }
                }
                if (*ec) {
                    locked_cout() << "sub interval timer error:" << ec->message() << std::endl;
                    exit(-1);
                }
                yield {
                    switch (bc_.version) {
                    case am::protocol_version::v5: {
                        pci->c.send(
                            am::v5::subscribe_packet{
                                // sync version can be called because the previous timer handler is on the strand
                                *pci->c.acquire_unique_packet_id(),
                                {
                                    { am::allocate_buffer(bc_.topic_prefix + pci->index_str), bc_.qos }
                                },
                                am::properties{}
                            },
                            as::append(
                                *this,
                                pci
                            )
                        );
                    } break;
                    case am::protocol_version::v3_1_1: {
                        pci->c.send(
                            am::v3_1_1::subscribe_packet{
                                // sync version can be called because the previous timer handler is on the strand
                                *pci->c.acquire_unique_packet_id(),
                                {
                                    { am::allocate_buffer(bc_.topic_prefix + pci->index_str), bc_.qos }
                                }
                            },
                            as::append(
                                *this,
                                pci
                            )
                        );
                    } break;
                    default:
                        locked_cout() << "invalid MQTT version" << std::endl;
                        exit(-1);
                    }
                    pci->init_timer(pci->c.strand());
                }
                if (*se) {
                    locked_cout() << "subscribe send error:" << se->what() << std::endl;
                    exit(-1);
                }
                // MQTT suback recv
                yield pci->c.recv(
                    as::append(
                        *this,
                        pci
                    )
                );
                pv.visit(
                    am::overload {
                        [&](am::v5::suback_packet const& p) {
                            if (p.entries().front() == am::suback_reason_code::granted_qos_0 ||
                                p.entries().front() == am::suback_reason_code::granted_qos_1 ||
                                p.entries().front() == am::suback_reason_code::granted_qos_2) {
                                --bc_.rest_sub;
                            }
                            else {
                                locked_cout() << "v5 suback:" << p.entries().front() << std::endl;
                                exit(-1);
                            }
                        },
                        [&](am::v3_1_1::suback_packet const& p) {
                            if (p.entries().front() == am::suback_return_code::success_maximum_qos_0 ||
                                p.entries().front() == am::suback_return_code::success_maximum_qos_1 ||
                                p.entries().front() == am::suback_return_code::success_maximum_qos_2) {
                                --bc_.rest_sub;
                            }
                            else {
                                locked_cout() << "v3.1.1 suback:" << p.entries().front() << std::endl;
                                exit(-1);
                            }
                        },
                        [](auto const&) {
                        }
                    }
                );
                if (bc_.rest_sub != 0) return;
                if (bc_.md == mode::recv) {
                    locked_cout() << "all subscriberd. run `bench --mode send`" << std::endl;
                    bc_.tim_progress->cancel();
                }
            }

            if (bc_.md == mode::single || bc_.md == mode::send) {
                yield {
                    if (bc_.manager_hp) {
                        locked_cout() << "work as worker" << std::endl;
                        as::io_context ioc;
                        as::ip::tcp::resolver r{ioc};
                        auto eps = r.resolve(bc_.manager_hp->host, std::to_string(bc_.manager_hp->port));
                        as::ip::tcp::socket sock{ioc};
                        as::connect(sock, eps);
                        std::string str;
                        str.resize(32); // time since epoch string
                        as::read(sock, as::buffer(str));
                        auto ts_val = boost::lexical_cast<std::uint64_t>(str);
                        locked_cout() << "start time_point:" << ts_val << std::endl;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__APPLE__)
                        auto ts = std::chrono::duration_cast<
                            std::chrono::microseconds
                        >(std::chrono::nanoseconds(ts_val));
#else  // defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__APPLE__)
                        auto ts = std::chrono::nanoseconds(ts_val);
#endif // defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__APPLE__)
                        std::chrono::system_clock::time_point tp(ts);
                        bc_.tim_sync.expires_at(tp);
                        bc_.tim_sync.async_wait(*this);
                    }
                    else if (bc_.workers.size() != 0) {
                        locked_cout() << "work as manager" << std::endl;
                        auto base = std::chrono::system_clock::now() + std::chrono::seconds(10);
                        auto ts_base = static_cast<std::uint64_t>(
                            std::chrono::duration_cast<std::chrono::nanoseconds>(
                                base.time_since_epoch()
                            ).count()
                        );
                        locked_cout() << "start time_point:" << ts_base << std::endl;

                        std::uint64_t worker_delay = bc_.all_interval_ns  / (bc_.workers.size() + 1);
                        for (std::uint64_t i = 0; i != bc_.workers.size(); ++i) {
                            auto ts_val = ts_base + worker_delay * (i + 1);

                            std::string ts_str = (boost::format("%032d") % ts_val).str();
                            as::write(*bc_.workers[i], as::buffer(ts_str));
                        }
                        bc_.tim_sync.expires_at(base);
                        bc_.tim_sync.async_wait(*this);
                    }
                    else {
                        as::dispatch(*this);
                    }
                }
                yield {
                    if (bc_.pub_idle_count == 0) {
                        bc_.ph.store(phase::pub_after_idle_delay);
                        bc_.tp_pub_delay = std::chrono::steady_clock::now();
                        bc_.tim_delay.expires_after(std::chrono::milliseconds(bc_.pub_delay_ms));
                        bc_.tim_delay.async_wait(*this);
                    }
                    else {
                        bc_.ph.store(phase::pub_delay);
                        bc_.tp_pub_delay = std::chrono::steady_clock::now();
                        bc_.tim_delay.expires_after(std::chrono::milliseconds(bc_.pub_delay_ms));
                        bc_.tim_delay.async_wait(*this);
                    }
                }
                if (bc_.ph.load() == phase::pub_delay) {
                    if (*ec) {
                        locked_cout() << "pub idle delay timer error:" << ec->message();
                        exit(-1);
                    }
                    else {
                        bc_.ph.store(phase::idle);
                        bc_.tp_idle = std::chrono::steady_clock::now();
                    }
                }
                else {
                    if (*ec) {
                        locked_cout() << "pub after idle delay timer error:" << ec->message();
                        exit(-1);
                    }
                    else {
                        bc_.ph.store(phase::publish);
                        bc_.tp_publish = std::chrono::steady_clock::now();
                    }
                }
            }
            yield {
                std::size_t index = 0;
                for (auto& ci : cis_) {
                    if (bc_.md == mode::single || bc_.md == mode::send) {
                        // pub interval
                        auto tp =
                            std::chrono::nanoseconds(bc_.all_interval_ns) * index++;
                        ci.tim->expires_after(tp);
                        ci.tim->async_wait(
                            as::append(
                                *this,
                                &ci
                            )
                        );
                    }
                    // pub recv
                    ci.c.recv(
                        as::append(
                            *this,
                            &ci
                        )
                    );
                }
            }

            for (;;) yield {
                auto send_publish =
                    [this, &pci] (packet_id_t pid, am::pub::opts opts) {
                        switch (bc_.version) {
                        case am::protocol_version::v5: {
                            pci->c.send(
                                am::v5::publish_packet{
                                    pid,
                                    am::allocate_buffer(bc_.topic_prefix + pci->index_str),
                                    pci->send_payload(bc_.md),
                                    opts,
                                    am::properties{}
                                },
                                as::append(
                                    *this,
                                    pci
                                )
                            );
                            return;
                        } break;
                        case am::protocol_version::v3_1_1: {
                            pci->c.send(
                                am::v3_1_1::publish_packet{
                                    pid,
                                    am::allocate_buffer(bc_.topic_prefix + pci->index_str),
                                    pci->send_payload(bc_.md),
                                    opts
                                },
                                as::append(
                                    *this,
                                    pci
                                )
                            );
                            return;
                        } break;
                        default:
                            locked_cout() << "invalid MQTT version" << std::endl;
                            exit(-1);
                        }
                    };

                if (ec) {
                    if (bc_.md == mode::single || bc_.md == mode::send) {
                        auto trigger_pub =
                            [&] {
                                packet_id_t pid = 0;
                                if (bc_.qos == am::qos::at_least_once ||
                                    bc_.qos == am::qos::exactly_once) {
                                    // sync version can be called because the previous timer handler is on the strand
                                    pid = *pci->c.acquire_unique_packet_id();
                                }
                                am::pub::opts opts = bc_.qos | bc_.retain;
                                pci->sent.at(pci->send_times - 1) = std::chrono::steady_clock::now();
                                send_publish(pid, opts);
                                BOOST_ASSERT(pci->send_times != 0);
                                --pci->send_times;
                            };


                        switch (bc_.ph.load()) {
                        case phase::idle:
                            // pub interval (idle) timer fired
                            if (*ec) {
                                locked_cout() << "pub interval (idle) timer error:" << ec->message() << std::endl;
                                exit(-1);
                            }
                            trigger_pub();
                            BOOST_ASSERT(pci->send_idle_count != 0);

                            if (bc_.md == mode::send) {
                                BOOST_ASSERT(bc_.rest_times > 0);
                                --bc_.rest_times;
                                if (bc_.rest_idle > 0) {
                                    if (--bc_.rest_idle == 0) {
                                        bc_.ph.store(phase::pub_after_idle_delay);
                                        // use system clock for multi node synchronization
                                        bc_.tp_pub_after_idle_delay = std::chrono::steady_clock::now();
                                        locked_cout() << "Publish (measure) delay" << std::endl;
                                        bc_.tim_delay.expires_after(std::chrono::milliseconds(bc_.pub_after_idle_delay_ms));
                                        bc_.tim_delay.async_wait(*this);
                                    }
                                }
                            }
                            if (--pci->send_idle_count != 0) {
                                pci->tim->expires_at(
                                    pci->tim->expiry() +
                                    std::chrono::milliseconds(bc_.pub_interval_ms)
                                );
                                pci->tim->async_wait(
                                    as::append(
                                        *this,
                                        pci
                                    )
                                );
                            }
                            break;
                        case phase::publish:
                            // pub interval timer fired
                            if (*ec) {
                                locked_cout() << "pub interval timer error:" << ec->message() << std::endl;
                                exit(-1);
                            }
                            trigger_pub();
                            if (pci->send_times != 0) {
                                pci->tim->expires_at(
                                    pci->tim->expiry() +
                                    std::chrono::milliseconds(bc_.pub_interval_ms)
                                );
                                pci->tim->async_wait(
                                    as::append(
                                        *this,
                                        pci
                                    )
                                );
                            }
                            if (bc_.md == mode::send) {
                                BOOST_ASSERT(bc_.rest_times > 0);
                                --bc_.rest_times;
                                if (bc_.rest_times == 0) {
                                    locked_cout() << "all publish finished. after measured Ctrl-C to stop the program" << std::endl;
                                    bc_.tim_progress->cancel();
                                }
                            }
                            break;
                        case phase::pub_after_idle_delay: {
                            bc_.ph.store(phase::publish);
                            bc_.tp_publish = std::chrono::steady_clock::now();
                            locked_cout() << "Publish (measure)" << std::endl;
                            std::size_t index = 0;
                            for (auto& ci : cis_) {
                                // pub interval
                                auto tp =
                                    std::chrono::nanoseconds(bc_.all_interval_ns) * index++;
                                ci.tim->expires_after(tp);
                                ci.tim->async_wait(
                                    as::append(
                                        *this,
                                        &ci
                                    )
                                );
                            }
                        } break;
                        default:
                            BOOST_ASSERT(false);
                            break;
                        }
                    }
                }
                else if (se) {
                    // pub send result
                    if (*se) {
                        locked_cout() << "subscribe send error:" << se->what() << std::endl;
                        exit(-1);
                    }
                }
                else if (pv) {
                    // pub received
                    pub_recv ret = pub_recv::cont;
                    pv.visit(
                        am::overload {
                            [&](am::v5::publish_packet const& p) {
                                ret = recv_publish(
                                    *pci,
                                    p.packet_id(),
                                    p.opts(),
                                    p.topic(),
                                    p.payload(),
                                    p.props()
                                );
                            },
                            [&](am::v3_1_1::publish_packet const& p) {
                                ret = recv_publish(
                                    *pci,
                                    p.packet_id(),
                                    p.opts(),
                                    p.topic(),
                                    p.payload(),
                                    am::properties{}
                                );
                            },
                            [&](auto const&) {
                                // puback, pubrec, pubcomp
                            }
                        }
                    );
                    switch (ret) {
                    case pub_recv::cont:
                        // pub recv
                        break;
                    case pub_recv::idle_finish:
                        if (bc_.md == mode::single) {
                            bc_.ph.store(phase::pub_after_idle_delay);
                            bc_.tp_pub_after_idle_delay = std::chrono::steady_clock::now();
                            locked_cout() << "Publish (measure) delay" << std::endl;
                            bc_.tim_delay.expires_after(std::chrono::milliseconds(bc_.pub_after_idle_delay_ms));
                            bc_.tim_delay.async_wait(*this);
                        }
                        break;
                    case pub_recv::pub_finish: {
                        locked_cout() << "Report" << std::endl;
                        std::size_t maxmax = 0;
                        std::string maxmax_cid;
                        std::size_t maxmid = 0;
                        std::string maxmid_cid;
                        std::size_t maxavg = 0;
                        std::string maxavg_cid;
                        std::size_t maxmin = 0;
                        std::string maxmin_cid;
                        for (auto& ci : cis_) {
                            std::sort(ci.rtt_us.begin(), ci.rtt_us.end());
                            std::string cid = ci.get_client_id();
                            std::size_t max = ci.rtt_us.back();
                            std::size_t mid = ci.rtt_us.at(ci.rtt_us.size() / 2);
                            std::size_t avg = std::accumulate(
                                ci.rtt_us.begin(),
                                ci.rtt_us.end(),
                                std::size_t(0)
                            ) / ci.rtt_us.size();
                            std::size_t min = ci.rtt_us.front();
                            if (maxmax < max) {
                                maxmax = max;
                                maxmax_cid = cid;
                            }
                            if (maxmid < mid) {
                                maxmid = mid;
                                maxmid_cid = cid;
                            }
                            if (maxavg < avg) {
                                maxavg = avg;
                                maxavg_cid = cid;
                            }
                            if (maxmin < min) {
                                maxmin = min;
                                maxmin_cid = cid;
                            }
                            if (bc_.detail_report) {
                                locked_cout()
                                    << cid << " :"
                                    << " max:" << boost::format("%+12d") % max << " us | "
                                    << " mid:" << boost::format("%+12d") % mid << " us | "
                                    << " avg:" << boost::format("%+12d") % avg << " us | "
                                    << " min:" << boost::format("%+12d") % min << " us | "
                                    << std::endl;
                            }
                        }
                        locked_cout()
                            << "maxmax:" << boost::format("%+12d") % maxmax << " us "
                            << "(" << boost::format("%+8d") % (maxmax / 1000) << " ms ) "
                            << "client_id:" << maxmax_cid << std::endl;
                        locked_cout()
                            << "maxmid:" << boost::format("%+12d") % maxmid << " us "
                            << "(" << boost::format("%+8d") % (maxmid / 1000) << " ms ) "
                            << "client_id:" << maxmid_cid << std::endl;
                        locked_cout()
                            << "maxavg:" << boost::format("%+12d") % maxavg << " us "
                            << "(" << boost::format("%+8d") % (maxavg / 1000) << " ms ) "
                            << "client_id:" << maxavg_cid << std::endl;
                        locked_cout()
                            << "maxmin:" << boost::format("%+12d") % maxmin << " us "
                            << "(" << boost::format("%+8d") % (maxmin / 1000) << " ms ) "
                            << "client_id:" << maxmin_cid << std::endl;
                        locked_cout() << "Finish" << std::endl;
                        bc_.tim_progress->cancel();
                        if (bc_.close_after_report) {
                            for (auto& ci : cis_) {
                                ci.c.close([]{});
                            }
                            for (auto& guard_ioc : bc_.guard_iocs) guard_ioc.reset();
                            bc_.guard_ioc_timer.reset();
                        }
                        return;
                    } break;
                    }
                    pci->c.recv(
                        as::append(
                            *this,
                            pci
                        )
                    );
                }
            }
        }
    }

private:

    enum class pub_recv {
        cont,
        idle_finish,
        pub_finish
    };

    pub_recv recv_publish(
        ClientInfo& ci,
        packet_id_t /*packet_id*/,
        am::pub::opts pubopts,
        am::buffer topic_name,
        std::vector<am::buffer> const& payload,
        am::properties /*props*/) {
        if (pubopts.get_retain() == am::pub::retain::yes) {
            locked_cout() << "retained publish received and ignored topic:" << topic_name << std::endl;
            return pub_recv::cont;
        }
        BOOST_ASSERT(bc_.rest_times > 0);

        if (bc_.rest_idle == 0) {
            // actual measure (no idle)
            auto dur_us =
                [&] () -> std::int64_t {
                    if (bc_.md == mode::single) {
                        auto recv = std::chrono::steady_clock::now();
                        return
                            static_cast<std::int64_t>(
                                std::chrono::duration_cast<std::chrono::microseconds>(
                                    recv - ci.sent.at(ci.recv_times - 1)
                                ).count()
                            );
                    }
                    else {
                        auto recv = std::chrono::system_clock::now();
                        BOOST_ASSERT(bc_.md == mode::recv);
                        auto ts = payload.front().substr(8 + 8, ts_size);
                        auto ts_val = boost::lexical_cast<std::int64_t>(ts);
                        return
                            static_cast<std::int64_t>(
                                (
                                    static_cast<std::int64_t>(
                                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            recv.time_since_epoch()
                                        ).count()
                                    )
                                    -
                                    ts_val
                                ) / 1000
                            );
                    }
                } ();
            if (bc_.limit_ms != 0 && static_cast<unsigned long>(dur_us) > bc_.limit_ms * 1000) {
                locked_cout() << "RTT:" << (dur_us / 1000) << "ms over " << bc_.limit_ms << "ms" << std::endl;
            }
            if (bc_.compare && bc_.md == mode::single) {
                if (payload.front() != ci.recv_payload()) {
                    locked_cout() << "received payload doesn't match to sent one" << std::endl;
                    locked_cout() << "  expected: " << ci.recv_payload() << std::endl;
                    locked_cout() << "  received: " << payload.front() << std::endl;;
                }
            }
            if (topic_name != std::string_view(bc_.topic_prefix + ci.index_str)) {
                locked_cout() << "topic doesn't match" << std::endl;
                locked_cout() << "  expected: " << bc_.topic_prefix + ci.index_str << std::endl;
                locked_cout() << "  received: " << topic_name << std::endl;
            }
            ci.rtt_us.emplace_back(dur_us);
            BOOST_ASSERT(ci.recv_times != 0);
            --ci.recv_times;
            if (--bc_.rest_times == 0) {
                return pub_recv::pub_finish;
            }
            else {
                return pub_recv::cont;
            }
        }
        else {
            --ci.recv_times;
            --bc_.rest_times;
            std::size_t expected = 1;
            if (bc_.rest_idle.compare_exchange_strong(expected, 0)) { // bc_.rest_idle: 1 -> 0
                // exact finish idle publishing
                return pub_recv::idle_finish;
            }
            --bc_.rest_idle; // bc_.rest_idle 3 -> 2, 2 -> 1, ...
            // continue idle publishing
            return pub_recv::cont;
        }
    }

private:
    std::vector<ClientInfo>& cis_;
    bench_context& bc_;
    as::ip::tcp::resolver::results_type eps_;
    as::coroutine coro_;
};

#include <boost/asio/unyield.hpp>

int main(int argc, char *argv[]) {
    try {
        boost::program_options::options_description desc;

        constexpr std::size_t min_payload = 15;
        std::string payload_size_desc =
            "payload bytes. must be greater than " + std::to_string(min_payload);

        boost::program_options::options_description general_desc("General options");
        general_desc.add_options()
            ("help", "produce help message")
            (
                "cfg",
                boost::program_options::value<std::string>()->default_value("bench.conf"),
                "Load configuration file"
            )
            (
                "target",
                boost::program_options::value<std::vector<std::string>>(),
                "mqtt broker's hostname:port to connect. when you set this option  multiple times, "
                "then connect round robin for each client. "
                "Note set as --target host1:1883 --target host2:1883, not --target host1:1883 host2:1883."
            )
            (
                "target_index",
                boost::program_options::value<std::size_t>()->default_value(0),
                "start index of the target brokers"
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
                "qos",
                boost::program_options::value<unsigned int>()->default_value(0),
                "QoS 0, 1, or 2"
            )
            (
                "payload_size",
                boost::program_options::value<std::size_t>()->default_value(1024),
                payload_size_desc.c_str()
            )
            (
                "compare",
                boost::program_options::value<bool>()->default_value(false),
                "compare send/receive payloads"
            )
            (
                "retain",
                boost::program_options::value<bool>()->default_value(false),
                "set retain flag to publish"
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
                "start_index",
                boost::program_options::value<std::size_t>()->default_value(0),
                "start index of clients and topics"
            )
            (
                "times",
                boost::program_options::value<std::size_t>()->default_value(1000),
                "number of publishes for each client"
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
                "cid_prefix",
                boost::program_options::value<std::string>()->default_value(""),
                "client_id prefix. client_id is cid_prefix00000000 cid_prefix00000001 ..."
            )
            (
                "topic_prefix",
                boost::program_options::value<std::string>()->default_value(""),
                "topic_id prefix. topic is topic_prefix00000000 topic_prefix00000001 ..."
            )
            (
                "limit_ms",
                boost::program_options::value<std::size_t>()->default_value(0),
                "Output time over message if round trip time is greater than limit_ms. 0 means no limit"
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
                "clients",
                boost::program_options::value<std::size_t>()->default_value(1),
                "Number of clients."
            )
            (
                "con_interval_ms",
                boost::program_options::value<std::size_t>()->default_value(10),
                "connect interval (ms)"
            )
            (
                "sub_delay_ms",
                boost::program_options::value<std::size_t>()->default_value(1000),
                "subscribe delay after all connected (ms)"
            )
            (
                "sub_interval_ms",
                boost::program_options::value<std::size_t>()->default_value(10),
                "subscribe interval (ms)"
            )
            (
                "pub_delay_ms",
                boost::program_options::value<std::size_t>()->default_value(1000),
                "publish delay after all subscribed (ms)"
            )
            (
                "pub_after_idle_delay_ms",
                boost::program_options::value<std::size_t>()->default_value(1000),
                "publish delay after idle publishes are finished (ms)"
            )
            (
                "pub_interval_ms",
                boost::program_options::value<std::size_t>()->default_value(10),
                "publish interval for each clients (ms)"
            )
            (
                "detail_report",
                boost::program_options::value<bool>()->default_value(false),
                "report for each client's max mid avg min"
            )
            (
                "pub_idle_count",
                boost::program_options::value<std::size_t>()->default_value(1),
                "ideling publish count. it is useful to ignore authorization cache."
            )
            (
                "progress_timer_sec",
                boost::program_options::value<std::size_t>()->default_value(10),
                "report progress timer for each given seconds."
            )
            (
                "verbose",
                boost::program_options::value<unsigned int>()->default_value(1),
                "set verbose level, possible values:\n 0 - Fatal\n 1 - Error\n 2 - Warning\n 3 - Info\n 4 - Debug\n 5 - Trace"
            )
            (
                "cacert",
                boost::program_options::value<std::string>(),
                "CA Certificate file to verify server certificate for mqtts and wss connections"
            )
            (
                "ws_path",
                boost::program_options::value<std::string>(),
                "Web-Socket path for ws and wss connections"
            )
            (
                "mode",
                boost::program_options::value<std::string>()->default_value("single"),
                "bench mode. [single|send|recv] "
                "single is send/recv by bench. "
                "send is publish only. payload contains timestamp. "
                "recv is receive only. time is caluclated by timestamp. "
            )
            (
                "manager",
                boost::program_options::value<std::size_t>(),
                "managing publisher workers. the value is number of workers. "
                "manager itself is also a publisher. if --manager 5, then 6 publisher exists. "
            )
            (
                "manager_port",
                boost::program_options::value<std::uint16_t>(),
                "the communication port that communicate with wokers. "
            )
            (
                "work_for",
                boost::program_options::value<std::string>(),
                "manager_host:port work as a worker that is managed by the manager. "
            )
            (
                "close_after_report",
                boost::program_options::value<bool>()->default_value(true),
                "All clients disconnect after report. Set false if you want to avoid noises by close on multiple bench measuring. "
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
                          << "' not found,  bench doesn't use configuration file." << std::endl;
            }
        }

        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        }

        std::cout << "Set options:" << std::endl;
        for (auto const& e : vm) {
            std::cout << boost::format("%-24s") % e.first.c_str() << " : ";
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
            else if (auto p = boost::any_cast<std::vector<std::string>>(&e.second.value())) {
                for (auto const& e : *p) {
                    std::cout << e << " ";
                }
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

        if (!vm.count("target")) {
            std::cerr << "target host:port must be set at least one entry" << std::endl;
            return -1;
        }

        std::chrono::steady_clock::time_point tp_start = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point tp_sub_delay;
        std::chrono::steady_clock::time_point tp_subscribe;
        std::chrono::steady_clock::time_point tp_pub_delay;
        std::chrono::steady_clock::time_point tp_idle;
        std::chrono::steady_clock::time_point tp_pub_after_idle_delay;
        std::chrono::steady_clock::time_point tp_publish;

        auto detail_report = vm["detail_report"].as<bool>();

        am::optional<am::host_port> manager_hp;
        std::uint16_t manager_port = 0;
        std::size_t num_of_workers = 0;

        if (vm.count("manager")) {
            if (vm.count("work_for")) {
                std::cerr << "you cannot set options both manager and work_for" << std::endl;
                return -1;
            }
            if (!vm.count("manager_port")) {
                std::cerr << "if you set the option manager, then you need to set manager_port" << std::endl;
                return -1;
            }
            num_of_workers = vm["manager"].as<std::size_t>();
            if (num_of_workers == 0) {
                std::cerr << "manager option value (number of workers) must be greater than 0" << std::endl;
                return -1;
            }
            manager_port = vm["manager_port"].as<std::uint16_t>();
        }
        else if (vm.count("work_for")) {
            auto str = vm["work_for"].as<std::string>();
            manager_hp = am::host_port_from_string(str);
            if (!manager_hp) {
                std::cout
                    << "at option work_for, invalid host:port is set:"
                    << str
                    << std::endl;
                return -1;
            }
        }

        auto target = vm["target"].as<std::vector<std::string>>();
        auto target_index = vm["target_index"].as<std::size_t>();
        if (target_index >= target.size()) {
            std::cout
                << "target_index is out of the target size."
                << " target_index:" << target_index
                << " target size:" << target.size()
                << std::endl;
            return -1;
        }

        std::vector<am::host_port> hps;
        for (auto& t : target) {
            auto hp_opt = am::host_port_from_string(t);
            if (hp_opt) {
                hps.emplace_back(am::force_move(*hp_opt));
            }
            else {
                std::cout
                    << "at option target, invalid host:port is set:"
                    << t
                    << std::endl;
                return -1;
            }
        }
        auto protocol = vm["protocol"].as<std::string>();
        auto mqtt_version = vm["mqtt_version"].as<std::string>();
        auto qos = static_cast<am::qos>(vm["qos"].as<unsigned int>());
        auto retain =
            [&] () -> am::pub::retain {
                if (vm["retain"].as<bool>()) {
                    return am::pub::retain::yes;
                }
                return am::pub::retain::no;
            } ();
        auto clean_start = vm["clean_start"].as<bool>();
        auto sei = vm["sei"].as<std::uint32_t>();
        auto payload_size = vm["payload_size"].as<std::size_t>();
        if (payload_size <= min_payload) {
            std::cout
                << "payload_size must be greater than "
                << std::to_string(min_payload)
                << ". payload_size:" << payload_size
                << std::endl;
            return -1;
        }
        auto compare = vm["compare"].as<bool>();

        auto clients = vm["clients"].as<std::size_t>();
        auto start_index = vm["start_index"].as<std::size_t>();
        auto times = vm["times"].as<std::size_t>();
        if (times == 0) {
            std::cout << "times must be greater than 0" << std::endl;
            return -1;
        }
        auto pub_idle_count = vm["pub_idle_count"].as<std::size_t>();
        times += pub_idle_count;

        auto username =
            [&] () -> am::optional<std::string> {
                if (vm.count("username")) {
                    return vm["username"].as<std::string>();
                }
                return am::nullopt;
            } ();
        auto password =
            [&] () -> am::optional<std::string> {
                if (vm.count("password")) {
                    return vm["password"].as<std::string>();
                }
                return am::nullopt;
            } ();
        auto cid_prefix = vm["cid_prefix"].as<std::string>();
        auto topic_prefix = vm["topic_prefix"].as<std::string>();

        auto cacert =
            [&] () -> am::optional<std::string> {
                if (vm.count("cacert")) {
                    return vm["cacert"].as<std::string>();
                }
                return am::nullopt;
            } ();
        auto ws_path =
            [&] () -> am::optional<std::string> {
                if (vm.count("ws_path")) {
                    return vm["ws_path"].as<std::string>();
                }
                return am::nullopt;
            } ();

        auto limit_ms = vm["limit_ms"].as<std::size_t>();

        mode md;
        auto md_str = vm["mode"].as<std::string>();
        if (md_str == "single") {
            md = mode::single;
        }
        else if (md_str == "send") {
            md = mode::send;
        }
        else if (md_str == "recv") {
            md = mode::recv;
        }
        else {
            std::cout
                << "invalid mode:" << md_str
                << " mode should be [single|send|recv]."
                << std::endl;
            return -1;
        }

        if (md == mode::recv) {
            if (num_of_workers > 0) {
                std::cout
                    << "you cannot set options both mode recv and manager"
                    << std::endl;
                return -1;
            }
            if (manager_hp) {
                std::cout
                    << "you cannot set options both mode recv and work_for"
                    << std::endl;
                return -1;
            }
        }
        auto con_interval_ms = vm["con_interval_ms"].as<std::size_t>();
        auto sub_delay_ms = vm["sub_delay_ms"].as<std::size_t>();
        auto sub_interval_ms = vm["sub_interval_ms"].as<std::size_t>();
        auto pub_delay_ms = vm["pub_delay_ms"].as<std::size_t>();
        auto pub_after_idle_delay_ms = vm["pub_after_idle_delay_ms"].as<std::size_t>();
        auto pub_interval_ms = vm["pub_interval_ms"].as<std::size_t>();

        auto progress_timer_sec = vm["progress_timer_sec"].as<std::size_t>();

        std::uint64_t pub_interval_us = pub_interval_ms * 1000;
        std::cout << "pub_interval:" << pub_interval_us << " us" << std::endl;
        std::uint64_t all_interval_ns = pub_interval_us * 1000 / static_cast<std::uint64_t>(clients);
        std::cout << "all_interval:" << all_interval_ns << " ns" << std::endl;
        auto pps_str = boost::lexical_cast<std::string>(std::uint64_t(1) * 1000 * 1000 * 1000 / all_interval_ns);
        auto pps_str_with_comma =
            [&] {
                std::size_t num_of_comma = (pps_str.size() - 1) / 3;
                std::size_t i = 3 - (pps_str.size() - 1) % 3;
                std::string result;
                result.reserve(pps_str.size() + num_of_comma);
                for (auto c : pps_str) {
                    result += c;
                    if (i % 3 == 0 && num_of_comma != 0) {
                        result += ',';
                        --num_of_comma;
                    }
                    ++i;
                }
                return result;
            }();
        std::cout << pps_str_with_comma <<  " publish/sec" << std::endl;
        auto num_of_iocs =
            [&] () -> std::size_t {
                if (vm.count("iocs")) {
                    return vm["iocs"].as<std::size_t>();
                }
                return 1;
            } ();
        if (num_of_iocs == 0) {
            num_of_iocs = std::thread::hardware_concurrency();
            std::cout << "iocs set to auto decide (0). Automatically set to " << num_of_iocs << std::endl;;
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
            std::cout << "threads_per_ioc set to auto decide (0). Automatically set to " << threads_per_ioc << std::endl;
        }

        std::cout
            << "iocs:" << num_of_iocs
            << " threads_per_ioc:" << threads_per_ioc
            << " total threads:" << num_of_iocs * threads_per_ioc
            << std::endl;

        std::vector<as::io_context> iocs(num_of_iocs);
        BOOST_ASSERT(!iocs.empty());

        std::vector<
            as::executor_work_guard<
                as::io_context::executor_type
            >
        > guard_iocs;
        guard_iocs.reserve(iocs.size());
        for (auto& ioc : iocs) {
            guard_iocs.emplace_back(ioc.get_executor());
        }

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

        std::cout << "Prepare clients" << std::endl;
        std::cout << "  protocol:" << protocol << std::endl;

        struct client_info_base {
            client_info_base(
                std::string cid_prefix,
                std::size_t index,
                std::size_t payload_size,
                std::size_t times,
                std::size_t idle_count,
                std::string host,
                std::string port
            )
                :cid_prefix{am::force_move(cid_prefix)},
                 index_str{(boost::format("%08d") % index).str()},
                 send_times{times},
                 recv_times{times},
                 send_idle_count{idle_count},
                 recv_idle_count{idle_count},
                 host{am::force_move(host)},
                 port{am::force_move(port)}
            {
                payload_str.resize(payload_size);
                sent.resize(times);
                auto it = payload_str.begin() + min_payload + 1;
                auto end = payload_str.end();
                char c = 'A';
                for (; it != end; ++it) {
                    *it = c++;
                    if (c == 'Z') c = 'A';
                }
            }

            std::string get_client_id() const {
                return cid_prefix + index_str;
            }
            am::buffer send_payload(mode md) {
                std::string ret = payload_str;
                auto variable =
                    [&] {
                        switch (md) {
                        case mode::single:
                            return (boost::format("%s%08d") % index_str % send_times).str();
                        case mode::send:
                            return
                                (boost::format("%s%08d%032d")
                                 % index_str
                                 % send_times
                                 % std::chrono::duration_cast<std::chrono::nanoseconds>(
                                     // use system clock for multi node synchronization
                                     std::chrono::system_clock::now().time_since_epoch()).count()
                                ).str();
                        default:
                            locked_cout() << "invalid mode" << std::endl;
                            return std::string{};
                        }
                    } ();

                std::copy(variable.begin(), variable.end(), ret.begin());
                return am::allocate_buffer(ret);
            }

            am::buffer recv_payload() const {
                std::string ret = payload_str;
                auto variable = (boost::format("%s%08d") %index_str % recv_times).str();
                std::copy(variable.begin(), variable.end(), ret.begin());
                return am::allocate_buffer(ret);
            }

#if BOOST_VERSION < 107400 || defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
            using executor_t =  as::executor;
#else  // BOOST_VERSION < 107400 || defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
            using executor_t =  as::any_io_executor;
#endif // BOOST_VERSION < 107400 || defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

            void init_timer(executor_t exe) {
                tim = std::make_shared<as::steady_timer>(exe);
            }

            std::string cid_prefix;
            std::string index_str;
            std::string payload_str;
            std::size_t send_times;
            std::size_t recv_times;
            std::size_t send_idle_count;
            std::size_t recv_idle_count;
            std::vector<std::chrono::steady_clock::time_point> sent;
            std::vector<std::size_t> rtt_us;
            std::shared_ptr<as::steady_timer> tim;
            std::string host;
            std::string port;
        };

        as::io_context ioc_timer;
        as::io_context ioc_progress_timer;

        as::io_context ioc_sync;

        std::atomic<phase> ph{phase::connect};
        auto tim_progress = std::make_shared<as::steady_timer>(ioc_progress_timer);
        as::executor_work_guard<as::io_context::executor_type> guard_ioc_timer(ioc_timer.get_executor());
        as::steady_timer tim_delay{ioc_timer};
        as::system_timer tim_sync{ioc_timer};

        std::atomic<std::size_t> rest_connect{clients};
        std::atomic<std::size_t> rest_sub{clients};
        std::atomic<std::size_t> rest_idle{pub_idle_count * clients};
        std::atomic<std::uint64_t> rest_times{times * clients};

        as::ip::tcp::resolver res{ioc_timer.get_executor()};

        std::function <void()> tim_progress_proc =
            [&, wp = std::weak_ptr<as::steady_timer>(tim_progress)] {
                if (auto sp = wp.lock()) {
                    sp->expires_after(std::chrono::seconds(progress_timer_sec));
                    sp->async_wait(
                        [&] (boost::system::error_code const& ec) {
                            if (!ec) {
                                std::string ph_str;
                                auto now = std::chrono::steady_clock::now();
                                std::chrono::steady_clock::time_point tp_base;

                                switch(ph.load()) {
                                case phase::connect:
                                    ph_str = "connect";
                                    tp_base = tp_start;
                                    break;
                                case phase::sub_delay:
                                    ph_str = "sub_delay";
                                    tp_base = tp_sub_delay;
                                    break;
                                case phase::subscribe:
                                    ph_str = "subscribe";
                                    tp_base = tp_subscribe;
                                    break;
                                case phase::pub_delay:
                                    ph_str = "pub_delay";
                                    tp_base = tp_pub_delay;
                                    break;
                                case phase::idle:
                                    ph_str = "idle";
                                    tp_base = tp_idle;
                                    break;
                                case phase::pub_after_idle_delay:
                                    ph_str = "pub_after_idle_delay";
                                    tp_base = tp_pub_after_idle_delay;
                                    break;
                                case phase::publish:
                                    ph_str = "publish";
                                    tp_base = tp_publish;
                                    break;
                                default:
                                    BOOST_ASSERT(false);
                                    break;
                                }
                                auto dur_ph = std::chrono::duration_cast<std::chrono::seconds>(
                                    now - tp_base
                                ).count();
                                auto dur_total = std::chrono::duration_cast<std::chrono::seconds>(
                                    now - tp_start
                                ).count();
                                locked_cout()
                                    << "[progress] "
                                    << (
                                        boost::format("%-24s: %3d:%02d | total: %3d:%02d") %
                                        ph_str %
                                        (dur_ph / 60) % (dur_ph % 60) %
                                        (dur_total / 60) % (dur_total % 60)
                                    )
                                    << std::endl;
                                tim_progress_proc();
                            }
                        }
                    );
                }
            };

        std::vector<std::shared_ptr<as::ip::tcp::socket>> workers;
        workers.reserve(num_of_workers);
        if (num_of_workers != 0) {
            locked_cout() << "work as manager. num_of_workers:" << num_of_workers << std::endl;

            as::ip::tcp::acceptor ac{ioc_sync, as::ip::tcp::endpoint{as::ip::tcp::v4(), manager_port}};

            // as::ip::tcp::acceptor ac{ioc_sync, manager_port};
            for (std::size_t i = 0; i != num_of_workers; ++i) {
                auto s = std::make_shared<as::ip::tcp::socket>(ioc_sync);
                ac.accept(*s);
                workers.emplace_back(am::force_move(s));
                locked_cout() << "accepted" << std::endl;
            }
        }

        auto run_and_join =
            [&] {
                if (progress_timer_sec > 0) {
                    tim_progress_proc();
                }
                std::thread th_progress_timer {
                    [&] {
                        ioc_progress_timer.run();
                    }
                };
                std::thread th_timer {
                    [&] {
                        ioc_timer.run();
                    }
                };
                std::vector<std::thread> ths;
                ths.reserve(num_of_iocs * threads_per_ioc);
                for (auto& ioc : iocs) {
                    for (std::size_t i = 0; i != threads_per_ioc; ++i) {
                        ths.emplace_back(
                            [&] {
                                ioc.run();
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
                            std::cerr << "Signal " << num << " received. exit program" << std::endl;
                            exit(-1);
                        }
                    }
                );
                std::thread th_signal {
                    [&] {
                        ioc_signal.run();
                    }
                };


                for (auto& th : ths) th.join();
                th_timer.join();
                tim_progress->cancel();
                th_progress_timer.join();
                signals.cancel();
                th_signal.join();
            };

        auto bc = bench_context(
            res,
            ws_path,
            version,
            sei,
            clean_start,
            username,
            password,
            topic_prefix,
            qos,
            retain,
            ph,
            ioc_timer,
            tim_progress,
            guard_ioc_timer,
            tim_delay,
            tim_sync,
            pub_idle_count,
            rest_connect,
            rest_sub,
            rest_idle,
            rest_times,
            con_interval_ms,
            sub_delay_ms,
            sub_interval_ms,
            pub_delay_ms,
            pub_after_idle_delay_ms,
            pub_interval_ms,
            all_interval_ns,
            limit_ms,
            compare,
            tp_start,
            tp_sub_delay,
            tp_subscribe,
            tp_pub_delay,
            tp_idle,
            tp_pub_after_idle_delay,
            tp_publish,
            detail_report,
            guard_iocs,
            md,
            workers,
            manager_hp,
            vm["close_after_report"].as<bool>()
        );

        if (protocol == "mqtt") {
            using client_t = am::endpoint<am::role::client, am::protocol::mqtt>;
            struct client_info : client_info_base {
                using client_type = client_t;
                client_info(
                    client_t c,
                    std::string cid_prefix,
                    std::size_t index,
                    std::size_t payload_size,
                    std::size_t times,
                    std::size_t idle_count,
                    std::string host,
                    std::string port
                )
                    :client_info_base{
                        am::force_move(cid_prefix),
                        index,
                        payload_size,
                        times,
                        idle_count,
                        am::force_move(host),
                        am::force_move(port)
                     },
                     c{am::force_move(c)}
                {
                }
                client_t c;
            };

            std::vector<client_info> cis;
            cis.reserve(clients);
            std::size_t hps_index = target_index;
            for (std::size_t i = 0; i != clients; ++i) {
                cis.emplace_back(
                    client_t{
                        version,
                        iocs.at(i % num_of_iocs).get_executor()
                    },
                    cid_prefix,
                    i + start_index,
                    payload_size,
                    times,
                    pub_idle_count,
                    hps[hps_index].host,
                    std::to_string(hps[hps_index].port)
                );
                ++hps_index;
                if (hps_index == hps.size()) hps_index = 0;
            }
            auto b = bench(
                cis,
                bc
            );
            b();
            run_and_join();
        }
        else if (protocol == "mqtts") {
#if defined(ASYNC_MQTT_USE_TLS)
            using client_t = am::endpoint<am::role::client, am::protocol::mqtts>;
            struct client_info : client_info_base {
                using client_type = client_t;
                client_info(
                    client_t c,
                    std::string cid_prefix,
                    std::size_t index,
                    std::size_t payload_size,
                    std::size_t times,
                    std::size_t idle_count,
                    std::string host,
                    std::string port
                )
                    :client_info_base{
                        am::force_move(cid_prefix),
                        index,
                        payload_size,
                        times,
                        idle_count,
                        am::force_move(host),
                        am::force_move(port)
                     },
                     c{am::force_move(c)}
                {
                }
                client_t c;
            };

            std::vector<client_info> cis;
            cis.reserve(clients);
            std::size_t hps_index = target_index;
            for (std::size_t i = 0; i != clients; ++i) {
                am::tls::context ctx{am::tls::context::tlsv12};
                if (cacert) {
                    ctx.set_verify_mode(am::tls::verify_peer);
                    ctx.load_verify_file(*cacert);
                }
                else {
                    ctx.set_verify_mode(am::tls::verify_none);
                }
                cis.emplace_back(
                    client_t{
                        version,
                        iocs.at(i % num_of_iocs).get_executor(),
                        ctx
                    },
                    cid_prefix,
                    i + start_index,
                    payload_size,
                    times,
                    pub_idle_count,
                    hps[hps_index].host,
                    std::to_string(hps[hps_index].port)
                );
                ++hps_index;
                if (hps_index == hps.size()) hps_index = 0;
            }
            auto b = bench(
                cis,
                bc
            );
            b();
            run_and_join();
            return 0;
#else  // defined(ASYNC_MQTT_USE_TLS)
            std::cout << "ASYNC_MQTT_USE_TLS compiler option is required" << std::endl;
            return -1;
#endif // defined(ASYNC_MQTT_USE_TLS)
        }
        else if (protocol == "ws") {
#if defined(ASYNC_MQTT_USE_WS)
            using client_t = am::endpoint<am::role::client, am::protocol::ws>;
            struct client_info : client_info_base {
                using client_type = client_t;
                client_info(
                    client_t c,
                    std::string cid_prefix,
                    std::size_t index,
                    std::size_t payload_size,
                    std::size_t times,
                    std::size_t idle_count,
                    std::string host,
                    std::string port
                )
                    :client_info_base{
                        am::force_move(cid_prefix),
                        index,
                        payload_size,
                        times,
                        idle_count,
                        am::force_move(host),
                        am::force_move(port)
                     },
                     c{am::force_move(c)}
                {
                }
                client_t c;
            };

            std::vector<client_info> cis;
            cis.reserve(clients);
            std::size_t hps_index = target_index;
            for (std::size_t i = 0; i != clients; ++i) {
                cis.emplace_back(
                    client_t{
                        version,
                        iocs.at(i % num_of_iocs).get_executor()
                    },
                    cid_prefix,
                    i + start_index,
                    payload_size,
                    times,
                    pub_idle_count,
                    hps[hps_index].host,
                    std::to_string(hps[hps_index].port)
                );
                ++hps_index;
                if (hps_index == hps.size()) hps_index = 0;
            }
            auto b = bench(
                cis,
                bc
            );
            b();
            run_and_join();
            return 0;
#else  // defined(ASYNC_MQTT_USE_WS)
            std::cout << "ASYNC_MQTT_USE_WS compiler option is required" << std::endl;
            return -1;
#endif // defined(ASYNC_MQTT_USE_WS)
        }
        else if (protocol == "wss") {
#if defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)
            using client_t = am::endpoint<am::role::client, am::protocol::wss>;
            struct client_info : client_info_base {
                using client_type = client_t;
                client_info(
                    client_t c,
                    std::string cid_prefix,
                    std::size_t index,
                    std::size_t payload_size,
                    std::size_t times,
                    std::size_t idle_count,
                    std::string host,
                    std::string port
                )
                    :client_info_base{
                        am::force_move(cid_prefix),
                        index,
                        payload_size,
                        times,
                        idle_count,
                        am::force_move(host),
                        am::force_move(port)
                     },
                     c{am::force_move(c)}
                {
                }
                client_t c;
            };

            std::vector<client_info> cis;
            cis.reserve(clients);
            std::size_t hps_index = target_index;
            for (std::size_t i = 0; i != clients; ++i) {
                am::tls::context ctx{am::tls::context::tlsv12};
                if (cacert) {
                    ctx.set_verify_mode(am::tls::verify_peer);
                    ctx.load_verify_file(*cacert);
                }
                else {
                    ctx.set_verify_mode(am::tls::verify_none);
                }
                cis.emplace_back(
                    client_t{
                        version,
                        iocs.at(i % num_of_iocs).get_executor(),
                        ctx
                    },
                    cid_prefix,
                    i + start_index,
                    payload_size,
                    times,
                    pub_idle_count,
                    hps[hps_index].host,
                    std::to_string(hps[hps_index].port)
                );
                ++hps_index;
                if (hps_index == hps.size()) hps_index = 0;
            }
            auto b = bench(
                cis,
                bc
            );
            b();
            run_and_join();
            return 0;
#else  // defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)
            std::cout << "ASYNC_MQTT_USE_TLS and ASYNC_MQTT_USE_WS compiler option are required" << std::endl;
            return -1;
#endif // defined(ASYNC_MQTT_USE_TLS) && defined(ASYNC_MQTT_USE_WS)
        }
        else {
            std::cerr << "invalid protocol:" << protocol << " it should be mqtt, mqtts, ws, or wss" << std::endl;
            return -1;
        }


    }
    catch(std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}
