// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_SESSION_STATE_HPP)
#define ASYNC_MQTT_BROKER_SESSION_STATE_HPP

#include <chrono>
#include <set>

#include <boost/asio/io_context.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/protocol/packet/will.hpp>

#include <broker/sub_con_map.hpp>
#include <broker/shared_target.hpp>
#include <broker/tags.hpp>
#include <broker/inflight_message.hpp>
#include <broker/offline_message.hpp>
#include <broker/mutex.hpp>

namespace async_mqtt {

namespace as = boost::asio;
namespace mi = boost::multi_index;

template <typename Sp>
class session_states;

/**
 * http://docs.oasis-open.org/mqtt/mqtt/v5.0/cs02/mqtt-v5.0-cs02.html#_Session_State
 *
 * 4.1 Session State
 * In order to implement QoS 1 and QoS 2 protocol flows the Client and Server need to associate state with the Client Identifier, this is referred to as the Session State. The Server also stores the subscriptions as part of the Session State.
 * The session can continue across a sequence of Network Connections. It lasts as long as the latest Network Connection plus the Session Expiry Interval.
 * The Session State in the Server consists of:
 * · The existence of a Session, even if the rest of the Session State is empty.
 * · The Clients subscriptions, including any Subscription Identifiers.
 * · QoS 1 and QoS 2 messages which have been sent to the Client, but have not been completely acknowledged.
 * · QoS 1 and QoS 2 messages pending transmission to the Client and OPTIONALLY QoS 0 messages pending transmission to the Client.
 * · QoS 2 messages which have been received from the Client, but have not been completely acknowledged.
 * · The Will Message and the Will Delay Interval
 * · If the Session is currently not connected, the time at which the Session will end and Session State will be discarded.
 *
 * Retained messages do not form part of the Session State in the Server, they are not deleted as a result of a Session ending.
 */
template <typename Sp>
struct session_state : std::enable_shared_from_this<session_state<Sp>> {
    using this_type = session_state<Sp>;
    using epsp_type = Sp;
    using epwp_type = typename epsp_type::weak_type;
    using will_sender_type = std::function<
        void(
            session_state<epsp_type> const& source_ss,
            std::string topic,
            std::vector<buffer> payload,
            pub::opts pubopts,
            properties props
        )
    >;

    static std::shared_ptr<session_state<Sp>> create(
        mutex& mtx_subs_map,
        sub_con_map<epsp_type>& subs_map,
        shared_target<epsp_type>& shared_targets,
        epsp_type epsp,
        std::string client_id,
        std::string const& username,
        std::optional<will> will,
        will_sender_type will_sender,
        bool clean_start,
        std::optional<std::chrono::steady_clock::duration> will_expiry_interval,
        std::optional<std::chrono::steady_clock::duration> session_expiry_interval
    ) {
        struct impl : session_state<Sp> {
            impl(
                mutex& mtx_subs_map,
                sub_con_map<epsp_type>& subs_map,
                shared_target<epsp_type>& shared_targets,
                epsp_type epsp,
                std::string client_id,
                std::string const& username,
                will_sender_type will_sender,
                bool clean_start,
                std::optional<std::chrono::steady_clock::duration> session_expiry_interval)
                :
                session_state<Sp> {
                    mtx_subs_map,
                    subs_map,
                    shared_targets,
                    force_move(epsp),
                    force_move(client_id),
                    username,
                    force_move(will_sender),
                    clean_start,
                    force_move(session_expiry_interval)
                }
            {}
        };
        std::shared_ptr<session_state<Sp>> sssp = std::make_shared<impl>(
            mtx_subs_map,
            subs_map,
            shared_targets,
            force_move(epsp),
            force_move(client_id),
            username,
            force_move(will_sender),
            clean_start,
            force_move(session_expiry_interval)
        );
        sssp->update_will(will, will_expiry_interval);
        return sssp;
    }

    ~session_state() {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "session destroy";
        send_will_impl();
        clean();
    }

