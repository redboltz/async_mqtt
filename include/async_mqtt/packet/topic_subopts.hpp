// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP)
#define ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP

#include <async_mqtt/packet/topic_sharename.hpp>
#include <async_mqtt/packet/subopts.hpp>

namespace async_mqtt {

class topic_subopts {
public:
    topic_subopts(
        buffer all_topic,
        sub::opts opts
    ): topic_sharename_{force_move(all_topic)},
       opts_{opts}
    {
    }

    buffer const& topic() const {
        return topic_sharename_.topic();
    }

    buffer const& sharename() const {
        return topic_sharename_.sharename();
    }

    sub::opts const& opts() const { // return reference in mandatory
        return opts_;
    }

    operator bool() const {
        return static_cast<bool>(topic_sharename_);
    }

    friend
    bool operator<(topic_subopts const& lhs, topic_subopts const& rhs) {
        return
            std::tie(lhs.topic_sharename_, lhs.opts_) <
            std::tie(rhs.topic_sharename_, rhs.opts_);
    }

    friend
    bool operator==(topic_subopts const& lhs, topic_subopts const& rhs) {
        return
            std::tie(lhs.topic_sharename_, lhs.opts_) ==
            std::tie(rhs.topic_sharename_, rhs.opts_);
    }

private:
    topic_sharename topic_sharename_;
    sub::opts opts_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP
