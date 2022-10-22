// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_SHARED_TARGET_HPP)
#define ASYNC_MQTT_BROKER_SHARED_TARGET_HPP

#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/time_point_t.hpp>

#include <async_mqtt/broker/session_state_fwd.hpp>
#include <async_mqtt/broker/tags.hpp>
#include <async_mqtt/broker/mutex.hpp>
#include <async_mqtt/broker/subscription.hpp>

namespace async_mqtt {

namespace mi = boost::multi_index;

template <typename Sp>
class shared_target {
public:
    void insert(buffer share_name, buffer topic_filter, subscription<Sp> sub, session_state<Sp>& ss);
    void erase(buffer share_name, buffer topic_filter, session_state<Sp> const& ss);
    void erase(session_state<Sp> const& ss);
    optional<std::tuple<session_state_ref<Sp>, subscription<Sp>>> get_target(buffer const& share_name, buffer const& topic_filter);

private:
    struct entry {
        entry(buffer share_name, session_state<Sp>& ss, time_point_t tp);

        buffer const& client_id() const;
        buffer share_name;
        session_state_ref<Sp> ssr;
        time_point_t tp;
        std::map<buffer, subscription<Sp>> tf_subs;
    };

    using mi_shared_target = mi::multi_index_container<
        entry,
        mi::indexed_by<
            mi::ordered_unique<
                mi::tag<tag_cid_sn>,
                mi::key<&entry::client_id, &entry::share_name>
            >,
            mi::ordered_non_unique<
                mi::tag<tag_sn_tp>,
                mi::key<&entry::share_name, &entry::tp>
            >
        >
    >;

    mutable mutex mtx_targets_;
    mi_shared_target targets_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_SHARED_TARGET_HPP
