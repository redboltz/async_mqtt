# Using stackless coroutine

This is stackless coroutine approach. It uses switch-case based Boost.Asio stackless corotuine.
See https://www.boost.org/doc/html/boost_asio/overview/composition/coroutine.html

It is convenient but a little tricky. Due to switch-case based, there are some restrictions especially define local variables.
If you can use [C++20 coroutine](cpp20_coro.md), I recommend use it.

stackless coroutine can avoid deeply nested callbacks. The code becomes easy to read.

## Prepare your application class

Here is a application class prototype:

```cpp
#include <boost/asio/yield.hpp>

struct app {
    // forwarding callbacks
    void operator()() {
        proc({}, {}, {}, {});
    }
    void operator()(boost::system::error_code const& ec) {
        proc(ec, {}, {}, {});
    }
    void operator()(boost::system::error_code ec, as::ip::tcp::resolver::results_type eps) {
        proc(ec, {}, {}, std::move(eps));
    }
    void operator()(boost::system::error_code ec, as::ip::tcp::endpoint /*unused*/) {
        proc(ec, {}, {}, {});
    }
    void operator()(am::system_error const& se) {
        proc({}, se, {}, {});
    }
    void operator()(am::packet_variant pv) {
        proc({}, {}, am::force_move(pv), {});
    }

private:
    void proc(
        boost::system::error_code const& ec,
        am::system_error const& se,
        am::packet_variant pv,
        std::optional<as::ip::tcp::resolver::results_type> eps
    ) {
        reenter (coro) {
        }
    }

    as::coroutine coro;
};

#include <boost/asio/unyield.hpp>
```

Then, add copyable member variables to access from `proc()` and initialize them by the constructor.

```cpp

struct app {
    app(as::ip::tcp::resolver& res,
        std::string host,
        std::string port,
        am::endpoint<am::role::client, am::protocol::mqtt>& amep
    ):res{res},
      host{std::move(host)},
      port{std::move(port)},
      amep{amep}
    {}
    
    // ...

private:
    void proc(
        boost::system::error_code const& ec,
        am::system_error const& se,
        am::packet_variant pv,
        std::optional<as::ip::tcp::resolver::results_type> eps
    ) {
        reenter (coro) {
        }
    }

    as::ip::tcp::resolver& res;
    std::string host;
    std::string port;
    am::endpoint<am::role::client, am::protocol::mqtt>& amep;
    std::size_t count = 0;    as::coroutine coro;
};
```

Then, create your application class instance and call the function call operator.

```cpp
    as::io_context ioc;

    // To get IP address from hostname
    as::ip::tcp::socket resolve_sock{ioc}; 
    as::ip::tcp::resolver res{resolve_sock.get_executor()};

    auto amep = am::endpoint<am::role::client, am::protocol::mqtt>::create(
        am::protocol_version::v3_1_1, // choose MQTT version v3_1_1 or v5
        ioc.get_executor() // args for underlying layer (mqtt)
        // mqtt is as::basic_stream_socket<as::ip::tcp, as::io_context::executor_type>
    );
    
    // Added these code
    app a{res, argv[1], argv[2], *amep};
    a();
    ioc.run();    
```

When `a()` is called then "start" is output.

```cpp
    void proc(
        boost::system::error_code const& ec,
        am::system_error const& se,
        am::packet_variant pv,
        std::optional<as::ip::tcp::resolver::results_type> eps
    ) {
        reenter (coro) {
            std::cout << "start" << std::endl;
        }
    }
```

This is the basic of stackless coroutine approach.
Now, adding more meaningful sequence.

## Resolve hostname (mqtt, mqtts, ws, wss)

If you use IP Address directly, you can skip resolve phase.

```cpp
    void proc(
        boost::system::error_code const& ec,
        am::system_error const& se,
        am::packet_variant pv,
        std::optional<as::ip::tcp::resolver::results_type> eps
    ) {
        reenter (coro) {
            std::cout << "start" << std::endl;

            // Resolve hostname
            yield res.async_resolve(host, port, *this);
            // The function finish here, and when async_resolve is finished
            // resume at the next line.
            std::cout << "async_resolve:" << ec.message() << std::endl;
            if (ec) return;
```

