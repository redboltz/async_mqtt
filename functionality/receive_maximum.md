# Receive Maximum
Receive Maximum is a way to flow control PUBLISH(QoS1, QoS2) packets. They require to store for resending. So the node should prepare the memory packet size * Receive Maximum.
The maximum value of the packet size can be defined using [Maximum Packet Size](functionality/maximum_packet_size.md).

## Notifying Receive Maximum
There are two independent Receive Maximum. 

```mermaid
sequenceDiagram
Note left of client1: prepare 10 * MaximumPacketSize memory
client1->>broker: CONNECT ReceiveMaximum=10
Note right of broker: broker can send at most 10 PUBLISH (QoS1 or QoS2) packets to client1
Note right of broker: prepare 20 * MaximumPacketSize memory for client1
broker->>client1: CONNACK ReceiveMaximum=20
Note left of client1: client1 can send at most 10 PUBLISH (QoS1 or QoS2) packets to the broker
```

### broker to client Receive Maximum
The client can set `Receive Maximum` property that value is greater than 0 to the CONNECT packet. This means the client can receive the PUBLISH (QoS1 or QoS2) packet concurrently. If the sequence of publish is finished, 

Now writing...