    template <typename SessionExpireHandler>
    void become_offline(
        epsp_type epsp,
        SessionExpireHandler&& session_expire_handler
    ) {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "store inflight message";
        auto stored = epsp.get_stored_packets();
        for (auto& store : stored) {
            std::shared_ptr<as::steady_timer> tim_message_expiry;
            store.visit(
                overload {
                    [&](v5::publish_packet const& p) {
                        for (auto const& prop : p.props()) {
                            prop.visit(
                                overload {
                                    [&](property::message_expiry_interval const& v) {
                                        tim_message_expiry =
                                            std::make_shared<as::steady_timer>(
                                                exe_,
                                                std::chrono::seconds(v.val())
                                            );
                                        tim_message_expiry->async_wait(
                                            [this, wp = std::weak_ptr<as::steady_timer>(tim_message_expiry)]
                                            (error_code ec) {
                                                if (auto sp = wp.lock()) {
                                                    if (!ec) {
                                                        erase_inflight_message_by_expiry(sp);
                                                    }
                                                }
                                            }
                                        );
                                    },
                                    [](auto const&) {}
                                }
                            );
                        }
                    },
                    [&](auto const&) {}
                }
            );

            insert_inflight_message(
                force_move(store),
                force_move(tim_message_expiry)
            );
        }

        qos2_publish_handled_ = epsp.get_qos2_publish_handled_pids();

        if (session_expiry_interval_ &&
            *session_expiry_interval_ != std::chrono::seconds(session_never_expire)) {

            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "session expiry interval timer set";

            tim_session_expiry_->expires_after(*session_expiry_interval_);
            tim_session_expiry_->async_wait(
                [
                    this,
                    wp = std::weak_ptr<as::steady_timer>(tim_session_expiry_),
                    session_expire_handler = std::forward<SessionExpireHandler>(session_expire_handler)
                ]
                (error_code ec) {
                    if (auto sp = wp.lock()) {
                        if (!ec) {
                            ASYNC_MQTT_LOG("mqtt_broker", info)
                                << ASYNC_MQTT_ADD_VALUE(address, this)
                                << "session expired";
                            session_expire_handler(sp);
                        }
                    }
                }
            );
        }
    }

    void renew(
        epsp_type epsp,
        std::optional<will> will,
        bool clean_start,
        std::optional<std::chrono::steady_clock::duration> will_expiry_interval,
        std::optional<std::chrono::steady_clock::duration> session_expiry_interval
    ) {
        clean();
        epwp_ = epsp;
        auto version = epsp.get_protocol_version();
        if (version == protocol_version::v3_1_1) {
            remain_after_close_= !clean_start;
        }
        else {
            BOOST_ASSERT(version == protocol_version::v5);
            remain_after_close_=
                session_expiry_interval_ &&
                *session_expiry_interval_ != std::chrono::steady_clock::duration::zero();
        }
        update_will(force_move(will), will_expiry_interval);
        session_expiry_interval_ = force_move(session_expiry_interval);
    }