The important point is

```cpp
            yield res.async_resolve(host, port, *this);
```

The third argument of [async_resolve](https://www.boost.org/doc/html/boost_asio/reference/ip__basic_resolver/async_resolve/overload2.html) is CompletionToken. In this case, asio document said it ResolveToken. When we use stackless coroutine, we pass `*this` as the CompletionToken. The function proc() is implicitly returned and async_resolve starts processing.
When async process (resolving hostname) is finished, then the following operator() is called:

```cpp
    void operator()(boost::system::error_code ec, as::ip::tcp::resolver::results_type eps) {
        proc(ec, {}, {}, std::move(eps));
    }
```

Then, proc is called. You can distinguish which async process is finished by proc()'s parameter.
You can check `ec` as follows:

```cpp
            std::cout << "async_resolve:" << ec.message() << std::endl;
            if (ec) return;
```

Even if proc() is called again, the following part of the code is skipped:

```cpp
            std::cout << "start" << std::endl;

            // Resolve hostname
            yield res.async_resolve(host, port, *this);
```

This is switch-case based Boost.Asio stackless coroutine mechanism.
See https://www.boost.org/doc/html/boost_asio/overview/composition/coroutine.html

## TCP connect (mqtt, mqtts, ws, wss)

Now, hostname is resolved. The next step is making TCP connection.

```cpp
            // Underlying TCP connect
            yield as::async_connect(
                amep.next_layer(), // or amep.lowest_layer()
                *eps,
                *this
            );
            std::cout
                << "TCP connected ec:"
                << ec.message()
                << std::endl;

            if (ec) return;
```

## TLS handshake and WS handshake
See [Examples](#Examples)

## Send MQTT CONNECT packet

Create MQTT CONNECT packet and send it as follows:

```cpp
            // Send MQTT CONNECT
            yield amep.send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    am::allocate_buffer("cid1"),
                    am::nullopt, // will
                    am::nullopt, // username set like am::allocate_buffer("user1"),
                    am::nullopt  // password set like am::allocate_buffer("pass1")
                },
                *this
            );
```

When async process is finished the function resumes at the following line:

```cpp
            if (se) {
                std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
                return;
            }
```

The parameter of the completion token is `system_error const& se`.
See [API reference](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html).

## Recv MQTT CONNACK packet

Receive MQTT packet as follows:

```cpp
            // Recv MQTT CONNACK
            yield amep.recv(*this);
```

When a packet is received then the function resumes at the following line:

```cpp
            if (pv) {
                pv.visit(
                    am::overload {
                        [&](am::v3_1_1::connack_packet const& p) {
                            std::cout
                                << "MQTT CONNACK recv"
                                << " sp:" << p.session_present()
                                << std::endl;
                        },
                        [](auto const&) {}
                    }
                );
            }
            else {
                std::cout
                    << "MQTT CONNACK recv error:"
                    << pv.get<am::system_error>().what()
                    << std::endl;
                return;
            }
```

The parameter of the completion token is `packet_variant pv`. You can access the `pv` using visit function and overloaded lamnda expressions. Each lambda expression is corresponding to the actual packet type.
`pv` can be evalurated as bool. If any receive error happens then `pv` evaluated as false, otherwise true.

## Send/Recv packets
See the simple example [ep_slcoro_mqtt_client.cpp](../../main/example/ep_slcoro_mqtt_client.cpp).

If you want to know more complex usecase, [client_cli.cpp](../../main/tool/client_cli.cpp) is helpful.
This is commandline MQTT client application.

# Examples
- [ep_slcoro_mqtt_client.cpp](../../main/example/ep_slcoro_mqtt_client.cpp)
- [ep_slcoro_mqtts_client.cpp](../../main/example/ep_slcoro_mqtts_client.cpp)
- [ep_slcoro_ws_client.cpp](../../main/example/ep_slcoro_ws_client.cpp)
- [ep_slcoro_wss_client.cpp](../../main/example/ep_slcoro_wss_client.cpp)
