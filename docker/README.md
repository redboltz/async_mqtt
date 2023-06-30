# Docker container

**Important note**
I tried to migrate the base docker image from ubuntu to alpine. At first, it looks working well. But when I do some performance test, I got very poor performance on the alpine version.
I investigate a little and I found
https://superuser.com/questions/1219609/why-is-the-alpine-docker-image-over-50-slower-than-the-ubuntu-image

The docker images are ubuntu -> alpine -> ubuntu(now).

## Launch script

### broker.sh

Parameter: `[version] [broker_params...]`

Example:

Command|version|broker params
---|---|---
./broker|latest|none(no override broker.conf)
./broker 1.0.0 --verbose 5|1.0.0|override --verbose to 5
./broker --verbose 5|latest|override --verbose to 5

### bench.sh

Parameter: `[version] [broker_params...]`

Example:

Command|version|bench params
---|---|---
./bench|latest|none(no override bench.conf)
./bench 1.0.0 --verbose 5|1.0.0|override --verbose to 5
./bench --verbose 5|latest|override --verbose to 5

### client_cli.sh

Parameter: `[version] [broker_params...]`

Example:

Command|version|client_cli params
---|---|---
./client_cli|latest|none(no override cli.conf)
./client_cli 1.0.0 --verbose 5|1.0.0|override --verbose to 5
./client_cli --verbose 5|latest|override --verbose to 5

#### client_cli menu
- cli
  - pub
    - publish
    - params: topic payload qos
  - sub
    - subscribe
    - params: topic_filter qos
  - unsub
    - unsubscribe
    - params: topic_filter
  - bpub
    - build publish (sub menu)
       - topic: set topic
       - payload: set payload
       - retain: set retain
       - qos: set qos
       - send: send publish packet
       - show: show publish packet
       - clear: clear publish packet to default
       - cli: back to upper menu
  - bsub
    - build subscribe (sub menu)
       - topic: set topic
       - qos: set qos
       - nl: set no local
       - rap: set retain as published
       - rh: set retain handling
       - sid: set subscribe identifier property
       - send: send subscribe packet
       - show: show subscribe packet
       - clear: clear subscribe packet to default
       - cli: back to upper menu

help: show menu
exit: exit program


## Broker port mapping

Protocol | Host | Container
---|---|---
mqtt|1883|1883
mqtts|8883|8883
ws|10080|10080
wss|10443|10443

The ports on the host need to be usable.
You can change the ports of the host by editing `broker.sh`.

## Access from bench, client_cli (container) to the host

`host.docker.internal` is the name of host.

When you run `./broker.sh` in your PC (host), you can run `./bench.sh --host host.docker.internal` / `./client_cli.sh --host host.docker.internal`, then bench/client_cli container can access to the broker container.

## Config

`conf/` directory is mapped to the container.

### Broker

- conf/broker.conf
  - broker config file
- conf/auth.json
  - broker auth file
- conf/server.crt.pem
  - example server certificate file
- conf/server.key.pem
  - example server certificate key file

### Bench

- conf/bench.conf
  - bench config file
- conf/cacert.pem
  - verify file. verifier is bench. verify target is broker.

### Client CLI

- conf/cli.conf
  - client_cli config file
- conf/cacert.pem
  - verify file. verifier is client_cli. verify target is broker.
