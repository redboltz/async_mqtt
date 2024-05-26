// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_BROKER_HPP)
#define ASYNC_MQTT_BROKER_BROKER_HPP

#include <async_mqtt/all.hpp>
#include <broker/endpoint_variant.hpp>
#include <broker/security.hpp>
#include <broker/mutex.hpp>
#include <broker/session_state.hpp>
#include <broker/sub_con_map.hpp>
#include <broker/retained_messages.hpp>
#include <broker/retained_topic_map.hpp>
#include <broker/shared_target_impl.hpp>
#include <broker/mutex.hpp>
#include <broker/uuid.hpp>

#include <broker/constant.hpp>
#include <broker/security.hpp>

namespace async_mqtt {


template <typename Epsp>
class broker {
    using epsp_type = epsp_wrap<Epsp>;
    using this_type = broker<Epsp>;

public:
    broker(as::io_context& timer_ioc)
        :timer_ioc_{timer_ioc},
         tim_disconnect_{timer_ioc_} {
        std::unique_lock<mutex> g_sec{mtx_security_};
        security_.default_config();
    }

    void handle_accept(epsp_type epsp, std::optional<std::string> preauthed_user_name = {}) {
        epsp.set_preauthed_user_name(force_move(preauthed_user_name));
        async_read_packet(force_move(epsp));
    }

