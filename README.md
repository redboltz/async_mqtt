# async_mqtt

Asynchronous MQTT communication library.

Version 1.0.9 [![Actions Status](https://github.com/redboltz/async_mqtt/workflows/CI/badge.svg)](https://github.com/redboltz/async_mqtt/actions)[![codecov](https://codecov.io/gh/redboltz/async_mqtt/branch/main/graph/badge.svg)](https://codecov.io/gh/redboltz/async_mqtt)

This is Boost.Asio oriented asynchronous MQTT communication library. You can use async_mqtt to develop not only your MQTT client application but also your server (e.g. broker).
Based on https://github.com/redboltz/mqtt_cpp experience, there are many improvements. See overview.

You can run prebuilt tools using docker.
Also you can find tutorials.
See https://github.com/redboltz/async_mqtt/blob/doc/README.md.

# Overview

[API Reference](https://redboltz.github.io/async_mqtt)

## Boost.Asio style asynchronous APIs support

### [Completion Token](https://www.boost.org/doc/html/boost_asio/overview/model/completion_tokens.html) is supported
- Callbacks
  - Can wait mutiple events
  - High performance
  - Deep callback nesting
  - For simple and continuous multiple waiting usecases
  - [example/ep_cb_mqtt_client.cpp](example/ep_cb_mqtt_client.cpp)
- [`boost::asio::use_future`](https://www.boost.org/doc/html/boost_asio/overview/composition/futures.html)
  - Can't wait multiple events
  - Less performance than callbacks and stackless coroutines
  - Easy to read
  - For simple and straight usecases
  - [example/ep_future_mqtt_client.cpp](example/ep_future_mqtt_client.cpp)
- [Stackless Coroutine (`boost::asio::coroutine`)](https://www.boost.org/doc/html/boost_asio/overview/composition/coroutine.html)
  - Can wait multiple event. You can distinguish which event is invoked by operator()'s overloaded parameter.
  - High performance
  - Easy to read but difficult to learn `yield` notation
  - [example/ep_slcoro_mqtt_client.cpp](example/ep_slcoro_mqtt_client.cpp)
- [C++20Coroutine](https://www.boost.org/doc/html/boost_asio/overview/composition/cpp20_coroutines.html)
  - Can wait multiple event.
  - High performance. (But a little bit slower than Stackless Coroutine)
  - Easy to read.
  - [example/ep_slcoro_mqtt_client.cpp](example/ep_cpp20coro_mqtt_client.cpp)
- [and more](https://www.boost.org/doc/html/boost_asio/overview/composition.html)

I recommend using [Stackless Coroutine (`boost::asio::coroutine`)](https://www.boost.org/doc/html/boost_asio/overview/composition/coroutine.html) because it can avoid deep nested callbacks and higher performance than [`boost::asio::use_future`](https://www.boost.org/doc/html/boost_asio/overview/composition/futures.html). C++20 Coroutine is also a good choice. It requires C++20 support. It is more elegant than Stackless Coroutine but a little bit slower than Stackless coroutine.

### recv() funtion
You need to call recv() function when you want to receive MQTT packet. It is similar to Boost.Asio read() function.
You can control packet receiving timing. async_mqtt doesn't use handler registering style APIs such as `set_publish_handler()`. If you need handler registering APIs, you can create them using recv().
recv() function is more flexible than handler registering APIs. In addition, it works well with [Completion Token](https://www.boost.org/doc/html/boost_asio/overview/model/completion_tokens.html).

You cannot call recv() function continuously. You can call the next recv() function after the previous recv() function's CompletionToken is invoked or in the Completion handler.

#### packet_variant
recv()'s CompletionToken parameter is [packet_variant](/include/async_mqtt/packet/packet_variant.hpp). It is a variant type of all MQTT packets and error.

NOTE: async_mqtt has basic_foobar type and foobar type if the type contains MQTT's Packet Identifier. basic_foobar takes PacketIdBytes parameter. basic_foobar<2> is the same as foobar. MQTT spec defines the size of Packet Identifier to 2. But some of clustering brokers use expanded Packet Identifer for inter brokers communication. General users doesn't need to care basic_foobar types, simply use foobar.

You can access packet_variant as follows:

```cpp
namespace am = async_mqtt; // always use this namespace alias in this document
```

```cpp
am::packet_variant pv = ...; // from CompletionToken
if (pv) { // pv is packet_variant
     pv.visit(
         am::overload {
              [&](am::v3_1_1::connack_packet const& p) {
                  std::cout
                      << "MQTT CONNACK recv "
                      << "sp:" << p.session_present()
                      << std::endl;
              },
              // other packets handling code here
              [](auto const&) {}
         }
    );
}
else {
    std::cout
         << "MQTT CONNACK recv error:"
         << pv.get<am::system_error>().what()
         << std::endl;
}
```

`pv` can evaluated as bool. If `pv` has valid packet then it is convert to true, otherwise false. Typically, when underlying socket is disconnected, `pv` contains error and it is evaluated as false.

#### Control Packet Type filter
You might interested in the specific packets. Your application doesn't want to care non important packet like pingresp, puback, pubrec, pubrel, and pubcomp packets.

You can filter packets as follows:

```cpp
// ep is endpoint
ep.recv(am::filter::match, {am::control_packet_type::publish}, completion_token);
```

When you set `filter::match` as the first argument, the second parameter is a list of matching MQTT Control Packet types. If unmatched packets are received, completion_token isn't invoked but received packets are apropriately proccessed.
If error is happened, completion_token is invoked with packet_variant that contains error.

```cpp
// ep is endpoint
ep.recv(am::filter::except, {am::control_packet_type::pingresp, am::control_packet_type::puback}, completion_token);
```

When you set `filter::except` as the first argument, the second parameter is a list of ignoring MQTT Control Packet types. If the packets int the list are received, completion_token isn't invoked but received packets are apropriately proccessed.
If error is happened, completion_token is invoked with packet_variant that contains error.


### send() function
MQTT has various packet types for example CONNECT, PUBLISH, SUBSCRIBE and so on. In order to send the packet, first create the packet and then pass it as send() parameter. If you send timing is a protocol error then the send() CompletionToken is invoked with system_error.
You can call send() continuously. async_mqtt endpoint has queuing mechanism. When the previous send() function's CompletionToken is invoked, then the next packet in the queue is sent if exists.


### Customizable underlying layer

You can use TCP(mqtt), TLS(mqtts), WebSocket(ws), TLS+WebSocket(wss). They are [predefined](predefined_underlying_layer.hpp).
In addition, you can use any underlying layer that is compatible to `boost::asio::ip::tcp::socket`.


TLS, WS configuration and handshaking are separated from async_mqtt core.

### Packet Based APIs

async_mqtt automatically update endpoint's interenal state when packet sending and receiving. For example, When you send CONNECT packet with maximum_packet_size property, endpoint set maximum packet size for receiving. See the packet and property API reference.

### Non Packet Based APIs
Some of functionalites are not corresponding to packet.
endpoint member function | Effects
---|---
set_auto_pub_response|If set true, then PUBACK, PUBREC, PUBREL, and PUBCOMP will be sent automatically when the corresponding packet is received.
set_auto_ping_response|If set true, then PINGRESP will be sent automatically when PINGREQ is received.
set_auto_map_topic_alias_send|If set true, TopicAlias is automatically acquired and applied on sending PUBLISH packets. The limit is decidec by received TopicAliasMaximum property. If it is 0, no TopicAlias is used. If TopicAlias is fully used, then overwrite the oldest TopicAlias (LRU algorithm).
set_auto_replace_topic_alias_send|It is similar to set_auto_map_topic_alias but not automatically acquired. So you need to register topicalias by yourself. If set true, then TopicAlias is automatically applied if TopicAlias is already registered.
set_ping_resp_recv_timeout_ms|Set timer after sending PINGREQ packet. The timer would be cancelled when PINGRESP packet is received. If timer is fired then the connection is disconnected automatically.

### Strand
async_mqtt endpoint has internal strand. The CompletionToken is called in the strand excpet you set your custom associated executor to ke CompletionToken. In the strand, you can call some of sync APIs. If you call those APIs out of strand, then assertion failed.

endpoint member function | effects
---|---
acquire_unique_packet_id|Acquire the new unique packet_id
register_packet_id|Register the packet_id
release_packet_id|Release the packet_id
get_qos2_publish_handled_pids|Get already PUBLISH recv CompletionToken is invoked packet_ids
restore_qos2_publish_handled_pids|Restore already PUBLISH recv CompletionToken is invoked packet_ids
restore_packets|Restore pacets as stored packets
get_stored_packets|Get stored packets
get_protocol_version|Get MQTT protocol version

acquire_unique_packet_id, register_packet_id, and release_packet_id have async version (the same name overload) to call out of strand.


## Config
### cmake
Flag | Effects
---|---
ASYNC_MQTT_USE_TLS|Enables TLS
ASYNC_MQTT_USE_WS|Enables Websockets (compilation time becomes longer)
ASYNC_MQTT_USE_LOG|Enable logging via Boost.Log
ASYNC_MQTT_BUILD_UNIT_TESTS|Build unit tests
ASYNC_MQTT_BUILD_SYSTEM_TESTS|Build system tests. The system tests requires broker.
ASYNC_MQTT_BUILD_TOOLS|Build tools (broker, bench, etc)
ASYNC_MQTT_BUILD_EXAMPLES|Build examples

### C++ preprocessor macro

Flag | Effects
---|---
ASYNC_MQTT_USE_TLS|Enables TLS
ASYNC_MQTT_USE_WS|Enables Websockets (compilation time becomes longer)
ASYNC_MQTT_USE_LOG|Enable logging via Boost.Log

## Requirement

- Compiler: C++17 or later
- Boost Libraries:  1.81.0 or later
