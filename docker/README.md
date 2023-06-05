# Docker container

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

## Broker port mapping

Protocol | Host | Container
---|---|---
mqtt|1883|1883
mqtts|8883|8883
ws|10080|10080
wss|10443|10443

The ports on the host need to be usable.
You can change the ports of the host by editing `broker.sh`.

## Access from bench (container) to the host

`host.docker.internal` is the name of host.

When you run `./broker.sh` in your PC (host), you can run `./bench.sh --host host.docker.internal`, then bench container can access to the broker container.

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