    /**
     * @brief configure the security settings
     */
    void set_security(security&& sec) {
        std::unique_lock<mutex> g_sec{mtx_security_};
        security_ = force_move(sec);
    }

private:
    void async_read_packet(epsp_type epsp) {
        epsp.async_recv(
            [this, epsp]
            (error_code const& ec, packet_variant pv) mutable {
                if (ec) {
                    ASYNC_MQTT_LOG("mqtt_broker", info)
                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                        << ec.message();
                    close_proc(
                        force_move(epsp),
                        true // send_will
                    );
                    return;
                }
                BOOST_ASSERT(pv);
                pv.visit(
                    overload {
                        [&](v3_1_1::connect_packet& p) {
                            connect_handler(
                                force_move(epsp),
                                p.client_id(),
                                p.user_name(),
                                p.password(),
                                p.get_will(),
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
                                p.get_will(),
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
                                p.payload_as_buffer(),
                                properties{}
                            );
                        },
                        [&](v5::publish_packet& p) {
                            publish_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.opts(),
                                p.topic(),
                                p.payload_as_buffer(),
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
                        [&](v3_1_1::suback_packet&) {
                            // TBD receive invalid packet
                        },
                        [&](v5::suback_packet&) {
                            // TBD receive invalid packet
                        },
                        [&](v3_1_1::unsubscribe_packet& p) {
                            unsubscribe_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.entries(),
                                properties{}
                            );
                        },
                        [&](v5::unsubscribe_packet& p) {
                            unsubscribe_handler(
                                force_move(epsp),
                                p.packet_id(),
                                p.entries(),
                                p.props()
                            );
                        },
                        [&](v3_1_1::pingreq_packet&) {
                            pingreq_handler(
                                force_move(epsp)
                            );
                        },
                        [&](v5::pingreq_packet&) {
                            pingreq_handler(
                                force_move(epsp)
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
                        [&](v5::auth_packet& p) {
                            auth_handler(
                                force_move(epsp),
                                p.code(),
                                p.props()
                            );
                        },
                        [&](auto const&) {
                            ASYNC_MQTT_LOG("mqtt_broker", fatal)
                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                << "invalid variant";
                        }
                    }
                );
            }
        );
    }

    void connect_handler(
        epsp_type epsp,
        std::string client_id,
        std::optional<std::string> noauth_username,
        std::optional<std::string> password,
        std::optional<will> will,
        bool clean_start,
        std::uint16_t /*keep_alive*/,
        properties props
    ) {
        std::optional<std::string> username;
        if (auto paun_opt = epsp.get_preauthed_user_name()) {
            std::shared_lock<mutex> g_sec{mtx_security_};
            if (security_.login_cert(*paun_opt)) {
                username = force_move(*paun_opt);
            }
        }
        else if (!noauth_username && !password) {
            std::shared_lock<mutex> g_sec{mtx_security_};
            username = security_.login_anonymous();
        }
        else if (noauth_username && password) {
            std::shared_lock<mutex> g_sec{mtx_security_};
            username = security_.login(*noauth_username, *password);
        }

        // If login fails, try the unauthenticated user
        if (!username) {
            std::shared_lock<mutex> g_sec{mtx_security_};
            username = security_.login_unauthenticated();
        }

        std::optional<std::chrono::steady_clock::duration> session_expiry_interval;
        std::optional<std::chrono::steady_clock::duration> will_expiry_interval;
        bool response_topic_requested = false;

        auto version = epsp.get_protocol_version();
        if (version == protocol_version::v5) {
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
                        },
                        [&](auto const&) {}
                    }
                );
            }
            if (will) {
                for (auto const& prop : will->props()) {
                    prop.visit(
                        overload {
                            [&](property::message_expiry_interval const& v) {
                                will_expiry_interval.emplace(std::chrono::seconds(v.val()));
                            },
                            [&](auto const&) {}
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
                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                << "User failed to login: "
                << (noauth_username ? std::string(*noauth_username) : std::string("anonymous user"));

            send_connack(
                epsp,
                false, // session present
                false, // authenticated
                force_move(connack_props),
                [epsp, version](error_code) mutable {
                    disconnect_and_close(
                        epsp,
                        version,
                        disconnect_reason_code::not_authorized,
                        as::detached
                    );
                }
            );
            return;
        }

        if (client_id.empty()) {
            if (!handle_empty_client_id(epsp, client_id, clean_start, connack_props)) {
                return;
            }
            // A new client id was generated
            client_id = epsp.get_client_id();
        }

        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
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
            (*it)->client_id() != client_id ||
            (*it)->get_username() != *username
        ) {
            // new connection
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                << "cid:" << client_id
                << " new connection inserted.";
            it = idx.emplace_hint(
                it,
                session_state<epsp_type>::create(
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
                        this->do_publish(std::forward<decltype(params)>(params)...);
                    },
                    clean_start,
                    force_move(will_expiry_interval),
                    force_move(session_expiry_interval)
                )
            );
            if (response_topic_requested) {
                // set_response_topic never modify key part
                set_response_topic(const_cast<session_state<epsp_type>&>(**it), connack_props, *username);
            }

            send_connack(
                epsp,
                false, // session present
                true,  // authenticated
                force_move(connack_props),
                [this, epsp](error_code) mutable {
                    async_read_packet(force_move(epsp));
                }
            );
        }
        else if (auto old_epsp = const_cast<session_state<epsp_type>&>(**it).lock()) {
            // online overwrite
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                << "cid:" << client_id
                << " old connection " << old_epsp.get_address() << " exists and is online. close it ";
            close_proc_no_lock(
                old_epsp,
                true,
                disconnect_reason_code::session_taken_over,
                [
                    this,
                    epsp,
                    &idx,
                    it,
                    connack_props = force_move(connack_props),
                    clean_start,
                    client_id = force_move(client_id),
                    response_topic_requested,
                    username = force_move(username),
                    will = force_move(will),
                    will_expiry_interval,
                    session_expiry_interval
                ]
                (bool remain_as_offline) mutable {
                    if (remain_as_offline) {
                        // offline exists -> online
                        offline_to_online(
                            force_move(epsp),
                            force_move(will),
                            force_move(will_expiry_interval),
                            force_move(session_expiry_interval),
                            clean_start,
                            force_move(*username),
                            idx,
                            it,
                            response_topic_requested,
                            force_move(connack_props)
                        );
                    }
                    else {
                        // new connection
                        ASYNC_MQTT_LOG("mqtt_broker", trace)
                            << ASYNC_MQTT_ADD_VALUE(address, this)
                            << "cid:" << client_id
                            << "online connection exists, discard old one due to session_expiry and renew";
                        bool inserted;
                        std::tie(it, inserted) = idx.emplace(
                            session_state<epsp_type>::create(
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
                                    this->do_publish(std::forward<decltype(params)>(params)...);
                                },
                                clean_start,
                                force_move(will_expiry_interval),
                                force_move(session_expiry_interval)
                            )
                        );
                        BOOST_ASSERT(inserted);
                        if (response_topic_requested) {
                            // set_response_topic never modify key part
                            set_response_topic(const_cast<session_state<epsp_type>&>(**it), connack_props, *username);
                        }
                        send_connack(
                            epsp,
                            false, // session present
                            true,  // authenticated
                            force_move(connack_props),
                            [this, epsp](error_code) mutable {
                                async_read_packet(force_move(epsp));
                            }
                        );
                    }
                }
            );
        }
        else {
            // offline exists -> online
            offline_to_online(
                force_move(epsp),
                force_move(will),
                force_move(will_expiry_interval),
                force_move(session_expiry_interval),
                clean_start,
                force_move(*username),
                idx,
                it,
                response_topic_requested,
                force_move(connack_props)
            );
        }
    }

