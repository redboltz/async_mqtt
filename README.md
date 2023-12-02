# Index
- [Run on the docker container](#run-on-the-docker-container)
- [Free trial broker on cloud](#Free-trial-broker-on-cloud)
- Tutorial
  - [Create Endpoint](tutorial/create_endpoint.md)
  - [Stackless Coroutine](tutorial/sl_coro.md)
- [Performance](#performance)

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

# Performance
I've measured how many publishes per second can async_mqtt broker treated.

## Environment
AWS EC2 c5.4xlarge, c5.12xlarge

- Benchmark targets
  - mosquitto version:2.0.11
  - hivemqt-ce-2023.4
  - async_mqtt 1.0.6

Single broker, multiple clients.
clients are genereted by https://github.com/redboltz/async_mqtt/tree/main/docker bench.sh

It publishes packets and receive it. Measure RTT(Round trip time).
Each clinet 100 publish/second (pps). Increase the number of clients until RTT over 1second.
For example, the number of client is 6,000, that means 600,000 (600K) pps.
Publish payload is 1024 bytes.

## Result

### QoS0
ec2\broker|mosquitto|HiveMQ-CE|async_mqtt
---|---|---|---
c5.4xlarge|90|53|250
c5.12xlarge|90|66|610

value is Kpps (Kilo publish per second)

### QoS1
ec2\broker|mosquitto|HiveMQ-CE|async_mqtt
---|---|---|---
c5.4xlarge|52|36|155
c5.12xlarge|52|58|390

value is Kpps (Kilo publish per second)

### QoS2
ec2\broker|mosquitto|HiveMQ-CE|async_mqtt
---|---|---|---
c5.4xlarge|28|24|89
c5.12xlarge|28|38|210

value is Kpps (Kilo publish per second)

![bench.png](img/bench.png)
