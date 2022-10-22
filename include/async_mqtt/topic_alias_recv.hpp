// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TOPIC_ALIAS_RECV_HPP)
#define ASYNC_MQTT_TOPIC_ALIAS_RECV_HPP

#include <string>
#include <unordered_map>
#include <array>

#include <boost/assert.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>

#include <async_mqtt/constant.hpp>
#include <async_mqtt/type.hpp>
#include <async_mqtt/log.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/string_view.hpp>

namespace async_mqtt {

namespace mi = boost::multi_index;

class topic_alias_recv {
public:
    topic_alias_recv(topic_alias_t max)
        :max_{max} {}

    void insert_or_update(string_view topic, topic_alias_t alias) {
        ASYNC_MQTT_LOG("mqtt_impl", trace)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "topic_alias_recv insert"
            << " topic:" << topic
            << " alias:" << alias;
        BOOST_ASSERT(!topic.empty() && alias >= min_ && alias <= max_);
        auto it = aliases_.lower_bound(alias);
        if (it == aliases_.end() || it->alias != alias) {
            aliases_.emplace_hint(it, std::string(topic), alias);
        }
        else {
            aliases_.modify(
                it,
                [&](entry& e) {
                    e.topic = std::string{topic};
                },
                [](auto&) { BOOST_ASSERT(false); }
            );

        }
    }

    std::string find(topic_alias_t alias) const {
        BOOST_ASSERT(alias >= min_ && alias <= max_);
        std::string topic;
        auto it = aliases_.find(alias);
        if (it != aliases_.end()) topic = it->topic;

        ASYNC_MQTT_LOG("mqtt_impl", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "find_topic_by_alias"
            << " alias:" << alias
            << " topic:" << topic;

        return topic;
    }

    void clear() {
        ASYNC_MQTT_LOG("mqtt_impl", info)
            << ASYNC_MQTT_ADD_VALUE(address, this)
            << "clear_topic_alias";
        aliases_.clear();
    }

    topic_alias_t max() const { return max_; }

private:
    static constexpr topic_alias_t min_ = 1;
    topic_alias_t max_;

    struct entry {
        entry(std::string topic, topic_alias_t alias)
            : topic{force_move(topic)}, alias{alias} {}

        std::string topic;
        topic_alias_t alias;
    };
    using mi_topic_alias = mi::multi_index_container<
        entry,
        mi::indexed_by<
            mi::ordered_unique<
                mi::key<&entry::alias>
            >
        >
    >;

    mi_topic_alias aliases_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_TOPIC_ALIAS_RECV_HPP