    template <typename Idx, typename It>
    void offline_to_online(
        epsp_type epsp,
        std::optional<will> will,
        std::optional<std::chrono::steady_clock::duration> will_expiry_interval,
        std::optional<std::chrono::steady_clock::duration> session_expiry_interval,
        bool clean_start,
        std::string username,
        Idx& idx,
        It it,
        bool response_topic_requested,
        properties connack_props
    ) {
        if (clean_start) {
            // discard offline session
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "un:" << username
                << "offline connection exists, discard old one due to new one's clean_start and renew";
            idx.modify(
                it,
                [&](auto& e) {
                    e->renew(
                        epsp,
                        will,
                        clean_start,
                        will_expiry_interval,
                        session_expiry_interval
                    );
                },
                [](auto&) { BOOST_ASSERT(false); }
            );
            if (response_topic_requested) {
                // set_response_topic never modify key part
                set_response_topic(const_cast<session_state<epsp_type>&>(**it), connack_props, username);
            }
            send_connack(
                epsp,
                false, // session present
                true,  // authenticated
                force_move(connack_props),
                [
                    this,
                    epsp
                ](error_code) mutable {
                    async_read_packet(force_move(epsp));
                }
            );
        }
        else {
            // inherit online session if previous session's session exists
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "un:" << username
                << "offline connection exists, and inherit it";
            if (response_topic_requested) {
                // set_response_topic never modify key part
                set_response_topic(const_cast<session_state<epsp_type>&>(**it), connack_props, username);
            }

            epsp.dispatch(
                [
                    this,
                    epsp,
                    &idx,
                    it,
                    username = force_move(username),
                    will = force_move(will),
                    will_expiry_interval,
                    session_expiry_interval,
                    connack_props = force_move(connack_props)
                ]
                () mutable {
                    idx.modify(
                        it,
                        [&](auto& e) {
                            e->inherit(
                                epsp,
                                force_move(will),
                                will_expiry_interval,
                                force_move(session_expiry_interval)
                            );
                        },
                        [](auto&) { BOOST_ASSERT(false); }
                    );
                    send_connack(
                        epsp,
                        true, // session present
                        true, // authenticated
                        force_move(connack_props),
                        [
                            this,
                            epsp,
                            &idx,
                            it
                        ]
                        (error_code const& ec) mutable {
                            if (ec) {
                                ASYNC_MQTT_LOG("mqtt_broker", trace)
                                    << ASYNC_MQTT_ADD_VALUE(address, this)
                                    << ec.message();
                                return;
                            }
                            idx.modify(
                                it,
                                [&](auto& e) {
                                    e->send_inflight_messages();
                                    e->send_all_offline_messages();
                                },
                                [](auto&) { BOOST_ASSERT(false); }
                            );
                            async_read_packet(force_move(epsp));
                        }
                    );
                }
            );
        }
    }

    void set_response_topic(
        session_state<epsp_type>& s,
        properties& connack_props,
        std::string const &username
    ) {
        auto response_topic =
            [&] {
                if (auto rt_opt = s.get_response_topic()) {
                    return *rt_opt;
                }
                auto rt = create_uuid_string();
                s.set_response_topic(rt);
                return rt;
            } ();

        auto rule_nr =
            [&] {
                std::unique_lock<mutex> g_sec{mtx_security_};
                return security_.add_auth(
                    response_topic,
                    { "@any" }, security::authorization::type::allow,
                    { username }, security::authorization::type::allow
                );
            } ();

        s.set_clean_handler(
            [this, response_topic, rule_nr]() {
                std::lock_guard<mutex> g(mtx_retains_);
                retains_.erase(response_topic);
                std::unique_lock<mutex> g_sec{mtx_security_};
                security_.remove_auth(rule_nr);
            }
        );

        connack_props.emplace_back(
            property::response_information{
                force_move(response_topic)
            }
        );
    }

