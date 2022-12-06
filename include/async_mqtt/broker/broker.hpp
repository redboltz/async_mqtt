// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_BROKER_HPP)
#define ASYNC_MQTT_BROKER_BROKER_HPP

#include <async_mqtt/endpoint_variant.hpp>
#include <async_mqtt/util/scope_guard.hpp>
#include <async_mqtt/broker/security.hpp>
#include <async_mqtt/broker/mutex.hpp>
#include <async_mqtt/broker/session_state.hpp>
#include <async_mqtt/broker/sub_con_map.hpp>
#include <async_mqtt/broker/retained_messages.hpp>
#include <async_mqtt/broker/retained_topic_map.hpp>
#include <async_mqtt/broker/shared_target_impl.hpp>
#include <async_mqtt/broker/mutex.hpp>
#include <async_mqtt/broker/uuid.hpp>

#include <async_mqtt/broker/constant.hpp>
#include <async_mqtt/broker/security.hpp>

namespace async_mqtt {


template <typename... NextLayer>
class broker {
    using epsp_t = endpoint_sp_variant<role::server, NextLayer...>;
    using epwp_t = endpoint_wp_variant<role::server, NextLayer...>;
public:
    broker(as::io_context& timer_ioc)
        :timer_ioc_{timer_ioc},
         tim_disconnect_{timer_ioc_} {
        security_.default_config();
    }

