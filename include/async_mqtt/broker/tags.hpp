// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_TAGS_HPP)
#define ASYNC_MQTT_BROKER_TAGS_HPP

namespace async_mqtt {

struct tag_seq {};
struct tag_con {};
struct tag_topic{};
struct tag_topic_filter{};
struct tag_con_topic_filter {};
struct tag_cid {};
struct tag_cid_topic_filter {};
struct tag_tim {};
struct tag_pid {};
struct tag_sn_tp {};
struct tag_cid_sn {};

} // namespace async_mqtt

#endif // ASYNC_MQTT_BROKER_TAGS_HPP
