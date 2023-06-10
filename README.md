# Index
- [Run on the docker container](#run-on-the-docker-container)
- [Tutorial](#tutorial)
# Run on the docker container
You can run async_mqtt broker, benchmarking tool, and CLI client using docker.

See https://github.com/redboltz/async_mqtt/tree/main/docker

## Quick start

### Preparation

```
git clone https://github.com/redboltz/async_mqtt.git
cd async_mqtt
cd docker
```

### Run broker

```
./broker.sh
```

If you don't have executing docker permission, then run as follows:

```
sudo ./broker.sh
```

### Run client_cli

On another terminal:

```
./client_cli.sh --host host.docker.internal
```

If you don't have executing docker permission, then run as follows:

```
sudo ./client_cli.sh --host host.docker.internal
```

Then, you can communicate to the broker container on the same machine.

You can subscribe and publish as follows:

```
cli> sub t1 1
cli> pub t1 hello 1
cli> exit
```

Red colored message is command response.
Cyan colored message is received packet.

Also you can connect mosquitto.org as follows.

```
./client_cli --host test.mosquitto.rog
```

# Tutorial

## Create endpoint

First, choose underlying layer.
![layer structure](img/layer.svg)

### mqtt

```cpp
    as::io_context ioc;

    // To get IP address from hostname
    as::ip::tcp::socket resolve_sock{ioc}; 
    as::ip::tcp::resolver res{resolve_sock.get_executor()};

    //         endpoint is client  choose underlying layer
    am::endpoint<am::role::client, am::protocol::mqtt> amep {
        am::protocol_version::v3_1_1, // choose MQTT version v3_1_1 or v5
        ioc.get_executor() // args for underlying layer (mqtt)
        // mqtt is as::basic_stream_socket<as::ip::tcp, as::io_context::executor_type>
    };
```

### mqtts

```cpp
    as::io_context ioc;
    
    // To get IP address from hostname
    as::ip::tcp::socket resolve_sock{ioc};
    as::ip::tcp::resolver res{resolve_sock.get_executor()};

    am::tls::context ctx{am::tls::context::tlsv12};
    ctx.set_verify_mode(am::tls::verify_none);
    // If you want to check server certificate, set cacert as follows.
    // ctx.load_verify_file(cacert);
  
    am::endpoint<am::role::client, am::protocol::mqtts> amep {
        am::protocol_version::v5, // choose MQTT version v3_1_1 or v5
        ioc.get_executor(),  // args for underlying layer (as::ssl::stream<mqtt>)
        ctx
    };
```

### ws

```cpp
    as::io_context ioc;

    // To get IP address from hostname
    as::ip::tcp::socket resolve_sock{ioc};
    as::ip::tcp::resolver res{resolve_sock.get_executor()};

    am::endpoint<am::role::client, am::protocol::ws> amep {
        am::protocol_version::v3_1_1, // choose MQTT version v3_1_1 or v5
        ioc.get_executor()  // args for underlying layer (bs::websocket::stream<mqtt>)
    };
```


### wss

```cpp
    as::io_context ioc;

    // To get IP address from hostname
    as::ip::tcp::socket resolve_sock{ioc};
    as::ip::tcp::resolver res{resolve_sock.get_executor()};
    
    am::tls::context ctx{am::tls::context::tlsv12};
    ctx.set_verify_mode(am::tls::verify_none);
    // If you want to check server certificate, set cacert as follows.
    // ctx.load_verify_file(cacert);
    
    am::endpoint<am::role::client, am::protocol::wss> amep {
        am::protocol_version::v3_1_1, // choose MQTT version v3_1_1 or v5
        ioc.get_executor(),  // args for underlying layer ( bs::websocket::stream<mqtts>)
        ctx                  // mqtts is as::ssl::stream<mqtt>
    };
```

## Using stackless coroutine

This is stackless coroutine approach.

### Prepare your application class

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

    //         endpoint is client  choose underlying layer
    am::endpoint<am::role::client, am::protocol::mqtt> amep {
        am::protocol_version::v3_1_1, // choose MQTT version v3_1_1 or v5
        ioc.get_executor() // args for underlying layer (mqtt)
        // mqtt is as::basic_stream_socket<as::ip::tcp, as::io_context::executor_type>
    };
    
    // Added these code
    app a{res, argv[1], argv[2], amep};
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

### Resolve hostname (mqtt, mqtts, ws, wss)

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

### TCP connect (mqtt, mqtts, ws, wss)

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

Layer access | mqtt | mqtts | ws | wss
---|---|---|---|---
next_layer()|TCP stream|TLS stream| WS stream | WS stream
next_layer()->next_layer()|-|TCP stream|TCP stream | TLS stream
next_layer()->next_layer()->next_layer()|-|-|-|TCP stream
lowest_layer()|TCP stream|TCP stream|TCP stream|TCP stream


