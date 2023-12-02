# Strand
endpoint has boost asio's strand internally.CompletionToken is called in the endpoint's strand. Strand is useful to avoid locks and mutexes.
Here is the list of APIs 

endpoint member function | effects
---|---
acquire_unique_packet_id|Acquire the new unique packet_id
register_packet_id|Register the packet_id
release_packet_id|Release the packet_id
get_qos2_publish_handled_pids|Get already PUBLISH recv CompletionToken is invoked packet_ids
restore_qos2_publish_handled_pids|Restore already PUBLISH recv CompletionToken is invoked packet_ids
restore_packets|Restore pacets as stored packets
get_stored_packets|Get stored packets
get_protocol_version|Get MQTT protocol version

acquire_unique_packet_id, register_packet_id, and release_packet_id have async version (the same name overload) to call out of strand.
