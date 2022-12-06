// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP)
#define ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/packet/subopts.hpp>

namespace async_mqtt {

class topic_subopts {
public:
    topic_subopts(
        buffer all_topic,
        sub::opts opts
    ): all_topic_{force_move(all_topic)},
       opts_{opts}
    {
        BOOST_ASSERT(topic_.size() <= 0xffff);
        auto const shared_prefix = string_view("$share/");
        if (all_topic_.substr(0, shared_prefix.size()) == shared_prefix) {
            sharename_ = all_topic_.substr(shared_prefix.size());

            auto const idx = sharename_.find_first_of('/');
            if (idx == string_view::npos) return;

            topic_ = sharename_.substr(idx + 1);
            sharename_.remove_suffix(sharename_.size() - idx);
        }
        else {
            topic_ = all_topic;
        }
    }

    buffer const& topic() const {
        return topic_;
    }

    buffer const& sharename() const {
        return sharename_;
    }

    sub::opts const& opts() const { // return reference in mandatory
        return opts_;
    }

    friend
    bool operator<(topic_subopts const& lhs, topic_subopts const& rhs) {
        return
            std::tie(lhs.topic_, lhs.opts_) <
            std::tie(rhs.topic_, rhs.opts_);
    }

    friend
    bool operator==(topic_subopts const& lhs, topic_subopts const& rhs) {
        return
            std::tie(lhs.topic_, lhs.opts_) ==
            std::tie(rhs.topic_, rhs.opts_);
    }

private:
    buffer all_topic_;
    buffer topic_;
    buffer sharename_;
    sub::opts opts_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP
