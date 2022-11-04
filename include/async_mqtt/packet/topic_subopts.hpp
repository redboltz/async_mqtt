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
        buffer topic,
        sub::opts opts
    ): topic_{force_move(topic)},
       opts_{opts}
    {
        BOOST_ASSERT(topic_.size() <= 0xffff);
    }

    buffer const& topic() const {
        return topic_;
    }

    sub::opts const& opts() const { // return reference in mandatory
        return opts_;
    }

    void set_topic(buffer topic) {
        BOOST_ASSERT(topic.size() <= 0xffff);
        topic_ = force_move(topic);
    }

    void set_opts(sub::opts val) {
        opts_ = val;
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
    buffer topic_;
    sub::opts opts_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP
