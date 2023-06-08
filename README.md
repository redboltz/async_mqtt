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
