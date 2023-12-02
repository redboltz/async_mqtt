# Index
- [Run on the docker container](#run-on-the-docker-container)
- [Free trial broker on cloud](#Free-trial-broker-on-cloud)
- Tutorial
  - [Create Endpoint](tutorial/create_endpoint.md)
  - [Stackless Coroutine](tutorial/sl_coro.md)
- [Performance](performance,md)

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

# Free trial broker on cloud

You can connect to the broker `async-mqtt.redboltz.net` for testing.

port|protocol|client_cli.sh access
---|---|---
1883|MQTT on TCP|./client_cli.sh --host async-mqtt.redboltz.net --port 1883 --protocol mqtt
8883|MQTT on TLS|./client_cli.sh --host async-mqtt.redboltz.net --port 8883 --protocol mqtts
10080|MQTT on Websocket|./client_cli.sh --host async-mqtt.redboltz.net --port 10080 --protocol ws
10443|MQTT on Websocket(TLS)|./client_cli.sh --host async-mqtt.redboltz.net --port 10443 --protocol wss

## Important notice
You are free to use it for any application, but please do not abuse or rely upon it for anything of importance. This server runs on an very low spec vps. It is not intended to demonstrate any performance characteristics.
Please don't publish anything sensitive, anybody could be listening.



## Examples
- [ep_slcoro_mqtt_client.cpp](../main/example/ep_slcoro_mqtt_client.cpp)
- [ep_slcoro_mqtts_client.cpp](../main/example/ep_slcoro_mqtts_client.cpp)
- [ep_slcoro_ws_client.cpp](../main/example/ep_slcoro_ws_client.cpp)
- [ep_slcoro_wss_client.cpp](../main/example/ep_slcoro_wss_client.cpp)