    void publish(
        epsp_type& epsp,
        std::string pub_topic,
        std::vector<buffer> payload,
        pub::opts pubopts,
        properties props) {

        auto send_publish =
            [this, epsp, pub_topic, payload = payload, pubopts, props, wp = this->weak_from_this()]
            (packet_id_type pid) mutable {
                if (auto sp = wp.lock()) {
                    switch (version_) {
                    case protocol_version::v3_1_1:
                        epsp.async_send(
                            v3_1_1::publish_packet{
                                pid,
                                force_move(pub_topic),
                                force_move(payload),
                                pubopts
                            },
                            [this, epsp](error_code const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, this)
                                        << "epsp:" << epsp.get_address() << " "
                                        << ec.message();
                                }
                            }
                        );
                        break;
                    case protocol_version::v5:
                        epsp.async_send(
                            v5::publish_packet{
                                pid,
                                force_move(pub_topic),
                                force_move(payload),
                                pubopts,
                                force_move(props)
                            },
                            [this, epsp](error_code const& ec) {
                                if (ec) {
                                    ASYNC_MQTT_LOG("mqtt_broker", info)
                                        << ASYNC_MQTT_ADD_VALUE(address, this)
                                        << "epsp:" << epsp.get_address() << " "
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
            };

        std::lock_guard<mutex> g(mtx_offline_messages_);
        if (offline_messages_.empty()) {
            auto qos_value = pubopts.get_qos();
            if (qos_value == qos::at_least_once ||
                qos_value == qos::exactly_once) {
                epsp.async_acquire_unique_packet_id(
                    [send_publish = force_move(send_publish)]
                    (error_code const&  ec, auto pid) mutable {
                        if (!ec) {
                            send_publish(pid);
                            return;
                        }
                    }
                );
                return;
            }
            else {
                send_publish(0);
                return;
            }
        }

        // offline_messages_ is not empty or packet_id_exhausted
        offline_messages_.push_back(
            exe_,
            force_move(pub_topic),
            force_move(payload),
            pubopts,
            force_move(props)
        );
    }

    void deliver(
        std::string pub_topic,
        std::vector<buffer> payload,
        pub::opts pubopts,
        properties props) {

        if (auto epsp = lock()) {
            publish(
                epsp,
                force_move(pub_topic),
                force_move(payload),
                pubopts,
                force_move(props)
            );
        }
        else {
            std::lock_guard<mutex> g(mtx_offline_messages_);
            offline_messages_.push_back(
                exe_,
                force_move(pub_topic),
                force_move(payload),
                pubopts,
                force_move(props)
            );
        }
    }

    void set_clean_handler(std::function<void()> handler) {
        clean_handler_ = force_move(handler);
    }

    void clean() {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "clean";
        if (clean_handler_) clean_handler_();
        if (tim_will_expiry_) tim_will_expiry_->cancel();
        will_value_ = std::nullopt;
        {
            std::lock_guard<mutex> g(mtx_inflight_messages_);
            inflight_messages_.clear();
        }
        {
            std::lock_guard<mutex> g(mtx_offline_messages_);
            offline_messages_.clear();
        }
        unsubscribe_all();
        shared_targets_.erase(*this);
        tim_will_delay_.cancel();

        session_expiry_interval_ = std::nullopt;
        if (tim_session_expiry_) tim_session_expiry_->cancel();
        qos2_publish_handled_.clear();
        response_topic_ = std::nullopt;
    }

    template <typename PublishRetainHandler>
    void subscribe(
        std::string share_name,
        std::string topic_filter,
        sub::opts subopts,
        PublishRetainHandler&& h,
        std::optional<std::size_t> sid = std::nullopt
    ) {
        subscription<epsp_type> sub {*this, share_name, topic_filter, subopts, sid };
        if (!share_name.empty()) {
            shared_targets_.insert(share_name, topic_filter, sub, *this);
        }
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "subscribe"
            << " share_name:" << share_name
            << " topic_filter:" << topic_filter
            << " qos:" << subopts.get_qos();

        auto handle_ret =
            [&] {
                std::lock_guard<mutex> g{mtx_subs_map_};
                return subs_map_.insert_or_assign(
                    force_move(topic_filter),
                    client_id_,
                    force_move(sub)
                );
            } ();

        auto rh = subopts.get_retain_handling();

        if (handle_ret.second) { // insert
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "subscription inserted";

            handles_.insert(handle_ret.first);
            if (rh == sub::retain_handling::send ||
                rh == sub::retain_handling::send_only_new_subscription) {
                std::forward<PublishRetainHandler>(h)();
            }
        }
        else { // update
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "subscription updated";

            if (rh == sub::retain_handling::send) {
                std::forward<PublishRetainHandler>(h)();
            }
        }
    }

    void unsubscribe(std::string const& share_name, std::string const& topic_filter) {
        if (!share_name.empty()) {
            shared_targets_.erase(share_name, topic_filter, *this);
        }
        std::lock_guard<mutex> g{mtx_subs_map_};
        auto handle = subs_map_.lookup(topic_filter);
        if (handle) {
            handles_.erase(*handle);
            subs_map_.erase(*handle, client_id_);
        }
    }

    void unsubscribe_all() {
        {
            std::lock_guard<mutex> g{mtx_subs_map_};
            for (auto const& h : handles_) {
                subs_map_.erase(h, client_id_);
            }
        }
        handles_.clear();
    }

    void update_will(
        std::optional<async_mqtt::will> will,
        std::optional<std::chrono::steady_clock::duration> will_expiry_interval) {
        tim_will_expiry_.reset();
        will_value_ = force_move(will);

        if (will_value_ && will_expiry_interval) {
            tim_will_expiry_ = std::make_shared<as::steady_timer>(exe_, *will_expiry_interval);
            tim_will_expiry_->async_wait(
                [this, wp = std::weak_ptr<as::steady_timer>(tim_will_expiry_)]
                (error_code ec) {
                    if (auto sp = wp.lock()) {
                        if (!ec) {
                            clear_will();
                        }
                    }
                }
            );
        }
    }

    void clear_will() {
        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "clear will. cid:" << client_id_;
        tim_will_expiry_.reset();
        will_value_ = std::nullopt;
    }

    void send_will() {
        if (!will_value_) return;

        auto wd_sec =
            [&] () -> std::size_t {
                std::optional<property::will_delay_interval> wd_opt;
                for (auto const& prop : will_value_->props()) {
                    prop.visit(
                        overload {
                            [&] (property::will_delay_interval const& v) {
                                wd_opt.emplace(v);
                            },
                            [&](auto const&) {}
                        }
                    );
                    if (wd_opt) return wd_opt->val();
                }
                return 0;
            } ();

        if (remain_after_close_ && wd_sec != 0) {
            ASYNC_MQTT_LOG("mqtt_broker", trace)
                << ASYNC_MQTT_ADD_VALUE(address, this)
                << "set will_delay. cid:" << client_id_ << " delay:" << wd_sec;
            tim_will_delay_.expires_after(std::chrono::seconds(wd_sec));
            tim_will_delay_.async_wait(
                [this]
                (error_code ec) {
                    if (!ec) {
                        send_will_impl();
                    }
                }
            );
        }
        else {
            send_will_impl();
        }
    }

    void insert_inflight_message(
        store_packet_variant msg,
        std::shared_ptr<as::steady_timer> tim_message_expiry
    ) {
        std::lock_guard<mutex> g(mtx_inflight_messages_);
        inflight_messages_.insert(
            force_move(msg),
            force_move(tim_message_expiry)
        );
    }

    void send_inflight_messages() {
        if (auto epsp = lock()) {
            std::lock_guard<mutex> g(mtx_inflight_messages_);
            inflight_messages_.send_all_messages(epsp);
        }
    }

    void erase_inflight_message_by_expiry(std::shared_ptr<as::steady_timer> const& sp) {
        std::lock_guard<mutex> g(mtx_inflight_messages_);
        auto& idx = inflight_messages_.get<tag_tim>();
        auto [b, e] = idx.equal_range(sp);
        while (b != e) {
        ASYNC_MQTT_LOG("mqtt_broker", info)
            << "message expired:" << b->packet();
            b = idx.erase(b);
        }
    }

    std::size_t erase_inflight_message_by_packet_id(packet_id_type packet_id) {
        std::lock_guard<mutex> g(mtx_inflight_messages_);
        auto& idx = inflight_messages_.get<tag_pid>();
        return idx.erase(packet_id);
    }

    void send_all_offline_messages() {
        if (auto epsp = lock()) {
            std::lock_guard<mutex> g(mtx_offline_messages_);
            offline_messages_.send_until_fail(epsp, get_protocol_version());
        }
    }

    void send_offline_messages_by_packet_id_release() {
        if (auto epsp = lock()) {
            std::lock_guard<mutex> g(mtx_offline_messages_);
            offline_messages_.send_until_fail(epsp, get_protocol_version());
        }
    }

    protocol_version get_protocol_version() const {
        return version_;
    }

    std::string const& client_id() const {
        return client_id_;
    }

    std::string const& get_username() const {
        return username_;
    }

    void inherit(
        epsp_type epsp,
        std::optional<will> will,
        std::optional<std::chrono::steady_clock::duration> will_expiry_interval,
        std::optional<std::chrono::steady_clock::duration> session_expiry_interval
    ) {
        ASYNC_MQTT_LOG("mqtt_broker", info)
            << ASYNC_MQTT_ADD_VALUE(address, epsp.get_address())
            << "inherit";

        epwp_ = epsp;
        exe_ = epsp.get_executor();
        auto version = epsp.get_protocol_version();
        if (version == protocol_version::v3_1_1) {
            remain_after_close_= true;
        }
        else {
            BOOST_ASSERT(version == protocol_version::v5);
            remain_after_close_=
                session_expiry_interval_ &&
                *session_expiry_interval_ != std::chrono::steady_clock::duration::zero();
        }
        // for old will
        tim_will_delay_.cancel();
        clear_will();
        // for new will
        update_will(force_move(will), will_expiry_interval);

        session_expiry_interval_ = force_move(session_expiry_interval);
        epsp.restore_qos2_publish_handled_pids(qos2_publish_handled_);
    }

    epsp_type lock() {
        return epwp_.lock();
    }

    std::optional<std::chrono::steady_clock::duration> session_expiry_interval() const {
        return session_expiry_interval_;
    }

    void set_response_topic(std::string topic) {
        response_topic_.emplace(force_move(topic));
    }

    std::optional<std::string> get_response_topic() const {
        return response_topic_;
    }

    bool remain_after_close() const {
        return remain_after_close_;
    }

private:
    // constructor
    session_state(
        mutex& mtx_subs_map,
        sub_con_map<epsp_type>& subs_map,
        shared_target<epsp_type>& shared_targets,
        epsp_type epsp,
        std::string client_id,
        std::string const& username,
        will_sender_type will_sender,
        bool clean_start,
        std::optional<std::chrono::steady_clock::duration> session_expiry_interval)
        :exe_(epsp.get_executor()),
         mtx_subs_map_(mtx_subs_map),
         subs_map_(subs_map),
         shared_targets_(shared_targets),
         epwp_(epsp),
         version_(epsp.get_protocol_version()),
         client_id_(force_move(client_id)),
         username_(username),
         session_expiry_interval_(force_move(session_expiry_interval)),
         tim_will_delay_(exe_),
         will_sender_(force_move(will_sender)),
         remain_after_close_(
            [&] {
                if (version_ == protocol_version::v3_1_1) {
                    return !clean_start;
                }
                else {
                    BOOST_ASSERT(version_ == protocol_version::v5);
                    return
                        session_expiry_interval_ &&
                        *session_expiry_interval_ != std::chrono::steady_clock::duration::zero();
                }
            } ()
         )
    {
    }

    void send_will_impl() {
        if (!will_value_) return;

        ASYNC_MQTT_LOG("mqtt_broker", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "send will. cid:" << client_id_;

        auto topic = force_move(will_value_->topic());
        auto payload = force_move(will_value_->message_as_buffer());
        auto opts = will_value_->get_qos() | will_value_->get_retain();
        auto props = force_move(will_value_->props());
        properties forward_props;

        for (auto&& prop : props) {
            force_move(prop).visit(
                overload {
                    [&](property::will_delay_interval&&) {
                        // WillDelayInterval is not forwarded
                    },
                    [&](auto&& p) {
                        forward_props.push_back(force_move(p));
                    }
                }
            );
        }


        will_value_ = std::nullopt;
        if (tim_will_expiry_) {
            auto d =
                std::chrono::duration_cast<std::chrono::seconds>(
                    tim_will_expiry_->expiry() - std::chrono::steady_clock::now()
                ).count();
            if (d < 0) d = 0;

            bool set = false;
            for (auto& prop : forward_props) {
                prop.visit(
                    overload {
                        [&](property::message_expiry_interval& v) {
                            v = property::message_expiry_interval{
                                static_cast<uint32_t>(d)
                            };
                            set = true;
                        },
                        [&](auto&) {}
                    }
                );
                if (set) break;
            }
        }
        if (will_sender_) {
            will_sender_(
                *this,
                force_move(topic),
                std::vector{force_move(payload)},
                opts,
                force_move(forward_props)
            );
        }
    }

private:
    friend class session_states<epsp_type>;

    as::any_io_executor exe_;
    std::shared_ptr<as::steady_timer> tim_will_expiry_;
    std::optional<async_mqtt::will> will_value_;

    mutex& mtx_subs_map_;
    sub_con_map<epsp_type>& subs_map_;
    shared_target<epsp_type>& shared_targets_;
    epwp_type epwp_;
    protocol_version version_;
    std::string client_id_;

    std::string username_;

    std::optional<std::chrono::steady_clock::duration> session_expiry_interval_;
    std::shared_ptr<as::steady_timer> tim_session_expiry_ =
        std::make_shared<as::steady_timer>(exe_);

    mutable mutex mtx_inflight_messages_;
    inflight_messages inflight_messages_;

    mutable mutex mtx_offline_messages_;
    offline_messages offline_messages_;

    using elem_type = typename sub_con_map<epsp_type>::handle;
    std::set<elem_type> handles_; // to efficient remove

    as::steady_timer tim_will_delay_;
    will_sender_type will_sender_;
    bool remain_after_close_;

    std::set<packet_id_type> qos2_publish_handled_;

    std::optional<std::string> response_topic_;
    std::function<void()> clean_handler_;
};

template <typename Sp>
class session_states {
    using epsp_type = Sp;
    using epwp_type = typename epsp_type::weak_type;
public:
    template <typename Tag>
    decltype(auto) get() {
        return entries_.template get<Tag>();
    }

    template <typename Tag>
    decltype(auto) get() const {
        return entries_.template get<Tag>();
    }

    void clear() {
        entries_.clear();
    }

private:
    // The mi_session_online container holds the relevant data about an active connection with the broker.
    // It can be queried either with the clientid, or with the shared pointer to the mqtt endpoint object
    using ss_type = session_state<epsp_type>;
    using elem_type = std::shared_ptr<ss_type>;
    using mi_session_state = mi::multi_index_container<
        elem_type,
        mi::indexed_by<
            // con is nullable
            mi::ordered_non_unique<
                mi::tag<tag_con>,
                mi::key<&ss_type::epwp_>
            >,
            mi::ordered_unique<
                mi::tag<tag_cid>,
                mi::key<
                    &ss_type::username_,
                    &ss_type::client_id_
                >
            >,
            mi::ordered_non_unique<
                mi::tag<tag_tim>,
                mi::key<&ss_type::tim_session_expiry_>
            >
        >
    >;

    mi_session_state entries_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_SESSION_STATE_HPP
