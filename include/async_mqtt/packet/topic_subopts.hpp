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
    ): topic_length_buf_{endian_static_vector(boost::numeric_cast<std::uint16_t>(topic.size()))},
       topic_{force_move(topic)},
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
        endian_store(boost::numeric_cast<std::uint16_t>(topic_.size()), topic_length_buf_.data());
    }

    void set_opts(sub::opts val) {
        opts_ = val;
    }

    static_vector<char, 2> const& topic_length_buf() const {
        return topic_length_buf_;
    }
private:
    static_vector<char, 2> topic_length_buf_;
    buffer topic_;
    sub::opts opts_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_TOPIC_SUBOPTS_HPP
