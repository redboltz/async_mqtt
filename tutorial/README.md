# Tutorial
## Create endpoint

First, choose underlying layer.
![layer structure](../img/layer.svg)

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

NOTE: `tls` is namespace alias of `boost::asio::ssl` by default.

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

## Layer access

Layer access | mqtt | mqtts | ws | wss
---|---|---|---|---
next_layer()|TCP stream|TLS stream| WS stream | WS stream
next_layer().next_layer()|-|TCP stream|TCP stream | TLS stream
next_layer().next_layer().next_layer()|-|-|-|TCP stream
lowest_layer()|TCP stream|TCP stream|TCP stream|TCP stream

