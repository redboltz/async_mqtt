# Strand
endpoint has boost asio's strand internally.CompletionToken is called in the endpoint's strand. Strand is useful to avoid locks and mutexes.
You can get the strand from the endpoint using `strand()` function and use it as the executor for Boost.Asio's async functions like `steady_timer`.

It could be annoying calling async APIs from obiously in the strand context. e.g. in the recv() CompletionToken. async_mqtt provides some of sync APIs.
Here is the list of **sync** APIs that can be called only in the strand.

endpoint member function | effects
---|---
[acquire_unique_packet_id](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html#a7d58c77fd13b77afdbc94a6e3c865b36)|Acquire the new unique packet_id
[register_packet_id](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html#a91683e5aa2ed234e5c14f79361ff2deb)|Register the packet_id
[release_packet_id](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html#a90777e5fde27013cc8d308d501a6ead8)|Release the packet_id
[get_qos2_publish_handled_pids](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html#a1aaf7274ef58eadf15428071cae9e894)|Get already PUBLISH recv CompletionToken is invoked packet_ids
[restore_qos2_publish_handled_pids](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html#a4696744f07068176e48e12aeb4998fb0)|Restore already PUBLISH recv CompletionToken is invoked packet_ids
[restore_packets](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html#a6dfe47bd9ab1590e66f110e3dbe1087e)|Restore pacets as stored packets
[get_stored_packets](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html#a5ed8d45ffcfb114533d8de5ddddb4f92)|Get stored packets
[get_protocol_version](https://redboltz.github.io/async_mqtt/doc/latest/html/classasync__mqtt_1_1basic__endpoint.html#a9cbabd5f427b1cb18d61ac49c7bbf83b)|Get MQTT protocol version

acquire_unique_packet_id, register_packet_id, release_packet_id, restore_packets, and get_stored_packets have async version (the same name overload) to call out of strand.