    bool handle_empty_client_id(
        epsp_type& epsp,
        std::string const& client_id,
        bool clean_start,
        properties& connack_props
    ) {
        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1:
            if (client_id.empty()) {
                if (clean_start) {
                    epsp.set_client_id(create_uuid_string());
                }
                else {
                    // https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349242
                    // If the Client supplies a zero-byte ClientId,
                    // the Client MUST also set CleanSession to 1 [MQTT-3.1.3-7].
                    // If it's a not a clean session, but no client id is provided,
                    // we would have no way to map this connection's session to a new connection later.
                    // So the connection must be rejected.
                    if (connack_) {
                        epsp.async_send(
                            v3_1_1::connack_packet{
                                false,
                                connect_return_code::identifier_rejected
                            },
                            [epsp]
                            (error_code const& ec) mutable {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                        << ec.message();
                                }
                                epsp.async_close(
                                    as::bind_executor(
                                        epsp.get_executor(),
                                        [epsp] {
                                            ASYNC_MQTT_LOG("mqtt_broker", info)
                                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                                << "closed";
                                        }
                                    )
                                );
                            }
                        );
                    }
                    return false;
                }
            }
            break;
        case protocol_version::v5:
            if (client_id.empty()) {
                // https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901059
                //  A Server MAY allow a Client to supply a ClientID that has a length of zero bytes,
                // however if it does so the Server MUST treat this as a special case and assign a
                // unique ClientID to that Client [MQTT-3.1.3-6]. It MUST then process the
                // CONNECT packet as if the Client had provided that unique ClientID,
                // and MUST return the Assigned Client Identifier in the CONNACK packet [MQTT-3.1.3-7].
                // If the Server rejects the ClientID it MAY respond to the CONNECT packet with a CONNACK
                // using Reason Code 0x85 (Client Identifier not valid) as described in section 4.13
                // Handling errors, and then it MUST close the Network Connection [MQTT-3.1.3-8].
                //
                // mqtt_cpp author's note: On v5.0, no Clean Start restriction is described.
                epsp.set_client_id(create_uuid_string());
                connack_props.emplace_back(
                    property::assigned_client_identifier{std::string{epsp.get_client_id()}}
                );
            }
            break;
        default:
            BOOST_ASSERT(false);
            return false;
        }
        return true;
    }

    struct send_connack_op {
        this_type& brk;
        epsp_type epsp;
        bool session_present;
        bool authenticated;
        properties props;

        template <typename Self>
        void operator()(Self& self) {
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                << "send_connack";
            // Reply to the connect message.
            switch (epsp.get_protocol_version()) {
            case protocol_version::v3_1_1:
                if (brk.connack_) {
                    epsp.async_send(
                        v3_1_1::connack_packet{
                            session_present,
                            authenticated ? connect_return_code::accepted
                                          : connect_return_code::not_authorized,
                        },
                        force_move(self)
                    );
                }
                break;
            case protocol_version::v5:
                // connack_props_ member varible is for testing
                if (brk.connack_props_.empty()) {
                    // props local variable is is for real case
                    props.emplace_back(property::topic_alias_maximum{topic_alias_max});
                    props.emplace_back(property::receive_maximum{receive_maximum_max});
                    if (brk.connack_) {
                        epsp.async_send(
                            v5::connack_packet{
                                session_present,
                                authenticated ? connect_reason_code::success
                                              : connect_reason_code::not_authorized,
                                force_move(props)
                            },
                            force_move(self)
                        );
                    }
                }
                else {
                    // use connack_props_ for testing
                    if (brk.connack_) {
                        epsp.async_send(
                            v5::connack_packet{
                                session_present,
                                authenticated ? connect_reason_code::success
                                              : connect_reason_code::not_authorized,
                                brk.connack_props_
                            },
                            force_move(self)
                        );
                    }
                }
                break;
            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename Self>
        void operator()(Self& self, error_code se) {
            self.complete(se);
        }
    };

    template <typename CompletionToken>
    auto send_connack(
        epsp_type& epsp,
        bool session_present,
        bool authenticated,
        properties props,
        CompletionToken&& token
    ) {
        auto exe = epsp.get_executor();
        return as::async_compose<
            CompletionToken,
            void(error_code const&)
        >(
            send_connack_op{
                *this,
                force_move(epsp),
                session_present,
                authenticated,
                force_move(props)
            },
            token,
            exe
        );
    }

    void publish_handler(
        epsp_type epsp,
        packet_id_type packet_id,
        pub::opts opts,
        std::string topic,
        std::vector<buffer> payload,
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
            [&] (bool authorized, bool matched) {
                switch (opts.get_qos()) {
                case qos::at_least_once:
                    switch (epsp.get_protocol_version()) {
                    case protocol_version::v3_1_1:
                        epsp.async_send(
                            v3_1_1::puback_packet{
                                packet_id
                            },
                            [epsp]
                            (error_code const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                    << ec.message();
                                }
                            }
                        );
                        break;
                    case protocol_version::v5: {
                        auto packet =
                            [&] {
                                if (authorized) {
                                    if (puback_props_.empty()) {
                                        if (matched) {
                                            return v5::puback_packet{packet_id};
                                        }
                                        else {
                                            return v5::puback_packet{
                                                packet_id,
                                                puback_reason_code::no_matching_subscribers
                                            };
                                        }
                                    }
                                    else {
                                        if (matched) {
                                            return v5::puback_packet{
                                                packet_id,
                                                puback_reason_code::success,
                                                puback_props_
                                            };
                                        }
                                        else {
                                            return v5::puback_packet{
                                                packet_id,
                                                puback_reason_code::no_matching_subscribers,
                                                puback_props_
                                            };
                                        }
                                    };
                                }
                                else {
                                    if (puback_props_.empty()) {
                                        return v5::puback_packet{
                                            packet_id,
                                            puback_reason_code::not_authorized
                                        };
                                    }
                                    else {
                                        return v5::puback_packet{
                                            packet_id,
                                            puback_reason_code::not_authorized,
                                            puback_props_
                                        };
                                    }
                                }
                            } ();
                        epsp.async_send(
                            force_move(packet),
                            [epsp]
                            (error_code const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                        << ec.message();
                                }
                            }
                        );
                    } break;
                    default:
                        BOOST_ASSERT(false);
                        break;
                    }
                    break;
                case qos::exactly_once:
                    switch (epsp.get_protocol_version()) {
                    case protocol_version::v3_1_1:
                        epsp.async_send(
                            v3_1_1::pubrec_packet{
                                packet_id
                            },
                            [epsp]
                            (error_code const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                        << ec.message();
                                }
                            }
                        );
                        break;
                    case protocol_version::v5: {
                        auto packet =
                            [&] {
                                if (authorized) {
                                    if (pubrec_props_.empty()) {
                                        if (matched) {
                                            return v5::pubrec_packet{packet_id};
                                        }
                                        else {
                                            return v5::pubrec_packet{
                                                packet_id,
                                                pubrec_reason_code::no_matching_subscribers
                                            };
                                        }
                                    }
                                    else {
                                        if (matched) {
                                            return v5::pubrec_packet{
                                                packet_id,
                                                pubrec_reason_code::success,
                                                pubrec_props_
                                            };
                                        }
                                        else {
                                            return v5::pubrec_packet{
                                                packet_id,
                                                pubrec_reason_code::no_matching_subscribers,
                                                pubrec_props_
                                            };
                                        }
                                    };
                                }
                                else {
                                    if (pubrec_props_.empty()) {
                                        return v5::pubrec_packet{
                                            packet_id,
                                            pubrec_reason_code::not_authorized
                                        };
                                    }
                                    else {
                                        return v5::pubrec_packet{
                                            packet_id,
                                            pubrec_reason_code::not_authorized,
                                            pubrec_props_
                                        };
                                    }
                                }
                            } ();
                        epsp.async_send(
                            force_move(packet),
                            [epsp]
                            (error_code const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                        << ec.message();
                                }
                            }
                        );
                    } break;
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
        if ([&] {
                std::shared_lock<mutex> g_sec{mtx_security_};
                return security_.auth_pub(topic, (*it)->get_username()) != security::authorization::type::allow;
            } ()
        ) {
            // Publish not authorized
            send_pubres(false, false);
            return;
        }

        properties forward_props;

        for (auto&& prop : props) {
            force_move(prop).visit(
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
                }
            );
        }

        bool matched = do_publish(
            **it,
            force_move(topic),
            force_move(payload),
            opts.get_qos() | opts.get_retain(), // remove dup flag
            force_move(forward_props)
        );

        send_pubres(true, matched);
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
    bool do_publish(
        session_state<epsp_type> const& source_ss,
        std::string topic,
        std::vector<buffer> payload,
        pub::opts opts,
        properties props
    ) {
        bool matched = false;

        // Get auth rights for this topic
        // auth_users prepared once here, and then referred multiple times in subs_map_.modify() for efficiency
        auto auth_users =
            [&] {
                std::shared_lock<mutex> g_sec{mtx_security_};
                return security_.auth_sub(topic);
            } ();

        // publish the message to subscribers.
        // retain is delivered as the original only if rap_value is rap::retain.
        // On MQTT v3.1.1, rap_value is always rap::dont.
        auto deliver =
            [&] (session_state<epsp_type>& ss, subscription<epsp_type>& sub, auto const& auth_users) {

                // See if this session is authorized to subscribe this topic
                {
                    std::shared_lock<mutex> g_sec{mtx_security_};
                    auto access = security_.auth_sub_user(auth_users, ss.get_username());
                    if (access != security::authorization::type::allow) return false;
                }
                pub::opts new_opts = std::min(opts.get_qos(), sub.opts.get_qos());
                if (sub.opts.get_rap() == sub::rap::retain && opts.get_retain() == pub::retain::yes) {
                    new_opts |= pub::retain::yes;
                }

                if (sub.sid) {
                    props.push_back(property::subscription_identifier(boost::numeric_cast<std::uint32_t>(*sub.sid)));
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
                return true;
            };

        //                  share_name   topic_filter
        std::set<std::tuple<std::string_view, std::string_view>> sent;

        {
            std::shared_lock<mutex> g{mtx_subs_map_};
            subs_map_.modify(
                topic,
                [&](std::string const& /*key*/, subscription<epsp_type>& sub) {
                    if (sub.sharename.empty()) {
                        // Non shared subscriptions

                        // If NL (no local) subscription option is set and
                        // publisher is the same as subscriber, then skip it.
                        if (sub.opts.get_nl() == sub::nl::yes &&
                            sub.ss.get().client_id() ==  source_ss.client_id()) return;
                        if (deliver(sub.ss.get(), sub, auth_users)) matched = true;
                    }
                    else {
                        // Shared subscriptions
                        bool inserted;
                        std::tie(std::ignore, inserted) = sent.emplace(sub.sharename, sub.topic);
                        if (inserted) {
                            if (auto ssr_sub_opt = shared_targets_.get_target(sub.sharename, sub.topic)) {
                                auto [ssr, sub] = *ssr_sub_opt;
                                if (deliver(ssr.get(), sub, auth_users)) matched = true;
                            }
                        }
                    }
                }
            );
        }

        std::optional<std::chrono::steady_clock::duration> message_expiry_interval;
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
        if (opts.get_retain() == pub::retain::yes) {
            if (payload.empty()) {
                std::lock_guard<mutex> g(mtx_retains_);
                retains_.erase(topic);
            }
            else {
                std::shared_ptr<as::steady_timer> tim_message_expiry;
                if (message_expiry_interval) {
                    tim_message_expiry = std::make_shared<as::steady_timer>(timer_ioc_, *message_expiry_interval);
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
                    retain_type {
                        topic,
                        force_move(payload),
                        force_move(props),
                        opts.get_qos(),
                        tim_message_expiry
                    }
                );
            }
        }
        return matched;
    }

    void puback_handler(
        epsp_type epsp,
        packet_id_type packet_id,
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
        auto& ss = const_cast<session_state<epsp_type>&>(**it);
        ss.erase_inflight_message_by_packet_id(packet_id);
        ss.send_offline_messages_by_packet_id_release();
    }

    void pubrec_handler(
        epsp_type epsp,
        packet_id_type packet_id,
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
        auto& ss = const_cast<session_state<epsp_type>&>(**it);

        if (make_error_code(reason_code)) return;
        auto rc =
            [&] {
                ss.erase_inflight_message_by_packet_id(packet_id);
                if (!epsp.is_publish_processing(packet_id)) {
                    return pubrel_reason_code::packet_identifier_not_found;
                }
                else {
                    return pubrel_reason_code::success;
                }
            } ();

        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1:
            epsp.async_send(
                v3_1_1::pubrel_packet{
                    packet_id
                },
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
                    }
                }
            );
            break;
        case protocol_version::v5: {
            auto packet =
                [&] {
                    if (rc == pubrel_reason_code::success) {
                        if (pubrel_props_.empty()) {
                            return v5::pubrel_packet{
                                packet_id
                            };
                        }
                        else {
                            return v5::pubrel_packet{
                                packet_id,
                                rc,
                                pubrel_props_
                            };
                        }
                    }
                    else {
                        if (pubrel_props_.empty()) {
                            return v5::pubrel_packet{
                                packet_id,
                                rc
                            };
                        }
                        else {
                            return v5::pubrel_packet{
                                packet_id,
                                rc,
                                pubrel_props_
                            };
                        }
                    }
                } ();
            epsp.async_send(
                force_move(packet),
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
                    }
                }
            );
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void pubrel_handler(
        epsp_type epsp,
        packet_id_type packet_id,
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
            epsp.async_send(
                v3_1_1::pubcomp_packet{
                    packet_id
                },
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
                    }
                }
            );
            break;
        case protocol_version::v5: {
            auto packet =
                [&] {
                    if (reason_code == pubrel_reason_code::success) {
                        if (pubcomp_props_.empty()) {
                            return v5::pubcomp_packet{packet_id};
                        }
                        else {
                            return v5::pubcomp_packet{
                                packet_id,
                                pubcomp_reason_code::success,
                                pubcomp_props_
                            };
                        }
                    }
                    else {
                        if (pubcomp_props_.empty()) {
                            return v5::pubcomp_packet{
                                packet_id,
                                static_cast<pubcomp_reason_code>(reason_code)
                            };
                        }
                        else {
                            return v5::pubcomp_packet{
                                packet_id,
                                static_cast<pubcomp_reason_code>(reason_code),
                                pubcomp_props_
                            };
                        }
                    }
                } ();
            epsp.async_send(
                force_move(packet),
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
                    }
                }
            );
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void pubcomp_handler(
        epsp_type epsp,
        packet_id_type packet_id,
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
        auto& ss = const_cast<session_state<epsp_type>&>(**it);
        ss.erase_inflight_message_by_packet_id(packet_id);
        ss.send_offline_messages_by_packet_id_release();
    }

    void subscribe_handler(
        epsp_type epsp,
        packet_id_type packet_id,
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
        std::optional<session_state_ref<epsp_type>> ssr_opt;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<epsp_type>&>(**it);
        ssr_opt.emplace(ss);

        BOOST_ASSERT(ssr_opt);
        session_state_ref<epsp_type> ssr {*ssr_opt};

        auto publish_proc =
            [this, &ssr, &epsp](retain_type const& r, qos qos_value, std::optional<std::size_t> sid) {
                auto props = r.props;
                if (sid) {
                    props.push_back(property::subscription_identifier(std::uint32_t(*sid)));
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
                    epsp,
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
        std::optional<std::size_t> sid;

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
                if (!e ||
                    [&] {
                        std::shared_lock<mutex> g_sec{mtx_security_};
                        return security_.is_subscribe_authorized(ss.get_username(), e.topic());
                    } ()
                ) {
                    res.emplace_back(qos_to_suback_return_code(e.opts().get_qos())); // converts to granted_qos_x
                    ssr.get().subscribe(
                        e.sharename(),
                        e.topic(),
                        e.opts(),
                        [&] {
                            std::shared_lock<mutex> g(mtx_retains_);
                            retains_.find(
                                e.topic(),
                                [&](retain_type const& r) {
                                    retain_deliver.emplace_back(
                                        [&publish_proc, &r, qos_value = e.opts().get_qos(), sid] {
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
            epsp.async_send(
                v3_1_1::suback_packet{
                    packet_id,
                    force_move(res)
                },
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
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
                if (e) {
                    if ([&] {
                            std::shared_lock<mutex> g_sec{mtx_security_};
                            return security_.is_subscribe_authorized(ss.get_username(), e.topic());
                        } ()
                    ) {
                        res.emplace_back(qos_to_suback_reason_code(e.opts().get_qos())); // converts to granted_qos_x
                        ssr.get().subscribe(
                            e.sharename(),
                            e.topic(),
                            e.opts(),
                            [&] {
                                std::shared_lock<mutex> g(mtx_retains_);
                                retains_.find(
                                    e.topic(),
                                    [&](retain_type const& r) {
                                        retain_deliver.emplace_back(
                                            [&publish_proc, &r, qos_value = e.opts().get_qos(), sid] {
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
                else {
                    res.emplace_back(suback_reason_code::topic_filter_invalid);
                }
            }
            if (h_subscribe_props_) h_subscribe_props_(props);
            // Acknowledge the subscriptions, and the registered QOS settings
            epsp.async_send(
                v5::suback_packet{
                    packet_id,
                    force_move(res),
                    suback_props_
                },
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
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

    void unsubscribe_handler(
        epsp_type epsp,
        packet_id_type packet_id,
        std::vector<topic_sharename> entries,
        properties props
    ) {
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );


        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it  = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) return;

        // The element of sessions_ must have longer lifetime
        // than corresponding subscription.
        // Because the subscription store the reference of the element.
        std::optional<session_state_ref<epsp_type>> ssr_opt;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<epsp_type>&>(**it);
        ssr_opt.emplace(ss);

        BOOST_ASSERT(ssr_opt);
        session_state_ref<epsp_type> ssr {*ssr_opt};

        // For each subscription that this connection has
        // Compare against the list of topic filters, and remove
        // the subscription if the topic filter is in the list.
        for (auto const& e : entries) {
            ssr.get().unsubscribe(e.sharename(), e.topic());
        }

        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1:
            epsp.async_send(
                v3_1_1::unsuback_packet{
                    packet_id
                },
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
                    }
                }
            );
            break;
        case protocol_version::v5:
            if (h_unsubscribe_props_) h_unsubscribe_props_(props);
            epsp.async_send(
                v5::unsuback_packet{
                    packet_id,
                    std::vector<unsuback_reason_code>(
                        entries.size(),
                        unsuback_reason_code::success
                    ),
                    unsuback_props_
                },
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
                    }
                }
            );
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void pingreq_handler(
        epsp_type epsp
    ) {
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );

        if (!pingresp_) return;

        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1:
            epsp.async_send(
                v3_1_1::pingresp_packet{},
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
                    }
                }
            );
            break;
        case protocol_version::v5:
            epsp.async_send(
                v5::pingresp_packet{},
                [epsp]
                (error_code const& ec) {
                    if (ec) {
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << ec.message();
                    }
                }
            );
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    void disconnect_handler(
        epsp_type epsp,
        disconnect_reason_code rc,
        properties /*props*/
    ) {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
            << "disconnect_handler";
        if (delay_disconnect_) {
            tim_disconnect_.expires_after(*delay_disconnect_);
            tim_disconnect_.wait();
        }
        close_proc(
            force_move(epsp),
            rc == disconnect_reason_code::disconnect_with_will_message,
            rc
        );
    }

    struct close_proc_no_lock_op {
        close_proc_no_lock_op(
            this_type& brk,
            epsp_type epsp,
            bool send_will,
            std::optional<disconnect_reason_code> rc_opt
        ) :
            brk{brk},
            epsp{force_move(epsp)},
            send_will{send_will},
            rc_opt{rc_opt}
        {}

        this_type& brk;
        epsp_type epsp;
        bool send_will;
        std::optional<disconnect_reason_code> rc_opt;
        enum {close, complete} state = close;

        template <typename Self>
        void operator()(Self& self) {
            auto do_send_will =
                [&](session_state<epsp_type>& ss) {
                    if (send_will) {
                        ss.send_will();
                    }
                    else {
                        ss.clear_will();
                    }
                };
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                << "close_proc_no_lock";

            auto& idx = brk.sessions_.template get<tag_con>();
            auto it = idx.find(epsp);

            // act_sess_it == act_sess_idx.end() could happen if broker accepts
            // the session from client but the client closes the session  before sending
            // MQTT `CONNECT` message.
            // In this case, do nothing is correct behavior.
            if (it == idx.end()) {
                self.complete(false);
                return;
            }


            if ((*it)->remain_after_close()) {
                idx.modify(
                    it,
                    [&](std::shared_ptr<session_state<epsp_type>>& sssp) {
                        do_send_will(*sssp);
                        if (rc_opt) {
                            ASYNC_MQTT_LOG("mqtt_broker", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                << "disconnect_and_close() cid:" << sssp->client_id();
                            auto a_epsp{epsp};
                            disconnect_and_close(
                                a_epsp,
                                (*it)->get_protocol_version(),
                                *rc_opt,
                                as::append(
                                    force_move(self),
                                    sssp
                                )
                            );
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_broker", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                << "close cid:" << sssp->client_id();
                            auto a_epsp{epsp};
                            a_epsp.async_close(
                                as::append(
                                    force_move(self),
                                    sssp
                                )
                            );
                        }
                    }
                );
            }
            else {
                auto sssp{force_move(idx.extract(it).value())};
                do_send_will(*sssp);
                if (rc_opt) {
                    ASYNC_MQTT_LOG("mqtt_broker", trace)
                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                        << "disconnect_and_close() cid:" << sssp->client_id();
                    auto a_epsp{epsp};
                    disconnect_and_close(
                        a_epsp,
                        sssp->get_protocol_version(),
                        *rc_opt,
                        as::consign(
                            as::append(
                                force_move(self),
                                nullptr
                            ),
                            sssp
                        )
                    );
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_broker", trace)
                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                        << "close cid:" << sssp->client_id();
                    auto a_epsp{epsp};
                    a_epsp.async_close(
                        as::consign(
                            as::append(
                                force_move(self),
                                nullptr
                            ),
                            sssp
                        )
                    );
                }
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            std::shared_ptr<session_state<epsp_type>> sssp
        ) {
            ASYNC_MQTT_LOG("mqtt_broker", info)
                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                << "disconnect(optional)_and_closed";
            if (sssp) {
                // sessions_ index is never changed because owner_less
                // remains original order even if shared_count would be zero
                sssp->become_offline(
                    epsp,
                    [&brk = this->brk]
                    (std::shared_ptr<as::steady_timer> const& sp_tim) {
                        // lock for expire (async)
                        std::lock_guard<mutex> g(brk.mtx_sessions_);
                        brk.sessions_.template get<tag_tim>().erase(sp_tim);
                    }
                );
                self.complete(true);
            }
            else {
                self.complete(false);
            }
        }
    };

    /**
     * @brief close_proc_no_lock - clean up a connection that has been closed.
     *
     * @param ep - The underlying server (of whichever type) that is disconnecting.
     * @param send_will - Whether to publish this connections last will
     * @return true if offline session is remained, otherwise false
     */
    // TODO: Maybe change the name of this function.
    template <typename CompletionToken>
    auto close_proc_no_lock(
        epsp_type epsp,
        bool send_will,
        std::optional<disconnect_reason_code> rc_opt,
        CompletionToken&& token) {

        auto exe = epsp.get_executor();
        return as::async_compose<
            CompletionToken,
            void(bool)
        >(
            close_proc_no_lock_op{
                *this,
                force_move(epsp),
                send_will,
                force_move(rc_opt)
            },
            token,
            exe
        );
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
    void close_proc(
        epsp_type epsp,
        bool send_will,
        std::optional<disconnect_reason_code> rc_opt = std::nullopt
    ) {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
            << "close_proc";
        std::lock_guard<mutex> g(mtx_sessions_);
        close_proc_no_lock(force_move(epsp), send_will, rc_opt, [](bool){});
    }

    void auth_handler(
        epsp_type epsp,
        auth_reason_code /*rc*/,
        properties props
    ) {
        auto usg = unique_scope_guard(
            [&] {
                async_read_packet(force_move(epsp));
            }
        );

        if (h_auth_props_) h_auth_props_(force_move(props));
    }

    struct disconnect_and_close_op {
        disconnect_and_close_op(
            epsp_type epsp,
            protocol_version version,
            disconnect_reason_code rc
        ):
            epsp{force_move(epsp)},
            version{version},
            rc{rc},
            state{
                [&] {
                    if (version == protocol_version::v3_1_1) {
                        return close;
                    }
                    else {
                        return disconnect;
                    }
                }()
            }
        {
        }

        epsp_type epsp;
        protocol_version version;
        disconnect_reason_code rc;
        enum {disconnect, close, complete} state;

        template <typename Self>
        void operator()(
            Self& self,
            error_code = {}) {
            switch (state) {
            case disconnect: {
                state = close;
                auto a_epsp{epsp};
                a_epsp.async_send(
                    v5::disconnect_packet{
                        rc,
                        properties{}
                    },
                    force_move(self)
                );
            } break;
            case close: {
                state = complete;
                auto a_epsp{epsp};
                a_epsp.async_close(
                    force_move(self)
                );
            } break;
            case complete:
                ASYNC_MQTT_LOG("mqtt_broker", info)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << "closed";
                self.complete();
                break;
            }
        }
    };

    template <typename CompletionToken>
    static auto disconnect_and_close(
        epsp_type epsp,
        protocol_version version,
        disconnect_reason_code rc,
        CompletionToken&& token
    ) {
        auto exe = epsp.get_executor();
        return as::async_compose<
            CompletionToken,
            void()
        >(
            disconnect_and_close_op{
                force_move(epsp),
                version,
                rc
            },
            token,
            exe
        );
    }

private:

    as::io_context& timer_ioc_; ///< The boost asio context to run this broker on.
    as::steady_timer tim_disconnect_; ///< Used to delay disconnect handling for testing
    std::optional<std::chrono::steady_clock::duration> delay_disconnect_; ///< Used to delay disconnect handling for testing

    // Authorization and authentication settings
    mutable mutex mtx_security_;
    security security_;

    mutable mutex mtx_subs_map_;
    sub_con_map<epsp_type> subs_map_;   ///< subscription information
    shared_target<epsp_type> shared_targets_; ///< shared subscription targets

    ///< Map of active client id and connections
    /// session_state has references of subs_map_ and shared_targets_.
    /// because session_state (member of sessions_) has references of subs_map_ and shared_targets_.
    mutable mutex mtx_sessions_;
    session_states<epsp_type> sessions_;

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
