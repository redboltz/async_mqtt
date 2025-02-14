// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_BROKER_HPP)
#define ASYNC_MQTT_BROKER_BROKER_HPP

#include <boost/asio/experimental/parallel_group.hpp>

#include <async_mqtt/all.hpp>
#include <broker/endpoint_variant.hpp>
#include <broker/security.hpp>
#include <broker/mutex.hpp>
#include <coro_broker/session_state.hpp>
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
    broker(bool recycling_allocator = false)
        :recycling_allocator_{recycling_allocator} {
        std::unique_lock<mutex> g_sec{mtx_security_};
        security_.default_config();
    }

    as::awaitable<void>
    handle_accept(
        epsp_type epsp,
        std::optional<std::string> preauthed_user_name = {}
    ) {
        epsp.set_preauthed_user_name(force_move(preauthed_user_name));
        co_await recv_loop(force_move(epsp));
    }

    /**
     * @brief configure the security settings
     */
    void set_security(security&& sec) {
        std::unique_lock<mutex> g_sec{mtx_security_};
        security_ = force_move(sec);
    }

private:
    as::awaitable<void>
    recv_loop(epsp_type epsp) {
        bool cont = true;
        while (cont) {
            cont = co_await async_read_packet(epsp);
        }
        co_return;
    }

    as::awaitable<bool>
    async_read_packet(epsp_type& epsp) {
        error_code ec;
        std::optional<packet_variant> pv_opt;
        if (recycling_allocator_) {
            std::tie(ec, pv_opt) = co_await epsp.async_recv(
                as::bind_allocator(
                    as::recycling_allocator<char>(),
                    as::as_tuple(as::use_awaitable)
                )
            );
        }
        else {
            std::tie(ec, pv_opt) = co_await epsp.async_recv(
                as::as_tuple(as::use_awaitable)
            );
        }

        if (ec) {
            ASYNC_MQTT_LOG("mqtt_broker", info)
                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                << ec.message();
            auto result_opt = get_or_extract_session<tag_con>(epsp);
            if (!result_opt) co_return false;
            auto [remain_after_close, sssp] = *result_opt;
            co_await close_proc(
                epsp,
                remain_after_close,
                sssp,
                true,         // send_will
                std::nullopt, // reasonc_ocde
                as::as_tuple(as::deferred)
            );
            co_return false;
        }
        BOOST_ASSERT(pv_opt);
        auto& pv{*pv_opt};
        auto cont = co_await pv.visit(
            overload {
                [&](v3_1_1::connect_packet& p) -> as::awaitable<bool> {
                    co_await connect_handler(
                        epsp,
                        p.client_id(),
                        p.user_name(),
                        p.password(),
                        p.get_will(),
                        p.clean_session(),
                        p.keep_alive(),
                        properties{}
                    );
                    co_return true;
                },
                [&](v5::connect_packet& p) -> as::awaitable<bool> {
                    co_await connect_handler(
                        epsp,
                        p.client_id(),
                        p.user_name(),
                        p.password(),
                        p.get_will(),
                        p.clean_start(),
                        p.keep_alive(),
                        p.props()
                    );
                    co_return true;
                },
                [&](v3_1_1::publish_packet& p) -> as::awaitable<bool> {
                    co_await publish_handler(
                        epsp,
                        p.packet_id(),
                        p.opts(),
                        p.topic(),
                        p.payload_as_buffer(),
                        properties{}
                    );
                    co_return true;
                },
                [&](v5::publish_packet& p) -> as::awaitable<bool> {
                    co_await publish_handler(
                        epsp,
                        p.packet_id(),
                        p.opts(),
                        p.topic(),
                        p.payload_as_buffer(),
                        p.props()
                    );
                    co_return true;
                },
                [&](v3_1_1::puback_packet& p) -> as::awaitable<bool> {
                    co_await puback_handler(
                        epsp,
                        p.packet_id(),
                        puback_reason_code::success,
                        properties{}
                    );
                    co_return true;
                },
                [&](v5::puback_packet& p) -> as::awaitable<bool> {
                    co_await puback_handler(
                        epsp,
                        p.packet_id(),
                        p.code(),
                        p.props()
                    );
                    co_return true;
                },
                [&](v3_1_1::pubrec_packet& p) -> as::awaitable<bool> {
                    co_await pubrec_handler(
                        epsp,
                        p.packet_id(),
                        pubrec_reason_code::success,
                        properties{}
                    );
                    co_return true;
                },
                [&](v5::pubrec_packet& p) -> as::awaitable<bool> {
                    co_await pubrec_handler(
                        epsp,
                        p.packet_id(),
                        p.code(),
                        p.props()
                    );
                    co_return true;
                },
                [&](v3_1_1::pubrel_packet& p) -> as::awaitable<bool> {
                    co_await pubrel_handler(
                        epsp,
                        p.packet_id(),
                        pubrel_reason_code::success,
                        properties{}
                    );
                    co_return true;
                },
                [&](v5::pubrel_packet& p) -> as::awaitable<bool> {
                    co_await pubrel_handler(
                        epsp,
                        p.packet_id(),
                        p.code(),
                        p.props()
                    );
                    co_return true;
                },
                [&](v3_1_1::pubcomp_packet& p) -> as::awaitable<bool> {
                    co_await pubcomp_handler(
                        epsp,
                        p.packet_id(),
                        pubcomp_reason_code::success,
                        properties{}
                    );
                    co_return true;
                },
                [&](v5::pubcomp_packet& p) -> as::awaitable<bool> {
                    co_await pubcomp_handler(
                        epsp,
                        p.packet_id(),
                        p.code(),
                        p.props()
                    );
                    co_return true;
                },
                [&](v3_1_1::subscribe_packet& p) -> as::awaitable<bool> {
                    co_await subscribe_handler(
                        epsp,
                        p.packet_id(),
                        p.entries(),
                        properties{}
                    );
                    co_return true;
                },
                [&](v5::subscribe_packet& p) -> as::awaitable<bool> {
                    co_await subscribe_handler(
                        epsp,
                        p.packet_id(),
                        p.entries(),
                        p.props()
                    );
                    co_return true;
                },
                [&](v3_1_1::suback_packet&) -> as::awaitable<bool> {
                    // TBD receive invalid packet
                    co_return true;
                },
                [&](v5::suback_packet&) -> as::awaitable<bool> {
                    // TBD receive invalid packet
                    co_return true;
                },
                [&](v3_1_1::unsubscribe_packet& p) -> as::awaitable<bool> {
                    co_await unsubscribe_handler(
                        epsp,
                        p.packet_id(),
                        p.entries(),
                        properties{}
                    );
                    co_return true;
                },
                [&](v5::unsubscribe_packet& p) -> as::awaitable<bool> {
                    co_await unsubscribe_handler(
                        epsp,
                        p.packet_id(),
                        p.entries(),
                        p.props()
                    );
                    co_return true;
                },
                [&](v3_1_1::pingreq_packet&) -> as::awaitable<bool> {
                    co_await pingreq_handler(
                        epsp
                    );
                    co_return true;
                },
                [&](v5::pingreq_packet&) -> as::awaitable<bool> {
                    co_await pingreq_handler(
                        epsp
                    );
                    co_return true;
                },
                [&](v3_1_1::disconnect_packet&) -> as::awaitable<bool> {
                    co_await disconnect_handler(
                        epsp,
                        disconnect_reason_code::normal_disconnection,
                        properties{}
                    );
                    co_return false;
                },
                [&](v5::disconnect_packet& p) -> as::awaitable<bool> {
                    co_await disconnect_handler(
                        epsp,
                        p.code(),
                        p.props()
                    );
                    co_return false;
                },
                [&](v5::auth_packet& p) -> as::awaitable<bool> {
                    co_await auth_handler(
                        epsp,
                        p.code(),
                        p.props()
                    );
                    co_return true;
                },
                [&](auto const&) -> as::awaitable<bool> {
                    ASYNC_MQTT_LOG("mqtt_broker", fatal)
                        << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                        << "invalid variant";
                    co_return false;
                }
            }
        );

        co_return cont;
    }

    as::awaitable<void>
    connect_handler(
        epsp_type& epsp,
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

            co_await send_connack(
                epsp,
                false, // session present
                false, // authenticated
                force_move(connack_props)
            );
            co_await disconnect_and_close(
                epsp,
                version,
                disconnect_reason_code::not_authorized
            );
            co_return;
        }

        if (client_id.empty()) {
            auto result = co_await handle_empty_client_id(
                epsp,
                client_id,
                clean_start,
                connack_props
            );
            if (!result) {
                co_return;
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
        std::unique_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_cid>();
        auto it = idx.lower_bound(std::make_tuple(*username, client_id));

        auto renew_or_inherit =
            [&] {
                idx.modify(
                    it,
                    [&](auto& e) {
                        if (clean_start) {
                            // discard offline session
                            ASYNC_MQTT_LOG("mqtt_broker", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "un:" << *username
                                << "offline connection exists, discard old one due to new one's clean_start and renew";
                            e->renew(
                                epsp,
                                will,
                                clean_start,
                                will_expiry_interval,
                                session_expiry_interval
                            );
                        }
                        else {
                            // inherit online session if previous session's session exists
                            ASYNC_MQTT_LOG("mqtt_broker", trace)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "un:" << *username
                                << "offline connection exists, and inherit it";
                            e->inherit(
                                epsp,
                                will,
                                will_expiry_interval,
                                session_expiry_interval
                            );
                        }
                    },
                    [](auto&) { BOOST_ASSERT(false); }
                );
            };

        if (it == idx.end() ||
            (*it)->client_id() != client_id ||
            (*it)->get_username() != *username
        ) {
            // no existing gonnection
            // new connection
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                << "cid:" << client_id
                << " new connection inserted.";
            it = idx.emplace_hint(
                it,
                session_state<epsp_type>::create(
                    mtx_subs_map_,
                    subs_map_,
                    shared_targets_,
                    epsp,
                    client_id,
                    *username,
                    force_move(will),
                    // will_sender
                    [this](auto&&... params) -> as::awaitable<void> {
                        co_await this->do_publish(std::forward<decltype(params)>(params)...);
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
            g.unlock();
            co_await send_connack(
                epsp,
                false, // session present
                true,  // authenticated
                force_move(connack_props)
            );
        }
        else {
            // connection exists
            auto remain_after_close = (*it)->remain_after_close();
            std::shared_ptr<session_state<epsp_type>> sssp;
            if (remain_after_close) {
                sssp = const_cast<std::shared_ptr<session_state<epsp_type>>&>(*it);
            }
            else {
                sssp = force_move(idx.extract(it).value());
            }
            if (auto old_epsp = sssp->lock()) {
                // online overwrite
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << "cid:" << client_id
                    << " old connection " << old_epsp.get_address() << " exists and is online. close it ";
                if (remain_after_close) {
                    renew_or_inherit();
                    g.unlock();
                    co_await close_proc(
                        old_epsp,
                        remain_after_close,
                        sssp,
                        true,
                        disconnect_reason_code::session_taken_over,
                        as::as_tuple(as::deferred)
                    );
                    // offline exists -> online
                    co_await offline_to_online(
                        epsp,
                        *sssp,
                        clean_start,
                        force_move(*username),
                        response_topic_requested,
                        force_move(connack_props)
                    );
                }
                else {
                    // old one is extracted (not in sessions_) by get_or_extract_session_no_lock().
                    // new connection
                    ASYNC_MQTT_LOG("mqtt_broker", trace)
                        << ASYNC_MQTT_ADD_VALUE(address, this)
                        << "cid:" << client_id
                        << "online connection exists, discard old one due to session_expiry and renew";
                    bool inserted;
                    std::tie(it, inserted) = idx.emplace(
                        session_state<epsp_type>::create(
                            mtx_subs_map_,
                            subs_map_,
                            shared_targets_,
                            epsp,
                            client_id,
                            *username,
                            force_move(will),
                            // will_sender
                            [this](auto&&... params) -> as::awaitable<void> {
                                co_await this->do_publish(std::forward<decltype(params)>(params)...);
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
                    g.unlock();
                    co_await close_proc(
                        old_epsp,
                        remain_after_close,
                        sssp,
                        true,
                        disconnect_reason_code::session_taken_over,
                        as::as_tuple(as::deferred)
                    );
                    co_await send_connack(
                        epsp,
                        false, // session present
                        true,  // authenticated
                        force_move(connack_props)
                    );
                }
            }
            else {
                renew_or_inherit();
                g.unlock();
                // offline exists -> online
                co_await offline_to_online(
                    epsp,
                    *sssp,
                    clean_start,
                    force_move(*username),
                    response_topic_requested,
                    force_move(connack_props)
                );
            }
        }
    }

    as::awaitable<void>
    offline_to_online(
        epsp_type epsp,
        session_state<epsp_type>& ss,
        bool clean_start,
        std::string username,
        bool response_topic_requested,
        properties connack_props
    ) {
        if (clean_start) {
            if (response_topic_requested) {
                // set_response_topic never modify key part
                set_response_topic(ss, connack_props, username);
            }
            co_await send_connack(
                epsp,
                false, // session present
                true,  // authenticated
                force_move(connack_props)
            );
        }
        else {
            if (response_topic_requested) {
                // set_response_topic never modify key part
                set_response_topic(ss, connack_props, username);
            }
            auto ec = co_await send_connack(
                epsp,
                true, // session present
                true, // authenticated
                force_move(connack_props)
            );
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, this)
                    << ec.message();
                co_return;
            }
            ss.send_inflight_messages();
            ss.send_all_offline_messages();
        }
        co_return;
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
                std::unique_lock<mutex> g(mtx_retains_);
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

    as::awaitable<bool>
    handle_empty_client_id(
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
                        auto [ec] = co_await epsp.async_send(
                            v3_1_1::connack_packet{
                                false,
                                connect_return_code::identifier_rejected
                            },
                            as::as_tuple(as::use_awaitable)
                        );
                        if (ec) {
                            ASYNC_MQTT_LOG("mqtt_broker", info)
                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                << ec.message();
                        }
                        co_await epsp.async_close(
                            as::as_tuple(as::use_awaitable)
                        );
                        ASYNC_MQTT_LOG("mqtt_broker", info)
                            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                            << "closed";
                    }
                    co_return false;
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
            co_return false;
        }
        co_return true;
    }

    as::awaitable<error_code>
    send_connack(
        epsp_type& epsp,
        bool session_present,
        bool authenticated,
        properties props
    ) {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
            << "send_connack";
        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1: {
            if (connack_) {
                auto [ec] = co_await epsp.async_send(
                    v3_1_1::connack_packet{
                        session_present,
                        authenticated ? connect_return_code::accepted
                                      : connect_return_code::not_authorized,
                    },
                    as::as_tuple(as::use_awaitable)
                );
                co_return ec;
            }
        } break;
        case protocol_version::v5: {
            // connack_props_ member varible is for testing
            if (connack_props_.empty()) {
                // props local variable is is for real case
                props.emplace_back(property::topic_alias_maximum{topic_alias_max});
                props.emplace_back(property::receive_maximum{receive_maximum_max});
                if (connack_) {
                    auto [ec] = co_await epsp.async_send(
                        v5::connack_packet{
                            session_present,
                            authenticated ? connect_reason_code::success
                                          : connect_reason_code::not_authorized,
                            force_move(props)
                        },
                        as::as_tuple(as::use_awaitable)
                    );
                    co_return ec;
                }
            }
            else {
                // use connack_props_ for testing
                if (connack_) {
                    auto [ec] = co_await epsp.async_send(
                        v5::connack_packet{
                            session_present,
                            authenticated ? connect_reason_code::success
                                          : connect_reason_code::not_authorized,
                            connack_props_
                        },
                        as::as_tuple(as::use_awaitable)
                    );
                    co_return ec;
                }
            }
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }

        co_return error_code{};
    }

    as::awaitable<void>
    publish_handler(
        epsp_type& epsp,
        packet_id_type packet_id,
        pub::opts opts,
        std::string topic,
        std::vector<buffer> payload,
        properties props
    ) {
        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) co_return;

        std::shared_ptr<session_state<epsp_type>> sssp{*it};
        g.unlock();

        auto send_pubres =
            [&] (bool authorized, bool matched) -> as::awaitable<void> {
                switch (opts.get_qos()) {
                case qos::at_least_once:
                    switch (epsp.get_protocol_version()) {
                    case protocol_version::v3_1_1: {
                        auto [ec] = co_await epsp.async_send(
                            v3_1_1::puback_packet{
                                packet_id
                            },
                            as::as_tuple(as::use_awaitable)
                        );
                        if (ec) {
                            ASYNC_MQTT_LOG("mqtt_broker", info)
                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                << ec.message();
                        }
                    } break;
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
                        auto [ec] = co_await epsp.async_send(
                            force_move(packet),
                            as::as_tuple(as::use_awaitable)
                        );
                        if (ec) {
                            ASYNC_MQTT_LOG("mqtt_broker", info)
                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                << ec.message();
                        }
                    } break;
                    default:
                        BOOST_ASSERT(false);
                        break;
                    }
                break;
                case qos::exactly_once:
                    switch (epsp.get_protocol_version()) {
                    case protocol_version::v3_1_1: {
                        auto [ec] = co_await epsp.async_send(
                            v3_1_1::pubrec_packet{
                                packet_id
                            },
                            as::as_tuple(as::use_awaitable)
                        );
                        if (ec) {
                            ASYNC_MQTT_LOG("mqtt_broker", info)
                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                << ec.message();
                        }
                    } break;
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

                        auto [ec] = co_await epsp.async_send(
                            force_move(packet),
                            as::as_tuple(as::use_awaitable)
                        );
                        if (ec) {
                            ASYNC_MQTT_LOG("mqtt_broker", info)
                                << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                                << ec.message();
                        }
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
                return security_.auth_pub(topic, sssp->get_username()) != security::authorization::type::allow;
            } ()
        ) {
            // Publish not authorized
            co_await send_pubres(false, false);
            co_return;
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

        bool matched = co_await do_publish(
            *sssp,
            force_move(topic),
            force_move(payload),
            opts.get_qos() | opts.get_retain(), // remove dup flag
            force_move(forward_props)
        );

        co_await send_pubres(true, matched);
        co_return;
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
    as::awaitable<bool>
    do_publish(
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
            [&]
            (session_state<epsp_type>& ss, subscription<epsp_type>& sub, auto const& auth_users)
            -> as::awaitable<bool> {
                // See if this session is authorized to subscribe this topic
                {
                    std::shared_lock<mutex> g_sec{mtx_security_};
                    auto access = security_.auth_sub_user(auth_users, ss.get_username());
                    if (access != security::authorization::type::allow) co_return false;
                }
                pub::opts new_opts = std::min(opts.get_qos(), sub.opts.get_qos());
                if (sub.opts.get_rap() == sub::rap::retain && opts.get_retain() == pub::retain::yes) {
                    new_opts |= pub::retain::yes;
                }

                if (sub.sid) {
                    props.push_back(property::subscription_identifier(boost::numeric_cast<std::uint32_t>(*sub.sid)));
                    co_await ss.deliver(
                        topic,
                        payload,
                        new_opts,
                        props
                    );
                    props.pop_back();
                }
                else {
                    co_await ss.deliver(
                        topic,
                        payload,
                        new_opts,
                        props
                    );
                }
                co_return true;
            };

        //                  share_name   topic_filter
        std::set<std::tuple<std::string_view, std::string_view>> sent;
        std::vector<
            std::tuple<
                as::any_io_executor,
                std::function<as::awaitable<void>()>
            >
        > pub_deliver;

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
                        pub_deliver.emplace_back(
                            source_ss.get_executor(),
                            [sub, auth_users, &matched, &deliver] () mutable -> as::awaitable<void> {
                                auto result = co_await deliver(sub.ss.get(), sub, auth_users);
                                if (result) matched = true;
                                co_return;
                            }
                        );
                    }
                    else {
                        // Shared subscriptions
                        bool inserted;
                        std::tie(std::ignore, inserted) = sent.emplace(sub.sharename, sub.topic);
                        if (inserted) {
                            if (auto ssr_sub_opt = shared_targets_.get_target(sub.sharename, sub.topic)) {
                                auto [ssr, sub] = *ssr_sub_opt;
                                pub_deliver.emplace_back(
                                    source_ss.get_executor(),
                                    [ssr, sub, auth_users, &matched, &deliver] () mutable -> as::awaitable<void> {
                                        auto result = co_await deliver(ssr.get(), sub, auth_users);
                                        if (result) matched = true;
                                        co_return;
                                    }
                                );
                            }
                        }
                    }
                }
            );
        }
        using op_type = decltype(
            as::co_spawn(
                std::declval<as::any_io_executor>(),
                std::declval<as::awaitable<void>>(),
                as::deferred
            )
        );
        std::vector<op_type> ops;
        ops.reserve(pub_deliver.size());
        for (auto& [exe, pd] : pub_deliver) {
            ops.push_back(
                as::co_spawn(
                    exe,
                    pd(),
                    as::deferred
                )
            );
        }
        if (!ops.empty()) {
            co_await as::experimental::make_parallel_group(force_move(ops)).async_wait(
                as::experimental::wait_for_all(),
                as::deferred
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
                std::unique_lock<mutex> g(mtx_retains_);
                retains_.erase(topic);
            }
            else {
                std::shared_ptr<as::steady_timer> tim_message_expiry;
                if (message_expiry_interval) {
                    tim_message_expiry = std::make_shared<as::steady_timer>(
                        source_ss.get_executor(),
                        *message_expiry_interval
                    );
                    tim_message_expiry->async_wait(
                        [this, topic = topic, wp = std::weak_ptr<as::steady_timer>(tim_message_expiry)]
                        (boost::system::error_code const& ec) {
                            if (auto sp = wp.lock()) {
                                if (!ec) {
                                    std::lock_guard<mutex> g(mtx_retains_);
                                    retains_.erase(topic);
                                }
                            }
                        }
                    );
                }

                std::unique_lock<mutex> g(mtx_retains_);
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
        co_return matched;
    }

    as::awaitable<void>
    puback_handler(
        epsp_type& epsp,
        packet_id_type packet_id,
        puback_reason_code /*reason_code*/,
        properties /*props*/
    ) {
        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) co_return;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<epsp_type>&>(**it);
        ss.erase_inflight_message_by_packet_id(packet_id);
        ss.send_offline_messages_by_packet_id_release();
    }

    as::awaitable<void>
    pubrec_handler(
        epsp_type& epsp,
        packet_id_type packet_id,
        pubrec_reason_code reason_code,
        properties /*props*/
    ) {
        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) co_return;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<epsp_type>&>(**it);

        if (make_error_code(reason_code)) co_return;
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

        co_return;
    }

    as::awaitable<void>
    pubrel_handler(
        epsp_type& epsp,
        packet_id_type packet_id,
        pubrel_reason_code reason_code,
        properties /*props*/
    ) {
        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) co_return;

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

        co_return;
    }

    as::awaitable<void>
    pubcomp_handler(
        epsp_type& epsp,
        packet_id_type packet_id,
        pubcomp_reason_code /*reason_code*/,
        properties /*props*/
    ){
        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) co_return;

        // const_cast is appropriate here
        // See https://github.com/boostorg/multi_index/issues/50
        auto& ss = const_cast<session_state<epsp_type>&>(**it);
        ss.erase_inflight_message_by_packet_id(packet_id);
        ss.send_offline_messages_by_packet_id_release();

        co_return;
    }

    as::awaitable<void>
    subscribe_handler(
        epsp_type& epsp,
        packet_id_type packet_id,
        std::vector<topic_subopts> const& entries,
        properties props
    ) {
        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) co_return;

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
            [&ssr, &epsp]
            (retain_type const& r, qos qos_value, std::optional<std::size_t> sid) -> as::awaitable<void> {
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
                co_await ssr.get().publish(
                    epsp,
                    r.topic,
                    r.payload,
                    std::min(r.qos_value, qos_value) | pub::retain::yes,
                    props
                );
            };

        std::vector<std::function<as::awaitable<void>()>> retain_deliver;
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
                                        [&publish_proc, &r, qos_value = e.opts().get_qos(), sid]
                                        () -> as::awaitable<void>{
                                            co_await publish_proc(r, qos_value, sid);
                                            co_return;
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
            auto [ec] = co_await epsp.async_send(
                v3_1_1::suback_packet{
                    packet_id,
                    force_move(res)
                },
                as::as_tuple(as::use_awaitable)
            );
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_broker", info)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << ec.message();
            }
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
                                            [&publish_proc, &r, qos_value = e.opts().get_qos(), sid]
                                            () -> as::awaitable<void>{
                                                co_await publish_proc(r, qos_value, sid);
                                                co_return;
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
            auto [ec] = co_await epsp.async_send(
                v5::suback_packet{
                    packet_id,
                    force_move(res),
                    suback_props_
                },
                as::as_tuple(as::use_awaitable)
            );
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_broker", info)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << ec.message();
            }
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }

        for (auto const& f : retain_deliver) {
            co_await f();
        }

        co_return;
    }

    as::awaitable<void>
    unsubscribe_handler(
        epsp_type& epsp,
        packet_id_type packet_id,
        std::vector<topic_sharename> entries,
        properties props
    ) {
        std::shared_lock<mutex> g(mtx_sessions_);
        auto& idx = sessions_.template get<tag_con>();
        auto it  = idx.find(epsp);

        // broker uses async_* APIs
        // If broker erase a connection, then async_force_disconnect()
        // and/or async_force_disconnect () is called.
        // During async operation, spep is valid but it has already been
        // erased from sessions_
        if (it == idx.end()) co_return;

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
        case protocol_version::v3_1_1: {
            auto [ec] = co_await epsp.async_send(
                v3_1_1::unsuback_packet{
                    packet_id
                },
                as::as_tuple(as::use_awaitable)
            );
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_broker", info)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << ec.message();
            }
        } break;
        case protocol_version::v5: {
            if (h_unsubscribe_props_) h_unsubscribe_props_(props);
            auto [ec] = co_await epsp.async_send(
                v5::unsuback_packet{
                    packet_id,
                    std::vector<unsuback_reason_code>(
                        entries.size(),
                        unsuback_reason_code::success
                    ),
                    unsuback_props_
                },
                as::as_tuple(as::use_awaitable)
            );
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_broker", info)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << ec.message();
            }
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }

        co_return;
    }

    as::awaitable<void>
    pingreq_handler(
        epsp_type& epsp
    ) {
        if (!pingresp_) co_return;

        switch (epsp.get_protocol_version()) {
        case protocol_version::v3_1_1: {
            auto [ec] = co_await epsp.async_send(
                v3_1_1::pingresp_packet{},
                as::as_tuple(as::use_awaitable)
            );
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_broker", info)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << ec.message();
            }
        } break;
        case protocol_version::v5: {
            auto [ec] = co_await epsp.async_send(
                v5::pingresp_packet{},
                as::as_tuple(as::use_awaitable)
            );
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_broker", info)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << ec.message();
            }
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }

        co_return;
    }

    as::awaitable<void>
    disconnect_handler(
        epsp_type& epsp,
        disconnect_reason_code rc,
        properties /*props*/
    ) {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
            << "disconnect_handler";
        if (delay_disconnect_) {
            ///< Used to delay disconnect handling for testing
            as::steady_timer tim_disconnect{epsp.get_executor()};
            tim_disconnect.expires_after(*delay_disconnect_);
            co_await tim_disconnect.async_wait(as::as_tuple(as::deferred));
        }

        auto result_opt = get_or_extract_session<tag_con>(epsp);
        auto [remain_after_close, sssp] = *result_opt;
        co_await close_proc(
            epsp,
            remain_after_close,
            sssp,
            rc == disconnect_reason_code::disconnect_with_will_message,
            rc,
            as::as_tuple(as::deferred)
        );
    }

    /**
     * @brief close_proc - clean up a connection that has been closed.
     *
     * @param ep - The underlying server (of whichever type) that is disconnecting.
     * @param send_will - Whether to publish this connections last will
     * @param rc - Reason Code for send pack DISCONNECT
     * @retrun remain as offline
     */
    template <typename CompletionToken>
    auto
    close_proc(
        epsp_type& epsp,
        bool remain_after_close,
        std::shared_ptr<session_state<epsp_type>> sssp,
        bool send_will,
        std::optional<disconnect_reason_code> rc_opt,
        CompletionToken&& token
    ) {
        return as::co_spawn(
            epsp.get_executor(),
            close_proc_impl(epsp, remain_after_close, sssp, send_will, rc_opt),
            std::forward<CompletionToken>(token)
        );
    }

    as::awaitable<void>
    auth_handler(
        epsp_type /*epsp*/,
        auth_reason_code /*rc*/,
        properties props
    ) {
        if (h_auth_props_) h_auth_props_(force_move(props));

        co_return;
    }

    as::awaitable<void>
    disconnect_and_close(
        epsp_type epsp,
        protocol_version version,
        disconnect_reason_code rc
    ) {
        if (version == protocol_version::v5) {
            auto [ec] = co_await epsp.async_send(
                v5::disconnect_packet{
                    rc,
                    properties{}
                },
                as::as_tuple(as::use_awaitable)
            );
            if (ec) {
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ec.message();
            }
        }

        co_await epsp.async_close(
            as::as_tuple(as::use_awaitable)
        );

        ASYNC_MQTT_LOG("mqtt_broker", info)
            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
            << "closed";

        co_return;
    }

    template <typename Tag, typename Key>
    std::optional<std::tuple<bool, std::shared_ptr<session_state<epsp_type>>>>
    get_or_extract_session(Key const& key) {
        std::unique_lock<mutex> g{mtx_sessions_};
        return get_or_extract_session_no_lock<Tag>(key);
    }

    template <typename Tag, typename Key>
    std::optional<std::tuple<bool, std::shared_ptr<session_state<epsp_type>>>>
    get_or_extract_session_no_lock(Key const& key) {
        bool remain_after_close = false;
        std::shared_ptr<session_state<epsp_type>> sssp;
        auto& idx = sessions_.template get<Tag>();
        auto it = idx.find(key);
        if (it == idx.end()) return std::nullopt;
        remain_after_close = (*it)->remain_after_close();
        if (remain_after_close) {
            sssp = const_cast<std::shared_ptr<session_state<epsp_type>>&>(*it);
        }
        else {
            sssp = force_move(idx.extract(it).value());
        }
        return std::make_tuple(remain_after_close, sssp);
    }

    as::awaitable<void>
    close_proc_impl(
        epsp_type& epsp,
        bool remain_after_close,
        std::shared_ptr<session_state<epsp_type>> sssp,
        bool send_will,
        std::optional<disconnect_reason_code> rc_opt
    ) {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
            << "close_proc";


        if (remain_after_close) {
            if (send_will) {
                co_await sssp->send_will();
            }
            else {
                sssp->clear_will();
            }
            if (rc_opt) {
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << "disconnect_and_close() cid:" << sssp->client_id();
                co_await disconnect_and_close(
                    epsp,
                    sssp->get_protocol_version(),
                    *rc_opt
                );
            }
            else {
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << "close cid:" << sssp->client_id();
                co_await epsp.async_close(
                    as::as_tuple(as::use_awaitable)
                );
            }
            sssp->become_offline(
                epsp,
                [this, sssp, epsp]
                (std::shared_ptr<as::steady_timer> const& sp_tim) mutable {
                    as::co_spawn(
                        epsp.get_executor(),
                        sssp->send_will(true), // no delay
                        as::consign(
                            as::detached,
                            sssp
                        )
                    );
                    // lock for expire (async)
                    std::unique_lock<mutex> g(mtx_sessions_);
                    sessions_.template get<tag_tim>().erase(sp_tim);
                }
            );
        }
        else {
            if (send_will) {
                co_await sssp->send_will();
            }
            else {
                sssp->clear_will();
            }
            if (rc_opt) {
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << "disconnect_and_close() cid:" << sssp->client_id();
                co_await disconnect_and_close(
                    epsp,
                    sssp->get_protocol_version(),
                    *rc_opt
                );
            }
            else {
                ASYNC_MQTT_LOG("mqtt_broker", trace)
                    << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
                    << "close cid:" << sssp->client_id();
                co_await epsp.async_close(
                    as::as_tuple(as::use_awaitable)
                );
            }
        }
        co_return;
    }

private:

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
    bool recycling_allocator_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_BROKER_HPP
