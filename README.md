# async_mqtt

Asynchronous MQTT communication library.

Version 8.0.1 [![Actions Status](https://github.com/redboltz/async_mqtt/workflows/CI/badge.svg)](https://github.com/redboltz/async_mqtt/actions)[![codecov](https://codecov.io/gh/redboltz/async_mqtt/branch/main/graph/badge.svg)](https://codecov.io/gh/redboltz/async_mqtt)

This is Boost.Asio oriented asynchronous MQTT communication library. You can use async_mqtt to develop not only your MQTT client application but also your server (e.g. broker).
Based on https://github.com/redboltz/mqtt_cpp experience, there are many improvements. See overview.

Document is https://github.com/redboltz/async_mqtt/blob/doc/README.adoc

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

## High level MQTT client APIs support (since 5.1.0)

See [document](https://redboltz.github.io/async_mqtt/doc/latest/tutorial/client.html) and [example](example/cl_cpp20coro_mqtt.cpp).

## Features

- Not only client but also server is supported.
  - endpoint(basic_endpoint) can be used to implement MQTT client application and/or server application like MQTT broker.
- Both MQTT v3.1.1 and v5.0 are supported.
  - In addition, undetermined is supported for server implementation. In this case, the protocol version is decided when you receive CONNECT packet.
- Continuous packet sending
  - You can send MQTT packets before the previous sending is not completed.
- Auto acquiring/mapping topic alias is supported.
  - In addition,  when resend PUBLISH packet after reconnecting, topic alias is removed and original topic is restored. It is required by MQTT v5.0 spec.

## Requirement

- Compiler: C++17 or later
- Boost Libraries:  1.81.0 or later
