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