    void handle_accept(epsp_t epsp) {
        async_read_packet(force_move(epsp));
    }

private:
    void async_read_packet(epsp_t epsp) {
        epsp.recv(
            [this, epsp = force_move(epsp)]
            (packet_variant pv) {
                pv.visit(
                    overload {
                        [&](v3_1_1::connect_packet& p) {
                            connect_handler(
                                force_move(epsp),
                                p.client_id(),
                                p.user_name(),
                                p.password(),
                                p.will(),
                                p.clean_session(),
                                p.keep_alive(),
                                properties{}
                            );
                        },
                        [&](v5::connect_packet& p) {
                            connect_handler(
                                force_move(epsp),
                                p.client_id(),
                                p.user_name(),
                                p.password(),
                                p.will(),
                                p.clean_start(),
                                p.keep_alive(),
                                p.props()
                            );
                        },
                        [&](v3_1_1::publish_packet& p) {
                            publish_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.opts(),
                                p.topic(),
                                p.payload(),
                                properties{}
                            );
                        },
                        [&](v5::publish_packet& p) {
                            publish_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.opts(),
                                p.topic(),
                                p.payload(),
                                p.props()
                            );
                        },
                        [&](v3_1_1::puback_packet& p) {
                            puback_handler(
                                force_move(epsp),
                                p.packet_id(),
                                puback_reason_code::success,
                                properties{}
                            );
                        },
                        [&](v5::puback_packet& p) {
                            puback_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.code(),
                                p.props()
                            );
                        },
                        [&](v3_1_1::pubrec_packet& p) {
                            pubrec_handler(
                                force_move(epsp),
                                p.packet_id(),
                                pubrec_reason_code::success,
                                properties{}
                            );
                        },
                        [&](v5::pubrec_packet& p) {
                            pubrec_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.code(),
                                p.props()
                            );
                        },
                        [&](v3_1_1::pubrel_packet& p) {
                            pubrel_handler(
                                force_move(epsp),
                                p.packet_id(),
                                pubrel_reason_code::success,
                                properties{}
                            );
                        },
                        [&](v5::pubrel_packet& p) {
                            pubrel_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.code(),
                                p.props()
                            );
                        },
                        [&](v3_1_1::pubcomp_packet& p) {
                            pubcomp_handler(
                                force_move(epsp),
                                p.packet_id(),
                                pubcomp_reason_code::success,
                                properties{}
                            );
                        },
                        [&](v5::pubcomp_packet& p) {
                            pubcomp_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.code(),
                                p.props()
                            );
                        },
                        [&](v3_1_1::subscribe_packet& p) {
                            subscribe_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.entries(),
                                properties{}
                            );
                        },
                        [&](v5::subscribe_packet& p) {
                            subscribe_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.entries(),
                                p.props()
                            );
                        },
                        [&](v3_1_1::disconnect_packet&) {
                            disconnect_handler(
                                force_move(epsp),
                                disconnect_reason_code::normal_disconnection,
                                properties{}
                            );
                        },
                        [&](v5::disconnect_packet& p) {
                            disconnect_handler(
                                force_move(epsp),
                                p.code(),
                                p.props()
                            );
                        },
                        [&](system_error const&) {
                            // TBD connack or disconnect send on error
                            close_proc(
                                force_move(epsp),
                                true // send_will
                            );
                        },
                        [&](auto const&) {
                        }
                    }
                );
            }
        );
    }

    void connect_handler(
        epsp_t epsp,
        buffer client_id,
        optional<buffer> noauth_username,
        optional<buffer> password,
        optional<will> will,
        bool clean_start,
        std::uint16_t /*keep_alive*/,
        properties props
    ) {
        optional<std::string> username;
        if (auto paun_opt = epsp.get_preauthed_user_name()) {
            if (security_.login_cert(*paun_opt)) {
                username = force_move(*paun_opt);
            }
        }
        else if (!noauth_username && !password) {
            username = security_.login_anonymous();
        }
        else if (noauth_username && password) {
            username = security_.login(*noauth_username, *password);
        }

        // If login fails, try the unauthenticated user
        if (!username) username = security_.login_unauthenticated();

        optional<std::chrono::steady_clock::duration> session_expiry_interval;
        optional<std::chrono::steady_clock::duration> will_expiry_interval;
        bool response_topic_requested = false;

        if (epsp.get_protocol_version() == protocol_version::v5) {
            for (auto const& prop : props) {
                prop.visit(
                    overload {
                        [&](property::session_expiry_interval const& v) {
                            if (v.val() != 0) {
                                session_expiry_interval.emplace(std::chrono::seconds(v.val()));
                            }
                        },
                        [&](property::request_response_information const& v) {
                            response_topic_requested = v.val();
                        }
                    }
                );
            }
            if (will) {
                for (auto const& prop : will->props()) {
                    prop.visit(
                        overload {
                            [&](property::message_expiry_interval const& v) {
                                will_expiry_interval.emplace(std::chrono::seconds(v.val()));
                            }
                        }
                    );
                }
            }

            // for test
            if (h_connect_props_) {
                h_connect_props_(props);
            }
        }

        properties connack_props;

        if (!username) {
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "User failed to login: "
                << (noauth_username ? std::string(*noauth_username) : std::string("anonymous user"));

            send_connack(
                epsp,
                false, // session present
                false, // authenticated
                force_move(connack_props),
                [epsp](error_code) {
                    disconnect_and_force_disconnect(epsp, disconnect_reason_code::not_authorized);
                }
            );
            async_read_packet(force_move(epsp));
            return;
        }

        if (client_id.empty()) {
            if (!handle_empty_client_id(epsp, client_id, clean_start, connack_props)) {
                return;
            }
            // A new client id was generated
            client_id = buffer(string_view(epsp->get_client_id()));
        }

        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "User logged in as: '" << *username << "', client_id: " << client_id;

        /**
         * http://docs.oasis-open.org/mqtt/mqtt/v5.0/cs02/mqtt-v5.0-cs02.html#_Toc514345311
         * 3.1.2.4 Clean Start
         * If a CONNECT packet is received with Clean Start is set to 1, the Client and Server MUST
         * discard any existing Session and start a new Session [MQTT-3.1.2-4]. Consequently,
         *  the Session Present flag in CONNACK is always set to 0 if Clean Start is set to 1.
         */

        // Find any sessions that have the same client_id
        std::lock_guard<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_cid>();
        auto it = idx.lower_bound(std::make_tuple(*username, client_id));
        if (it == idx.end() ||
            it->client_id() != client_id ||
            it->get_username() != *username
        ) {
            // new connection
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "cid:" << client_id
                << " new connection inserted.";
            it = idx.emplace_hint(
                it,
                timer_ioc_,
                mtx_subs_map_,
                subs_map_,
                shared_targets_,
                epsp,
                client_id,
                *username,
                force_move(will),
                // will_sender
                [this](auto&&... params) {
                    do_publish(std::forward<decltype(params)>(params)...);
                },
                force_move(will_expiry_interval),
                force_move(session_expiry_interval)
            );
            if (response_topic_requested) {
                // set_response_topic never modify key part
                set_response_topic(const_cast<session_state<NextLayer...>&>(*it), connack_props, *username);
            }

            send_connack(
                epsp,
                false, // session present
                true,  // authenticated
                force_move(connack_props)
            );
        }
        else if (it->online()) {
            // online overwrite
            if (close_proc_no_lock(it->con(), true, disconnect_reason_code::session_taken_over)) {
                // remain offline
                if (clean_start) {
                    // discard offline session
                    ASYNC_MQTT_LOG("mqtt_broker", trace)
                        << ASYNC_MQTT_ADD_VALUE(address, this)
                        << "cid:" << client_id
                        << "online connection exists, discard old one due to new one's clean_start and renew";
                    if (response_topic_requested) {
                        // set_response_topic never modify key part
                        set_response_topic(const_cast<session_state<NextLayer...>&>(*it), connack_props, *username);
                    }
                    send_connack(
                        epsp,
                        false, // session present
                        true,  // authenticated
                        force_move(connack_props)
                    );
                    idx.modify(
                        it,
                        [&](auto& e) {
                            e.clean();
                            e.update_will(timer_ioc_, force_move(will), will_expiry_interval);
                            e.set_username(*username);
                            // renew_session_expiry updates index
                            e.renew_session_expiry(force_move(session_expiry_interval));
                        },
                        [](auto&) { BOOST_ASSERT(false); }
                    );
                }
                else {
                    // inherit online session if previous session's session exists
                    ASYNC_MQTT_LOG("mqtt_broker", trace)
                        << ASYNC_MQTT_ADD_VALUE(address, this)
                        << "cid:" << client_id
                        << "online connection exists, inherit old one and renew";
                    if (response_topic_requested) {
                        // set_response_topic never modify key part
                        set_response_topic(const_cast<session_state<NextLayer...>&>(*it), connack_props, *username);
                    }
                    send_connack(
                        epsp,
                        true, // session present
                        true, // authenticated
                        force_move(connack_props),
                        [
                            this,
                            &idx,
                            it,
                            will = force_move(will),
                            clean_start,
                            epsp,
                            will_expiry_interval,
                            session_expiry_interval = session_expiry_interval,
                            username
                        ]
                        (error_code ec) mutable {
                            if (ec) {
                                ASYNC_MQTT_LOG("mqtt_broker", trace)
                                    << ASYNC_MQTT_ADD_VALUE(address, this)
                                    << ec.message();
                                return;
                            }
                            idx.modify(
                                it,
                                [&](auto& e) {
                                    e.renew(epsp, clean_start);
                                    e.set_username(*username);
                                    e.update_will(timer_ioc_, force_move(will), will_expiry_interval);
                                    // renew_session_expiry updates index
                                    e.renew_session_expiry(force_move(session_expiry_interval));
                                    e.send_inflight_messages();
                                    e.send_all_offline_messages();
                                },
                                [](auto&) { BOOST_ASSERT(false); }
                            );
                        }
                    );
                }
            }
            else {
                // new connection
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, this)
                    << "cid:" << client_id
                    << "online connection exists, discard old one due to session_expiry and renew";
                bool inserted;
                std::tie(it, inserted) = idx.emplace(
                    timer_ioc_,
                    mtx_subs_map_,
                    subs_map_,
                    shared_targets_,
                    epsp,
                    client_id,
                    *username,
                    force_move(will),
                    // will_sender
                    [this](auto&&... params) {
                        do_publish(std::forward<decltype(params)>(params)...);
                    },
                    force_move(will_expiry_interval),
                    force_move(session_expiry_interval)
                );
                BOOST_ASSERT(inserted);
                if (response_topic_requested) {
                    // set_response_topic never modify key part
                    set_response_topic(const_cast<session_state<NextLayer...>&>(*it), connack_props, *username);
                }
                send_connack(
                    epsp,
                    false, // session present
                    true,  // authenticated
                    force_move(connack_props)
                );
            }
        }
        else {
            // offline -> online
            if (clean_start) {
                // discard offline session
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, this)
                    << "cid:" << client_id
                    << "offline connection exists, discard old one due to new one's clean_start and renew";
                if (response_topic_requested) {
                    // set_response_topic never modify key part
                    set_response_topic(const_cast<session_state<NextLayer...>&>(*it), connack_props, *username);
                }
                send_connack(
                    epsp,
                    false, // session present
                    true,  // authenticated
                    force_move(connack_props)
                );
                idx.modify(
                    it,
                    [&](auto& e) {
                        e.clean();
                        e.renew(epsp, clean_start);
                        e.update_will(timer_ioc_, force_move(will), will_expiry_interval);
                        e.set_username(*username);
                        // renew_session_expiry updates index
                        e.renew_session_expiry(force_move(session_expiry_interval));
                    },
                    [](auto&) { BOOST_ASSERT(false); }
                );
            }
            else {
                // inherit offline session
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, this)
                    << "cid:" << client_id
                    << "offline connection exists, inherit old one and renew";
                if (response_topic_requested) {
                    // set_response_topic never modify key part
                    set_response_topic(const_cast<session_state<NextLayer...>&>(*it), connack_props, *username);
                }
                send_connack(
                    epsp,
                    true, // session present
                    true,  // authenticated
                    force_move(connack_props),
                    [
                        this,
                        &idx,
                        it,
                        will = force_move(will),
                        clean_start,
                        epsp,
                        will_expiry_interval,
                        session_expiry_interval,
                        username
                    ]
                    (error_code ec) mutable {
                        if (ec) {
                            ASYNC_MQTT_LOG("mqtt_broker", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << ec.message();
                            return;
                        }
                        idx.modify(
                            it,
                            [&](auto& e) {
                                e.renew(epsp, clean_start);
                                e.set_username(*username);
                                e.update_will(timer_ioc_, force_move(will), will_expiry_interval);
                                // renew_session_expiry updates index
                                e.renew_session_expiry(force_move(session_expiry_interval));
                                e.send_inflight_messages();
                                e.send_all_offline_messages();
                            },
                            [](auto&) { BOOST_ASSERT(false); }
                        );
                    }
                );
            }
        }

        async_read_packet(force_move(epsp));
    }

    void send_connack(
        epsp_t& epsp,
        bool session_present,
        bool authenticated,
        properties props,
        std::function<void(system_error const&)> finish = [](system_error const&){}
    ) {
        // Reply to the connect message.
        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1:
            if (connack_) {
                epsp.send(
                    v3_1_1::connack_packet{
                        session_present,
                        authenticated ? connect_return_code::accepted
                                      : connect_return_code::not_authorized,
                    },
                    [finish = force_move(finish)]
                    (system_error const& ec) {
                        finish(ec);
                    }
                );
            }
            break;
        case protocol_version::v5:
            // connack_props_ member varible is for testing
            if (connack_props_.empty()) {
                // props local variable is is for real case
                props.emplace_back(property::topic_alias_maximum{topic_alias_max});
                props.emplace_back(property::receive_maximum{receive_maximum_max});
                if (connack_) {
                    epsp.send(
                        v5::connack_packet{
                            session_present,
                            authenticated ? connect_reason_code::success
                                          : connect_reason_code::not_authorized,
                            force_move(props)
                        },
                        [finish = force_move(finish)]
                        (system_error const& ec) {
                            finish(ec);
                        }
                    );
                }
            }
            else {
                // use connack_props_ for testing
                if (connack_) {
                    epsp.send(
                        v5::connack_packet{
                            session_present,
                            authenticated ? connect_reason_code::success
                                          : connect_reason_code::not_authorized,
                            connack_props_
                        },
                        [finish = force_move(finish)]
                        (system_error const& ec) {
                            finish(ec);
                        }
                    );
                }
            }
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void publish_handler(
        epsp_t epsp,
        packet_id_t packet_id,
        pub::opts opts,
        buffer topic,
        buffer payload,
        properties props
    ) {
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );

        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) return;

        auto send_pubres =
            [&] (bool authorized = true) {
                switch (opts.qos()) {
                case qos::at_least_once:
                    switch (epsp.get_protocol_version()) {
                    case protocol_version::v3_1_1:
                        epsp.send(
                            v3_1_1::puback_packet{
                                packet_id
                            },
                            [epsp]
                            (system_error const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                        << ec.what();
                                }
                            }
                        );
                        break;
                    case protocol_version::v5:
                        epsp.send(
                            v5::puback_packet{
                                packet_id,
                                authorized ? puback_reason_code::success
                                           : puback_reason_code::not_authorized,
                                puback_props_
                            },
                            [epsp]
                            (system_error const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                        << ec.what();
                                }
                            }
                        );
                        break;
                    default:
                        BOOST_ASSERT(false);
                        break;
                    }
                    break;
                case qos::exactly_once:
                    switch (epsp.get_protocol_version()) {
                    case protocol_version::v3_1_1:
                        epsp.send(
                            v3_1_1::pubrec_packet{
                                packet_id
                            },
                            [epsp]
                            (system_error const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                        << ec.what();
                                }
                            }
                        );
                        break;
                    case protocol_version::v5:
                        epsp.send(
                            v5::pubrec_packet{
                                packet_id,
                                authorized ? pubrec_reason_code::success
                                           : pubrec_reason_code::not_authorized,
                                pubrec_props_
                            },
                            [epsp]
                            (system_error const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                        << ec.what();
                                }
                            }
                        );
                        break;
                    default:
                        BOOST_ASSERT(false);
                        break;
                    }
                    break;
                default:
                    break;
                }
            };

        // See if this session is authorized to publish this topic
        if (security_.auth_pub(topic, it->get_username()) != security::authorization::type::allow) {
            // Publish not authorized
            send_pubres(false);
            return;
        }

        properties forward_props;

        for (auto& prop : props) {
            prop.visit(
                overload {
                    [&](property::topic_alias&&) {
                        // TopicAlias is not forwarded
                        // https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901113
                        // A receiver MUST NOT carry forward any Topic Alias mappings from
                        // one Network Connection to another [MQTT-3.3.2-7].
                    },
                    [&](property::subscription_identifier&& p) {
                        ASYNC_MQTT_LOG("mqtt_broker", warning)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << "Subscription Identifier from client not forwarded sid:" << p.val();
                    },
                    [&](auto&& p) {
                        forward_props.push_back(force_move(p));
                    }
                },
                force_move(prop)
            );
        }

        do_publish(
            *it,
            force_move(topic),
            force_move(payload),
            opts.qos() | opts.retain(), // remove dup flag
            force_move(forward_props)
        );

        send_pubres();
    }

    /**
     * @brief do_publish Publish a message to any subscribed clients.
     *
     * @param source_ss - soource session_state.
     * @param topic - The topic to publish the message on.
     * @param payload - The payload of the message.
     * @param pubopts - publish options
     * @param props - properties
     */
    void do_publish(
        session_state<NextLayer...> const& source_ss,
        buffer topic,
        buffer payload,
        pub::opts opts,
        properties props
    ) {
        // Get auth rights for this topic
        // auth_users prepared once here, and then referred multiple times in subs_map_.modify() for efficiency
        auto auth_users = security_.auth_sub(topic);

        // publish the message to subscribers.
        // retain is delivered as the original only if rap_value is rap::retain.
        // On MQTT v3.1.1, rap_value is always rap::dont.
        auto deliver =
            [&] (session_state<NextLayer...>& ss, subscription<NextLayer...>& sub, auto const& auth_users) {

                // See if this session is authorized to subscribe this topic
                auto access = security_.auth_sub_user(auth_users, ss.get_username());
                if (access != security::authorization::type::allow) return;

                pub::opts new_opts = std::min(opts.qos(), sub.opts.qos());
                if (sub.opts.rap() == sub::rap::retain && opts.retain() == pub::retain::yes) {
                    new_opts |= pub::retain::yes;
                }

                if (sub.sid) {
                    props.push_back(property::subscription_identifier(*sub.sid));
                    ss.deliver(
                        timer_ioc_,
                        topic,
                        payload,
                        new_opts,
                        props
                    );
                    props.pop_back();
                }
                else {
                    ss.deliver(
                        timer_ioc_,
                        topic,
                        payload,
                        new_opts,
                        props
                    );
                }
            };

        //                  share_name   topic_filter
        std::set<std::tuple<string_view, string_view>> sent;

        {
            std::shared_lock<mutex> g{mtx_subs_map_};
            subs_map_.modify(
                topic,
                [&](buffer const& /*key*/, subscription<NextLayer...>& sub) {
                    if (sub.share_name.empty()) {
                        // Non shared subscriptions

                        // If NL (no local) subscription option is set and
                        // publisher is the same as subscriber, then skip it.
                        if (sub.opts.nl() == sub::nl::yes &&
                            sub.ss.get().client_id() ==  source_ss.client_id()) return;
                        deliver(sub.ss.get(), sub, auth_users);
                    }
                    else {
                        // Shared subscriptions
                        bool inserted;
                        std::tie(std::ignore, inserted) = sent.emplace(sub.share_name, sub.topic_filter);
                        if (inserted) {
                            if (auto ssr_opt = shared_targets_.get_target(sub.share_name, sub.topic_filter)) {
                                deliver(ssr_opt.value().get(), sub, auth_users);
                            }
                        }
                    }
                }
            );
        }

        optional<std::chrono::steady_clock::duration> message_expiry_interval;
        if (source_ss.get_protocol_version() == protocol_version::v5) {
            for (auto const& prop : props) {
                prop.visit(
                    overload {
                        [&](property::message_expiry_interval const& v) {
                            message_expiry_interval.emplace(std::chrono::seconds(v.val()));
                        },
                        [&](auto const&){}
                    }
                );
            }
        }

        /*
         * If the message is marked as being retained, then we
         * keep it in case a new subscription is added that matches
         * this topic.
         *
         * @note: The MQTT standard 3.3.1.3 RETAIN makes it clear that
         *        retained messages are global based on the topic, and
         *        are not scoped by the client id. So any client may
         *        publish a retained message on any topic, and the most
         *        recently published retained message on a particular
         *        topic is the message that is stored on the server.
         *
         * @note: The standard doesn't make it clear that publishing
         *        a message with zero length, but the retain flag not
         *        set, does not result in any existing retained message
         *        being removed. However, internet searching indicates
         *        that most brokers have opted to keep retained messages
         *        when receiving payload of zero bytes, unless the so
         *        received message has the retain flag set, in which case
         *        the retained message is removed.
         */
        if (opts.retain() == pub::retain::yes) {
            if (payload.empty()) {
                std::lock_guard<mutex> g(mtx_retains_);
                retains_.erase(topic);
            }
            else {
                std::shared_ptr<as::steady_timer> tim_message_expiry;
                if (message_expiry_interval) {
                    tim_message_expiry = std::make_shared<as::steady_timer>(timer_ioc_, message_expiry_interval.value());
                    tim_message_expiry->async_wait(
                        [this, topic = topic, wp = std::weak_ptr<as::steady_timer>(tim_message_expiry)]
                        (boost::system::error_code const& ec) {
                            if (auto sp = wp.lock()) {
                                if (!ec) {
                                    retains_.erase(topic);
                                }
                            }
                        }
                    );
                }

                std::lock_guard<mutex> g(mtx_retains_);
                retains_.insert_or_assign(
                    topic,
                    retain_t {
                        force_move(topic),
                        force_move(payload),
                        force_move(props),
                        opts.qos(),
                        tim_message_expiry
                    }
                );
            }
        }
    }

    void puback_handler(
        epsp_t epsp,
        packet_id_t packet_id,
        puback_reason_code /*reason_code*/,
        properties /*props*/
    ) {
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );

        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) return;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<NextLayer...>&>(*it);
        ss.erase_inflight_message_by_packet_id(packet_id);
        ss.send_offline_messages_by_packet_id_release();
    }

    void pubrec_handler(
        epsp_t epsp,
        packet_id_t packet_id,
        pubrec_reason_code reason_code,
        properties /*props*/
    ) {
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );

        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) return;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<NextLayer...>&>(*it);
        ss.erase_inflight_message_by_packet_id(packet_id);

        if (is_error(reason_code)) return;

        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1:
            epsp.send(
                v3_1_1::pubrel_packet{
                    packet_id
                },
                [epsp]
                (system_error const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.what();
                    }
                }
            );
            break;
        case protocol_version::v5:
            epsp.send(
                v5::pubrel_packet{
                    packet_id,
                    pubrel_reason_code::success,
                    pubrel_props_
                },
                [epsp]
                (system_error const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.what();
                    }
                }
            );
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void pubrel_handler(
        epsp_t epsp,
        packet_id_t packet_id,
        pubrel_reason_code reason_code,
        properties /*props*/
    ) {
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );

        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) return;

        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1:
            epsp.send(
                v3_1_1::pubcomp_packet{
                    packet_id
                },
                [epsp]
                (system_error const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.what();
                    }
                }
            );
            break;
        case protocol_version::v5:
            epsp.send(
                v5::pubcomp_packet{
                    packet_id,
                    static_cast<pubcomp_reason_code>(reason_code),
                    pubcomp_props_
                },
                [epsp]
                (system_error const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.what();
                    }
                }
            );
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void pubcomp_handler(
        epsp_t epsp,
        packet_id_t packet_id,
        pubcomp_reason_code /*reason_code*/,
        properties /*props*/
    ){
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );

        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) return;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<NextLayer...>&>(*it);
        ss.erase_inflight_message_by_packet_id(packet_id);
        ss.send_offline_messages_by_packet_id_release();
    }

    void subscribe_handler(
        epsp_t epsp,
        packet_id_t packet_id,
        std::vector<topic_subopts> const& entries,
        properties props
    ) {
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );

        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) return;

        // The element of sessions_ must have longer lifetime
        // than corresponding subscription.
        // Because the subscription store the reference of the element.
        optional<session_state_ref<NextLayer...>> ssr_opt;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<NextLayer...>&>(*it);
        ssr_opt.emplace(ss);

        BOOST_ASSERT(ssr_opt);
        session_state_ref<NextLayer...> ssr {*ssr_opt};

        auto publish_proc =
            [this, &ssr](retain_t const& r, qos qos_value, optional<std::size_t> sid) {
                auto props = r.props;
                if (sid) {
                    props.push_back(property::subscription_identifier(*sid));
                }
                if (r.tim_message_expiry) {
                    auto d =
                        std::chrono::duration_cast<std::chrono::seconds>(
                            r.tim_message_expiry->expiry() - std::chrono::steady_clock::now()
                        ).count();
                    for (auto& prop : props) {
                        prop.visit(
                            overload {
                                [&](property::message_expiry_interval& v) {
                                    v = property::message_expiry_interval(static_cast<uint32_t>(d));
                                },
                                [&](auto&) {}
                            }
                        );
                    }
                }
                ssr.get().publish(
                    timer_ioc_,
                    r.topic,
                    r.payload,
                    std::min(r.qos_value, qos_value) | pub::retain::yes,
                    props
                );
            };

        std::vector<std::function<void()>> retain_deliver;
        retain_deliver.reserve(entries.size());

        // subscription identifier
        optional<std::size_t> sid;

        // An in-order list of qos settings, used to send the reply.
        // The MQTT protocol 3.1.1 - 3.8.4 Response - paragraph 6
        // allows the server to grant a lower QOS than requested
        // So we reply with the QOS setting that was granted
        // not the one requested.
        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1: {
            std::vector<suback_return_code> res;
            res.reserve(entries.size());
            for (auto& e : entries) {
                if (security_.is_subscribe_authorized(ss.get_username(), e.topic())) {
                    res.emplace_back(qos_to_suback_return_code(e.opts().qos())); // converts to granted_qos_x
                    ssr.get().subscribe(
                        e.sharename(),
                        e.topic(),
                        e.opts(),
                        [&] {
                            std::shared_lock<mutex> g(mtx_retains_);
                            retains_.find(
                                e.topic(),
                                [&](retain_t const& r) {
                                    retain_deliver.emplace_back(
                                        [&publish_proc, &r, qos_value = e.opts().qos(), sid] {
                                            publish_proc(r, qos_value, sid);
                                        }
                                    );
                                }
                            );
                        }
                    );
                }
                else {
                    // User not authorized to subscribe to topic filter
                    res.emplace_back(suback_return_code::failure);
                }
            }
            // Acknowledge the subscriptions, and the registered QOS settings
            epsp.send(
                v3_1_1::suback_packet{
                    packet_id,
                    force_move(res)
                },
                [epsp]
                (system_error const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.what();
                    }
                }
            );
        } break;
        case protocol_version::v5: {
            // Get subscription identifier
            for (auto const& prop : props) {
                prop.visit(
                    overload {
                        [&](property::subscription_identifier const& v) {
                            // TBD error if 0
                            if (v.val() != 0) {
                                sid.emplace(v.val());
                            }
                        },
                        [&](auto const&) {}
                    }
                );
                if (sid) break;
            }

            std::vector<suback_reason_code> res;
            res.reserve(entries.size());
            for (auto& e : entries) {
                if (security_.is_subscribe_authorized(ss.get_username(), e.topic())) {
                    res.emplace_back(qos_to_suback_reason_code(e.opts().qos())); // converts to granted_qos_x
                    ssr.get().subscribe(
                        e.sharename(),
                        e.topic(),
                        e.opts(),
                        [&] {
                            std::shared_lock<mutex> g(mtx_retains_);
                            retains_.find(
                                e.topic(),
                                [&](retain_t const& r) {
                                    retain_deliver.emplace_back(
                                        [&publish_proc, &r, qos_value = e.opts().qos(), sid] {
                                            publish_proc(r, qos_value, sid);
                                        }
                                    );
                                }
                            );
                        },
                        sid
                    );
                }
                else {
                    // User not authorized to subscribe to topic filter
                    res.emplace_back(suback_reason_code::not_authorized);
                }
            }
            if (h_subscribe_props_) h_subscribe_props_(props);
            // Acknowledge the subscriptions, and the registered QOS settings
            epsp.send(
                v5::suback_packet{
                    packet_id,
                    force_move(res),
                    suback_props_
                },
                [epsp]
                (system_error const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.what();
                    }
                }
            );
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }

        for (auto const& f : retain_deliver) {
            f();
        }
    }

    void disconnect_handler(
        epsp_t epsp,
        disconnect_reason_code rc,
        properties props
    ) {
        if (delay_disconnect_) {
            tim_disconnect_.expires_after(*delay_disconnect_);
            tim_disconnect_.wait();
        }
        close_proc(
            force_move(epsp),
            rc == disconnect_reason_code::disconnect_with_will_message,
            force_move(props)
        );
    }


    /**
     * @brief close_proc_no_lock - clean up a connection that has been closed.
     *
     * @param ep - The underlying server (of whichever type) that is disconnecting.
     * @param send_will - Whether to publish this connections last will
     * @return true if offline session is remained, otherwise false
     */
    // TODO: Maybe change the name of this function.
    bool close_proc_no_lock(
        epsp_t epsp,
        bool send_will,
        optional<disconnect_reason_code> rc_opt) {

        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epwp_t{epsp});

        // act_sess_it == act_sess_idx.end() could happen if broker accepts
        // the session from client but the client closes the session  before sending
        // MQTT `CONNECT` message.
        // In this case, do nothing is correct behavior.
        if (it == idx.end()) return false;

        bool session_clear =
            [&] {
                if (epsp.get_protocol_version() == protocol_version::v3_1_1) {
                    return epsp.clean_session();
                }
                else {
                    BOOST_ASSERT(epsp.get_protocol_version() == protocol_version::v5);
                    auto const& sei_opt = it->session_expiry_interval();
                    return !sei_opt || *sei_opt == std::chrono::steady_clock::duration::zero();
                }
            } ();

        auto do_send_will =
            [&](session_state<NextLayer...>& ss) {
                if (send_will) {
                    ss.send_will();
                }
                else {
                    ss.clear_will();
                }
            };

        if (session_clear) {
            // const_cast is appropriate here
            // See https://github.com/boostorg/multi_index/issues/50
            auto& ss = const_cast<session_state<NextLayer...>&>(*it);
            do_send_will(ss);
            if (rc_opt) {
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << "disconnect_and_force_disconnect(async) cid:" << ss.client_id();
                disconnect_and_force_disconnect(epsp, *rc_opt);
            }
            else {
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << "force_disconnect(async) cid:" << ss.client_id();
                force_disconnect(epsp);
            }
            idx.erase(it);
            BOOST_ASSERT(sessions_.template get<tag_con>().find(epwp_t{epsp}) == sessions_.template get<tag_con>().end());
            return false;
        }
        else {
            idx.modify(
                it,
                [&](session_state<NextLayer...>& ss) {
                    do_send_will(ss);
                    if (rc_opt) {
                        ASYNC_MQTT_LOG("mqtt_broker", trace)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << "disconnect_and_force_disconnect(async) cid:" << ss.client_id();
                        disconnect_and_force_disconnect(epsp, *rc_opt);
                    }
                    else {
                        ASYNC_MQTT_LOG("mqtt_broker", trace)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << "force_disconnect(async) cid:" << ss.client_id();
                        force_disconnect(epsp);
                    }
                    // become_offline updates index
                    ss.become_offline(
                        [this]
                        (std::shared_ptr<as::steady_timer> const& sp_tim) {
                            sessions_.template get<tag_tim>().erase(sp_tim);
                        }
                    );
                },
                [](auto&) { BOOST_ASSERT(false); }
            );
            return true;
        }

    }

    /**
     * @brief close_proc - clean up a connection that has been closed.
     *
     * @param ep - The underlying server (of whichever type) that is disconnecting.
     * @param send_will - Whether to publish this connections last will
     * @param rc - Reason Code for send pack DISCONNECT
     * @return true if offline session is remained, otherwise false
     */
    // TODO: Maybe change the name of this function.
    bool close_proc(
        epsp_t epsp,
        bool send_will,
        optional<disconnect_reason_code> rc_opt = nullopt
    ) {
        std::lock_guard<mutex> g(mtx_sessions_);
        return close_proc_no_lock(force_move(epsp), send_will, rc_opt);
    }

private:

    as::io_context& timer_ioc_; ///< The boost asio context to run this broker on.
    as::steady_timer tim_disconnect_; ///< Used to delay disconnect handling for testing
    optional<std::chrono::steady_clock::duration> delay_disconnect_; ///< Used to delay disconnect handling for testing

    // Authorization and authentication settings
    security security_;

    mutable mutex mtx_subs_map_;
    sub_con_map<NextLayer...> subs_map_;   /// subscription information
    shared_target<NextLayer...> shared_targets_; /// shared subscription targets

    ///< Map of active client id and connections
    /// session_state has references of subs_map_ and shared_targets_.
    /// because session_state (member of sessions_) has references of subs_map_ and shared_targets_.
    mutable mutex mtx_sessions_;
    session_states<NextLayer...> sessions_;

    mutable mutex mtx_retains_;
    retained_messages retains_; ///< A list of messages retained so they can be sent to newly subscribed clients.

    // MQTTv5 members
    properties connack_props_;
    properties suback_props_;
    properties unsuback_props_;
    properties puback_props_;
    properties pubrec_props_;
    properties pubrel_props_;
    properties pubcomp_props_;
    std::function<void(properties const&)> h_connect_props_;
    std::function<void(properties const&)> h_disconnect_props_;
    std::function<void(properties const&)> h_publish_props_;
    std::function<void(properties const&)> h_puback_props_;
    std::function<void(properties const&)> h_pubrec_props_;
    std::function<void(properties const&)> h_pubrel_props_;
    std::function<void(properties const&)> h_pubcomp_props_;
    std::function<void(properties const&)> h_subscribe_props_;
    std::function<void(properties const&)> h_unsubscribe_props_;
    std::function<void(properties const&)> h_auth_props_;
    bool pingresp_ = true;
    bool connack_ = true;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_BROKER_HPP
