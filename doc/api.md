# Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`namespace `[`async_mqtt`](#namespaceasync__mqtt) | 
`namespace `[`async_mqtt::detail`](#namespaceasync__mqtt_1_1detail) | 
`namespace `[`boost::asio`](#namespaceboost_1_1asio) | 
`namespace `[`std`](#namespacestd) | 
`struct `[`async_mqtt::basic_endpoint::acquire_unique_packet_id_impl`](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl) | 
`struct `[`async_mqtt::basic_endpoint::close_impl`](#structasync__mqtt_1_1basic__endpoint_1_1close__impl) | 
`struct `[`async_mqtt::stream::close_impl`](#structasync__mqtt_1_1stream_1_1close__impl) | 
`struct `[`async_mqtt::store::elem_t`](#structasync__mqtt_1_1store_1_1elem__t) | 
`struct `[`async_mqtt::topic_alias_recv::entry`](#structasync__mqtt_1_1topic__alias__recv_1_1entry) | 
`struct `[`async_mqtt::topic_alias_send::entry`](#structasync__mqtt_1_1topic__alias__send_1_1entry) | 
`struct `[`async_mqtt::basic_endpoint::get_stored_packets_impl`](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl) | 
`struct `[`async_mqtt::stream::read_packet_impl`](#structasync__mqtt_1_1stream_1_1read__packet__impl) | 
`struct `[`async_mqtt::basic_endpoint::recv_impl`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl) | 
`struct `[`async_mqtt::basic_endpoint::register_packet_id_impl`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl) | 
`struct `[`async_mqtt::basic_endpoint::release_packet_id_impl`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl) | 
`struct `[`async_mqtt::basic_endpoint::restore_packets_impl`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl) | 
`struct `[`async_mqtt::basic_endpoint::send_impl`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl) | 
`struct `[`async_mqtt::topic_alias_send::tag_alias`](#structasync__mqtt_1_1topic__alias__send_1_1tag__alias) | 
`struct `[`async_mqtt::store::tag_res_id`](#structasync__mqtt_1_1store_1_1tag__res__id) | 
`struct `[`async_mqtt::store::tag_seq`](#structasync__mqtt_1_1store_1_1tag__seq) | 
`struct `[`async_mqtt::store::tag_tim`](#structasync__mqtt_1_1store_1_1tag__tim) | 
`struct `[`async_mqtt::topic_alias_send::tag_topic_name`](#structasync__mqtt_1_1topic__alias__send_1_1tag__topic__name) | 
`struct `[`async_mqtt::topic_alias_send::tag_tp`](#structasync__mqtt_1_1topic__alias__send_1_1tag__tp) | 
`struct `[`async_mqtt::stream::write_packet_impl`](#structasync__mqtt_1_1stream_1_1write__packet__impl) | 

# namespace `async_mqtt` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`enum `[`role`](#endpoint_8hpp_1a83492944f4f6b69eb11ce917d1f27c4a)            | MQTT endpoint connection role.
`enum `[`filter`](#endpoint_8hpp_1aed6ebca93cac28c67f2b4fb8641e775a)            | receive packet filter
`enum `[`connection_status`](#endpoint_8hpp_1a61a2142899f6d0de9f132d68d2ead233)            | 
`enum `[`severity_level`](#log_8hpp_1a3abd0c0ebf71c7d01def2a00423d1abf)            | 
`enum `[`protocol_version`](#protocol__version_8hpp_1a133d6d106d4e527d07fea2d12544c0c0)            | 
`public inline std::size_t `[`hash_value`](#buffer_8hpp_1aa32f966b1bfe6e2f16960e480a3eb908)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & v) noexcept`            | 
`public inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`operator""_mb`](#buffer_8hpp_1a5a85b821592e417958bf94f866051756)`(char const * str,std::size_t length)`            | user defined literals for buffer If user use this out of mqtt scope, then user need to declare `using namespace literals`. When user write "ABC"_mb, then this function is called. The created buffer doesn't hold any lifetimes because the string literals has static strage duration, so buffer doesn't need to hold the lifetime.
`public template<>`  <br/>`inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`allocate_buffer`](#buffer_8hpp_1ad8154cb2ff0b90f356ee7895a877b454)`(Iterator b,Iterator e)`            | create buffer from the pair of iterators It copies string that from b to e into shared_ptr_array. Then create buffer and return it. The buffer holds the lifetime of shared_ptr_array.
`public inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`allocate_buffer`](#buffer_8hpp_1ab64f2141bd17765a7fed519c33cf6b07)`(string_view sv)`            | create buffer from the string_view It copies string that from string_view into shared_ptr_array. Then create buffer and return it. The buffer holds the lifetime of shared_ptr_array.
`public inline `[`buffer`](#classasync__mqtt_1_1buffer)` const * `[`buffer_sequence_begin`](#buffer_8hpp_1aa129a379bcde41d9294d152e4d9865f4)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf)`            | 
`public inline `[`buffer`](#classasync__mqtt_1_1buffer)` const * `[`buffer_sequence_end`](#buffer_8hpp_1a11bc5114504a34d2ecd16a8a285a8b6a)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf)`            | 
`public template<>`  <br/>`inline Col::const_iterator `[`buffer_sequence_begin`](#buffer_8hpp_1ad23e6f5c8311a8302dd29036f0bcef45)`(Col const & col)`            | 
`public template<>`  <br/>`inline Col::const_iterator `[`buffer_sequence_end`](#buffer_8hpp_1a82e03acfc145a681b96b3bd1427809c7)`(Col const & col)`            | 
`public template<>`  <br/>`basic_packet_variant< PacketIdBytes > `[`buffer_to_basic_packet_variant`](#buffer__to__packet__variant_8hpp_1a8553540e2773cf8e0b0adeffab3693b4)`(`[`buffer`](#classasync__mqtt_1_1buffer)` buf,protocol_version ver)`            | 
`public inline packet_variant `[`buffer_to_packet_variant`](#buffer__to__packet__variant_8hpp_1af8aab4b419d9464247dc247352015ac1)`(`[`buffer`](#classasync__mqtt_1_1buffer)` buf,protocol_version ver)`            | 
`public constexpr bool `[`can_send_as_client`](#endpoint_8hpp_1a0e868e1771d650d1a3037dacc130e450)`(role r)`            | 
`public constexpr bool `[`can_send_as_server`](#endpoint_8hpp_1acef8e2f61b325bb6519466cf50025a47)`(role r)`            | 
`public inline optional< topic_alias_t > `[`get_topic_alias`](#endpoint_8hpp_1a869026bae9fb6d380aa451acaeaa1cd4)`(properties const & props)`            | 
`public template<>`  <br/>`inline system_error `[`make_error`](#exception_8hpp_1ad188624566d32be075c01827788ae9b2)`(errc::errc_t ec,WhatArg && wa)`            | 
`public inline bool `[`operator==`](#exception_8hpp_1ae86f3d9438819f5c9ce22375b2ae1b17)`(system_error const & lhs,system_error const & rhs)`            | 
`public inline bool `[`operator<`](#exception_8hpp_1a55bf83f4888d23b816b791e7ea16e2e1)`(system_error const & lhs,system_error const & rhs)`            | 
`public inline std::ostream & `[`operator<<`](#exception_8hpp_1a09eee1b9711cc858ff16e0d321783d3e)`(std::ostream & o,system_error const & v)`            | 
`public template<>`  <br/>`constexpr bool `[`is_strand`](#is__strand_8hpp_1a61078fb49a37d6342ffbc103ee132263)`()`            | 
`public inline std::ostream & `[`operator<<`](#log_8hpp_1a4784f7724ed0ebbdba4834f0666b0553)`(std::ostream & o,severity_level sev)`            | 
`public constexpr char const * `[`protocol_version_to_str`](#protocol__version_8hpp_1af85e0e1bf4a3ffa4862091ea286b0fb7)`(protocol_version v)`            | 
`public inline std::ostream & `[`operator<<`](#protocol__version_8hpp_1a6ad547f5d98dfee9c3436bd68f91de1b)`(std::ostream & os,protocol_version val)`            | 
`public template<>`  <br/>`void `[`setup_log`](#setup__log_8hpp_1aef1075f746c811a415f3c5bbf510b1c5)`(Params && ...)`            | 
`public inline static_vector< char, 4 > `[`val_to_variable_bytes`](#variable__bytes_8hpp_1a8b9d757055d4f21f4ad514eaf90e0273)`(std::uint32_t val)`            | 
`public template<>`  <br/>`constexpr std::enable_if_t< is_input_iterator< It >::value &&is_input_iterator< End >::value, optional< std::uint32_t > > `[`variable_bytes_to_val`](#variable__bytes_8hpp_1a357943cb86de3f9d17aa0b06bf3ea026)`(It & it,End e)`            | 
`public template<>`  <br/>`as::async_result< std::decay_t< CompletionToken >, void(boost::system::error_codeconst &, std::size_t)>::return_type `[`async_read`](#ws__fixed__size__async__read_8hpp_1a038934ea5fa4ada6411f82dea94a21d2)`(bs::websocket::stream< NextLayer > & stream,MutableBufferSequence const & mb,CompletionToken && token)`            | 
`public template<>`  <br/>`as::async_result< std::decay_t< CompletionToken >, void(boost::system::error_codeconst &, std::size_t)>::return_type `[`async_write`](#ws__fixed__size__async__read_8hpp_1a4b3a7bc0a6f5502016cfc050cc2b1617)`(bs::websocket::stream< NextLayer > & stream,ConstBufferSequence const & cbs,CompletionToken && token)`            | 
`class `[`async_mqtt::basic_endpoint`](#classasync__mqtt_1_1basic__endpoint) | MQTT endpoint corresponding to the connection.
`class `[`async_mqtt::buffer`](#classasync__mqtt_1_1buffer) | buffer that has string_view interface This class provides string_view interface. This class hold string_view target's lifetime optionally.
`class `[`async_mqtt::packet_id_manager`](#classasync__mqtt_1_1packet__id__manager) | 
`class `[`async_mqtt::store`](#classasync__mqtt_1_1store) | 
`class `[`async_mqtt::stream`](#classasync__mqtt_1_1stream) | 
`class `[`async_mqtt::topic_alias_recv`](#classasync__mqtt_1_1topic__alias__recv) | 
`class `[`async_mqtt::topic_alias_send`](#classasync__mqtt_1_1topic__alias__send) | 
`struct `[`async_mqtt::channel`](#structasync__mqtt_1_1channel) | 
`struct `[`async_mqtt::is_buffer_sequence`](#structasync__mqtt_1_1is__buffer__sequence) | 
`struct `[`async_mqtt::is_buffer_sequence< buffer >`](#structasync__mqtt_1_1is__buffer__sequence_3_01buffer_01_4) | 
`struct `[`async_mqtt::is_strand_template`](#structasync__mqtt_1_1is__strand__template) | 
`struct `[`async_mqtt::is_strand_template< as::strand< T > >`](#structasync__mqtt_1_1is__strand__template_3_01as_1_1strand_3_01T_01_4_01_4) | 
`struct `[`async_mqtt::is_tls`](#structasync__mqtt_1_1is__tls) | 
`struct `[`async_mqtt::is_ws`](#structasync__mqtt_1_1is__ws) | 
`struct `[`async_mqtt::system_error`](#structasync__mqtt_1_1system__error) | 

## Members

#### `enum `[`role`](#endpoint_8hpp_1a83492944f4f6b69eb11ce917d1f27c4a) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
client            | 
server            | as client. Can't send CONNACK, SUBACK, UNSUBACK, PINGRESP. Can send Other packets.
any            | as server. Can't send CONNECT, SUBSCRIBE, UNSUBSCRIBE, PINGREQ, DISCONNECT(only on v3.1.1). Can send Other packets.

MQTT endpoint connection role.

#### `enum `[`filter`](#endpoint_8hpp_1aed6ebca93cac28c67f2b4fb8641e775a) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
match            | 
except            | matched control_packet_type is target

receive packet filter

#### `enum `[`connection_status`](#endpoint_8hpp_1a61a2142899f6d0de9f132d68d2ead233) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
connecting            | 
connected            | 
disconnecting            | 
disconnected            | 

#### `enum `[`severity_level`](#log_8hpp_1a3abd0c0ebf71c7d01def2a00423d1abf) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
trace            | 
debug            | 
info            | 
warning            | 
error            | 
fatal            | 

#### `enum `[`protocol_version`](#protocol__version_8hpp_1a133d6d106d4e527d07fea2d12544c0c0) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
undetermined            | 
v3_1_1            | 
v5            | 

#### `public inline std::size_t `[`hash_value`](#buffer_8hpp_1aa32f966b1bfe6e2f16960e480a3eb908)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & v) noexcept` 

#### `public inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`operator""_mb`](#buffer_8hpp_1a5a85b821592e417958bf94f866051756)`(char const * str,std::size_t length)` 

user defined literals for buffer If user use this out of mqtt scope, then user need to declare `using namespace literals`. When user write "ABC"_mb, then this function is called. The created buffer doesn't hold any lifetimes because the string literals has static strage duration, so buffer doesn't need to hold the lifetime.

#### Parameters
* `str` the address of the string literal 

* `length` the length of the string literal 

#### Returns
buffer

#### `public template<>`  <br/>`inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`allocate_buffer`](#buffer_8hpp_1ad8154cb2ff0b90f356ee7895a877b454)`(Iterator b,Iterator e)` 

create buffer from the pair of iterators It copies string that from b to e into shared_ptr_array. Then create buffer and return it. The buffer holds the lifetime of shared_ptr_array.

#### Parameters
* `b` begin position iterator 

* `e` end position iterator 

#### Returns
buffer

#### `public inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`allocate_buffer`](#buffer_8hpp_1ab64f2141bd17765a7fed519c33cf6b07)`(string_view sv)` 

create buffer from the string_view It copies string that from string_view into shared_ptr_array. Then create buffer and return it. The buffer holds the lifetime of shared_ptr_array.

#### Parameters
* `sv` the source string_view 

#### Returns
buffer

#### `public inline `[`buffer`](#classasync__mqtt_1_1buffer)` const * `[`buffer_sequence_begin`](#buffer_8hpp_1aa129a379bcde41d9294d152e4d9865f4)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf)` 

#### `public inline `[`buffer`](#classasync__mqtt_1_1buffer)` const * `[`buffer_sequence_end`](#buffer_8hpp_1a11bc5114504a34d2ecd16a8a285a8b6a)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf)` 

#### `public template<>`  <br/>`inline Col::const_iterator `[`buffer_sequence_begin`](#buffer_8hpp_1ad23e6f5c8311a8302dd29036f0bcef45)`(Col const & col)` 

#### `public template<>`  <br/>`inline Col::const_iterator `[`buffer_sequence_end`](#buffer_8hpp_1a82e03acfc145a681b96b3bd1427809c7)`(Col const & col)` 

#### `public template<>`  <br/>`basic_packet_variant< PacketIdBytes > `[`buffer_to_basic_packet_variant`](#buffer__to__packet__variant_8hpp_1a8553540e2773cf8e0b0adeffab3693b4)`(`[`buffer`](#classasync__mqtt_1_1buffer)` buf,protocol_version ver)` 

#### `public inline packet_variant `[`buffer_to_packet_variant`](#buffer__to__packet__variant_8hpp_1af8aab4b419d9464247dc247352015ac1)`(`[`buffer`](#classasync__mqtt_1_1buffer)` buf,protocol_version ver)` 

#### `public constexpr bool `[`can_send_as_client`](#endpoint_8hpp_1a0e868e1771d650d1a3037dacc130e450)`(role r)` 

#### `public constexpr bool `[`can_send_as_server`](#endpoint_8hpp_1acef8e2f61b325bb6519466cf50025a47)`(role r)` 

#### `public inline optional< topic_alias_t > `[`get_topic_alias`](#endpoint_8hpp_1a869026bae9fb6d380aa451acaeaa1cd4)`(properties const & props)` 

#### `public template<>`  <br/>`inline system_error `[`make_error`](#exception_8hpp_1ad188624566d32be075c01827788ae9b2)`(errc::errc_t ec,WhatArg && wa)` 

#### `public inline bool `[`operator==`](#exception_8hpp_1ae86f3d9438819f5c9ce22375b2ae1b17)`(system_error const & lhs,system_error const & rhs)` 

#### `public inline bool `[`operator<`](#exception_8hpp_1a55bf83f4888d23b816b791e7ea16e2e1)`(system_error const & lhs,system_error const & rhs)` 

#### `public inline std::ostream & `[`operator<<`](#exception_8hpp_1a09eee1b9711cc858ff16e0d321783d3e)`(std::ostream & o,system_error const & v)` 

#### `public template<>`  <br/>`constexpr bool `[`is_strand`](#is__strand_8hpp_1a61078fb49a37d6342ffbc103ee132263)`()` 

#### `public inline std::ostream & `[`operator<<`](#log_8hpp_1a4784f7724ed0ebbdba4834f0666b0553)`(std::ostream & o,severity_level sev)` 

#### `public constexpr char const * `[`protocol_version_to_str`](#protocol__version_8hpp_1af85e0e1bf4a3ffa4862091ea286b0fb7)`(protocol_version v)` 

#### `public inline std::ostream & `[`operator<<`](#protocol__version_8hpp_1a6ad547f5d98dfee9c3436bd68f91de1b)`(std::ostream & os,protocol_version val)` 

#### `public template<>`  <br/>`void `[`setup_log`](#setup__log_8hpp_1aef1075f746c811a415f3c5bbf510b1c5)`(Params && ...)` 

#### `public inline static_vector< char, 4 > `[`val_to_variable_bytes`](#variable__bytes_8hpp_1a8b9d757055d4f21f4ad514eaf90e0273)`(std::uint32_t val)` 

#### `public template<>`  <br/>`constexpr std::enable_if_t< is_input_iterator< It >::value &&is_input_iterator< End >::value, optional< std::uint32_t > > `[`variable_bytes_to_val`](#variable__bytes_8hpp_1a357943cb86de3f9d17aa0b06bf3ea026)`(It & it,End e)` 

#### `public template<>`  <br/>`as::async_result< std::decay_t< CompletionToken >, void(boost::system::error_codeconst &, std::size_t)>::return_type `[`async_read`](#ws__fixed__size__async__read_8hpp_1a038934ea5fa4ada6411f82dea94a21d2)`(bs::websocket::stream< NextLayer > & stream,MutableBufferSequence const & mb,CompletionToken && token)` 

#### `public template<>`  <br/>`as::async_result< std::decay_t< CompletionToken >, void(boost::system::error_codeconst &, std::size_t)>::return_type `[`async_write`](#ws__fixed__size__async__read_8hpp_1a4b3a7bc0a6f5502016cfc050cc2b1617)`(bs::websocket::stream< NextLayer > & stream,ConstBufferSequence const & cbs,CompletionToken && token)` 

# class `async_mqtt::basic_endpoint` 

MQTT endpoint corresponding to the connection.

#### Parameters
* `Role` role for packet sendable checking 

* `PacketIdBytes` MQTT spec is 2. You can use `endpoint` for that. 

* `NextLayer` Just next layer for [basic_endpoint](#classasync__mqtt_1_1basic__endpoint). mqtt, mqtts, ws, and wss are predefined.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public template<>`  <br/>`inline  `[`basic_endpoint`](#classasync__mqtt_1_1basic__endpoint_1abc8fe2d40a34da40817c6624a086dc25)`(protocol_version ver,Args &&... args)` | constructor
`public inline strand_type const & `[`strand`](#classasync__mqtt_1_1basic__endpoint_1a3f4ac8feefc66e49003a30c710695e73)`() const` | strand getter
`public inline strand_type & `[`strand`](#classasync__mqtt_1_1basic__endpoint_1ac6b578d516addd0e490160d4189dd7b9)`()` | strand getter
`public inline next_layer_type const & `[`next_layer`](#classasync__mqtt_1_1basic__endpoint_1ada60e5d8c35b1d668a4edf5d0c332f0b)`() const` | next_layer getter
`public inline next_layer_type & `[`next_layer`](#classasync__mqtt_1_1basic__endpoint_1acc21bc830ae4ffd4a7803d3bdd166a21)`()` | next_layer getter
`public inline auto const & `[`lowest_layer`](#classasync__mqtt_1_1basic__endpoint_1ac421785ffce061dc7792a22b586806e4)`() const` | lowest_layer getter
`public inline auto & `[`lowest_layer`](#classasync__mqtt_1_1basic__endpoint_1a8f483895a76b9d399ab15c32fc201f81)`()` | lowest_layer getter
`public inline void `[`set_auto_pub_response`](#classasync__mqtt_1_1basic__endpoint_1a5e8920d50890684fc33eab70c709a90f)`(bool val)` | auto publish response setter. Should be called before [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0)/recv() call.
`public inline void `[`set_auto_ping_response`](#classasync__mqtt_1_1basic__endpoint_1a5e77ec0b180801e25279d35d225a7771)`(bool val)` | auto pingreq response setter. Should be called before [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0)/recv() call.
`public inline void `[`set_auto_map_topic_alias_send`](#classasync__mqtt_1_1basic__endpoint_1a596d2617fa46cd0f37b40afbf4f912df)`(bool val)` | auto map (allocate) topic alias on send PUBLISH packet. If all topic aliases are used, then overwrite by LRU algorithm. Should be called before [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0) call.
`public inline void `[`set_auto_replace_topic_alias_send`](#classasync__mqtt_1_1basic__endpoint_1a70f40da2602fb6b22049aafa815782e0)`(bool val)` | auto replace topic with corresponding topic alias on send PUBLISH packet. Registering topic alias need to do manually. Should be called before [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0) call.
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(optional< `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090) >)`>::return_type `[`acquire_unique_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a24c5e674d28854efa852129ddcb83625)`(CompletionToken && token)` | acuire unique packet_id.
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(bool)>::return_type `[`register_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a6d9145d84663d38743fb47cf2d9f124b)`(`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` packet_id,CompletionToken && token)` | register packet_id.
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void()>::return_type `[`release_packet_id`](#classasync__mqtt_1_1basic__endpoint_1ad4296e338bc6847e59ff64fd5aa16a25)`(`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` packet_id,CompletionToken && token)` | release packet_id.
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(system_error)>::return_type `[`send`](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0)`(Packet packet,CompletionToken && token)` | send packet users can call [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0) before the previous [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0)'s CompletionToken is invoked
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(`[`packet_variant_type`](#classasync__mqtt_1_1basic__endpoint_1a5098e22b58adbee93e452ede78ab1adc))`>::return_type `[`recv`](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25)`(CompletionToken && token)` | receive packet users CANNOT call [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) before the previous [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25)'s CompletionToken is invoked
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(`[`packet_variant_type`](#classasync__mqtt_1_1basic__endpoint_1a5098e22b58adbee93e452ede78ab1adc))`>::return_type `[`recv`](#classasync__mqtt_1_1basic__endpoint_1a8b0e5b8a19c9f9fde23077504a205480)`(std::set< control_packet_type > types,CompletionToken && token)` | receive packet users CANNOT call [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) before the previous [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25)'s CompletionToken is invoked if packet is not filterd, then next [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) starts automatically. if receive error happenes, then token would be invoked.
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(`[`packet_variant_type`](#classasync__mqtt_1_1basic__endpoint_1a5098e22b58adbee93e452ede78ab1adc))`>::return_type `[`recv`](#classasync__mqtt_1_1basic__endpoint_1afa1f136a75fbe2ffb2dac8e1b5a52094)`(filter fil,std::set< control_packet_type > types,CompletionToken && token)` | receive packet users CANNOT call [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) before the previous [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25)'s CompletionToken is invoked if packet is not filterd, then next [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) starts automatically. if receive error happenes, then token would be invoked. @params fil if `match` then matched types are targets. if `except` then not matched types are targets.
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void()>::return_type `[`close`](#classasync__mqtt_1_1basic__endpoint_1a58ea2597adf88429dd8abaadc0d9c024)`(CompletionToken && token)` | close the underlying connection
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void()>::return_type `[`restore_packets`](#classasync__mqtt_1_1basic__endpoint_1abb51449a3077b44cc3d56a1ecf9edcb0)`(std::vector< basic_store_packet_variant< PacketIdBytes > > pvs,CompletionToken && token)` | restore packets the restored packets would automatically send when CONNACK packet is received
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(std::vector< basic_store_packet_variant< PacketIdBytes > >)>::return_type `[`get_stored_packets`](#classasync__mqtt_1_1basic__endpoint_1a0f8b85838eb7a05202b3a0f7e7521961)`(CompletionToken && token) const` | get stored packets sotred packets mean inflight packets.
`public inline optional< `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` > `[`acquire_unique_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a7d58c77fd13b77afdbc94a6e3c865b36)`()` | acuire unique packet_id.
`public inline bool `[`register_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a91683e5aa2ed234e5c14f79361ff2deb)`(`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` pid)` | register packet_id.
`public inline void `[`release_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a90777e5fde27013cc8d308d501a6ead8)`(`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` pid)` | release packet_id.
`public inline std::set< `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` > `[`get_qos2_publish_handled_pids`](#classasync__mqtt_1_1basic__endpoint_1a1aaf7274ef58eadf15428071cae9e894)`() const` | Get processed but not released QoS2 packet ids This function should be called after disconnection.
`public inline void `[`restore_qos2_publish_handled_pids`](#classasync__mqtt_1_1basic__endpoint_1a4696744f07068176e48e12aeb4998fb0)`(std::set< `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` > pids)` | Restore processed but not released QoS2 packet ids This function should be called before receive the first publish.
`public inline void `[`restore_packets`](#classasync__mqtt_1_1basic__endpoint_1a6dfe47bd9ab1590e66f110e3dbe1087e)`(std::vector< basic_store_packet_variant< PacketIdBytes > > pvs)` | restore packets the restored packets would automatically send when CONNACK packet is received
`public inline std::vector< basic_store_packet_variant< PacketIdBytes > > `[`get_stored_packets`](#classasync__mqtt_1_1basic__endpoint_1a5ed8d45ffcfb114533d8de5ddddb4f92)`() const` | get stored packets sotred packets mean inflight packets.
`public inline protocol_version `[`get_protocol_version`](#classasync__mqtt_1_1basic__endpoint_1a9cbabd5f427b1cb18d61ac49c7bbf83b)`() const` | get MQTT protocol version
`public inline void `[`cancel_all_timers_for_test`](#classasync__mqtt_1_1basic__endpoint_1afd81415fa06d1b4fcafc70273628df9e)`()` | 
`public inline void `[`set_pingreq_send_interval_ms_for_test`](#classasync__mqtt_1_1basic__endpoint_1a94c953a39c2bacf5467a2bb21d2daac6)`(std::size_t ms)` | 
`typedef `[`this_type`](#classasync__mqtt_1_1basic__endpoint_1aca223957602c95071273e7a07c3d2d0c) | 
`typedef `[`stream_type`](#classasync__mqtt_1_1basic__endpoint_1acb36b8af5cc1833dea134c8efd7ce686) | 
`typedef `[`next_layer_type`](#classasync__mqtt_1_1basic__endpoint_1a045005fba8b583a870cde28a58da70bb) | 
`typedef `[`strand_type`](#classasync__mqtt_1_1basic__endpoint_1a842403c46cb455e467380f28d3a14289) | 
`typedef `[`packet_variant_type`](#classasync__mqtt_1_1basic__endpoint_1a5098e22b58adbee93e452ede78ab1adc) | Type of packet_variant.
`typedef `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090) | Type of MQTT Packet Identifier.

## Members

#### `public template<>`  <br/>`inline  `[`basic_endpoint`](#classasync__mqtt_1_1basic__endpoint_1abc8fe2d40a34da40817c6624a086dc25)`(protocol_version ver,Args &&... args)` 

constructor

#### Parameters
* `Args` Types for the next layer 

#### Parameters
* `ver` MQTT protocol version (v5 or v3_1_1) 

* `args` args for the next layer

#### `public inline strand_type const & `[`strand`](#classasync__mqtt_1_1basic__endpoint_1a3f4ac8feefc66e49003a30c710695e73)`() const` 

strand getter

#### Returns
const reference of the strand

#### `public inline strand_type & `[`strand`](#classasync__mqtt_1_1basic__endpoint_1ac6b578d516addd0e490160d4189dd7b9)`()` 

strand getter

#### Returns
eference of the strand

#### `public inline next_layer_type const & `[`next_layer`](#classasync__mqtt_1_1basic__endpoint_1ada60e5d8c35b1d668a4edf5d0c332f0b)`() const` 

next_layer getter

#### Returns
const reference of the next_layer

#### `public inline next_layer_type & `[`next_layer`](#classasync__mqtt_1_1basic__endpoint_1acc21bc830ae4ffd4a7803d3bdd166a21)`()` 

next_layer getter

#### Returns
reference of the next_layer

#### `public inline auto const & `[`lowest_layer`](#classasync__mqtt_1_1basic__endpoint_1ac421785ffce061dc7792a22b586806e4)`() const` 

lowest_layer getter

#### Returns
const reference of the lowest_layer

#### `public inline auto & `[`lowest_layer`](#classasync__mqtt_1_1basic__endpoint_1a8f483895a76b9d399ab15c32fc201f81)`()` 

lowest_layer getter

#### Returns
reference of the lowest_layer

#### `public inline void `[`set_auto_pub_response`](#classasync__mqtt_1_1basic__endpoint_1a5e8920d50890684fc33eab70c709a90f)`(bool val)` 

auto publish response setter. Should be called before [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0)/recv() call.

By default not automatically sending. 

#### Parameters
* `val` if true, puback, pubrec, pubrel, and pubcomp are automatically sent

#### `public inline void `[`set_auto_ping_response`](#classasync__mqtt_1_1basic__endpoint_1a5e77ec0b180801e25279d35d225a7771)`(bool val)` 

auto pingreq response setter. Should be called before [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0)/recv() call.

By default not automatically sending. 

#### Parameters
* `val` if true, puback, pubrec, pubrel, and pubcomp are automatically sent

#### `public inline void `[`set_auto_map_topic_alias_send`](#classasync__mqtt_1_1basic__endpoint_1a596d2617fa46cd0f37b40afbf4f912df)`(bool val)` 

auto map (allocate) topic alias on send PUBLISH packet. If all topic aliases are used, then overwrite by LRU algorithm. Should be called before [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0) call.

By default not automatically mapping. 

#### Parameters
* `val` if true, enable auto mapping, otherwise disable.

#### `public inline void `[`set_auto_replace_topic_alias_send`](#classasync__mqtt_1_1basic__endpoint_1a70f40da2602fb6b22049aafa815782e0)`(bool val)` 

auto replace topic with corresponding topic alias on send PUBLISH packet. Registering topic alias need to do manually. Should be called before [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0) call.

By default not automatically replacing. 

#### Parameters
* `val` if true, enable auto replacing, otherwise disable.

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(optional< `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090) >)`>::return_type `[`acquire_unique_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a24c5e674d28854efa852129ddcb83625)`(CompletionToken && token)` 

acuire unique packet_id.

#### Parameters
* `token` the param is optional<packet_id_t> 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(bool)>::return_type `[`register_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a6d9145d84663d38743fb47cf2d9f124b)`(`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` packet_id,CompletionToken && token)` 

register packet_id.

#### Parameters
* `packet_id` packet_id to register 

* `token` the param is bool. If true, success, otherwise the packet_id has already been used. 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void()>::return_type `[`release_packet_id`](#classasync__mqtt_1_1basic__endpoint_1ad4296e338bc6847e59ff64fd5aa16a25)`(`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` packet_id,CompletionToken && token)` 

release packet_id.

#### Parameters
* `packet_id` packet_id to release 

* `token` the param is void 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(system_error)>::return_type `[`send`](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0)`(Packet packet,CompletionToken && token)` 

send packet users can call [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0) before the previous [send()](#classasync__mqtt_1_1basic__endpoint_1aeadb5049b6fba3dd8075ba4f8daadcd0)'s CompletionToken is invoked

#### Parameters
* `packet` packet to send 

* `token` the param is system_error 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(`[`packet_variant_type`](#classasync__mqtt_1_1basic__endpoint_1a5098e22b58adbee93e452ede78ab1adc))`>::return_type `[`recv`](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25)`(CompletionToken && token)` 

receive packet users CANNOT call [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) before the previous [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25)'s CompletionToken is invoked

#### Parameters
* `token` the param is packet_variant_type 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(`[`packet_variant_type`](#classasync__mqtt_1_1basic__endpoint_1a5098e22b58adbee93e452ede78ab1adc))`>::return_type `[`recv`](#classasync__mqtt_1_1basic__endpoint_1a8b0e5b8a19c9f9fde23077504a205480)`(std::set< control_packet_type > types,CompletionToken && token)` 

receive packet users CANNOT call [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) before the previous [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25)'s CompletionToken is invoked if packet is not filterd, then next [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) starts automatically. if receive error happenes, then token would be invoked.

#### Parameters
* `types` target control_packet_types 

* `token` the param is packet_variant_type 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(`[`packet_variant_type`](#classasync__mqtt_1_1basic__endpoint_1a5098e22b58adbee93e452ede78ab1adc))`>::return_type `[`recv`](#classasync__mqtt_1_1basic__endpoint_1afa1f136a75fbe2ffb2dac8e1b5a52094)`(filter fil,std::set< control_packet_type > types,CompletionToken && token)` 

receive packet users CANNOT call [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) before the previous [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25)'s CompletionToken is invoked if packet is not filterd, then next [recv()](#classasync__mqtt_1_1basic__endpoint_1ae43f3f6c57a9e9267409132de7e44f25) starts automatically. if receive error happenes, then token would be invoked. @params fil if `match` then matched types are targets. if `except` then not matched types are targets.

#### Parameters
* `types` target control_packet_types 

* `token` the param is packet_variant_type 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void()>::return_type `[`close`](#classasync__mqtt_1_1basic__endpoint_1a58ea2597adf88429dd8abaadc0d9c024)`(CompletionToken && token)` 

close the underlying connection

#### Parameters
* `token` the param is void 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void()>::return_type `[`restore_packets`](#classasync__mqtt_1_1basic__endpoint_1abb51449a3077b44cc3d56a1ecf9edcb0)`(std::vector< basic_store_packet_variant< PacketIdBytes > > pvs,CompletionToken && token)` 

restore packets the restored packets would automatically send when CONNACK packet is received

#### Parameters
* `pvs` packets to restore 

* `token` the param is void 

#### Returns
deduced by token

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(std::vector< basic_store_packet_variant< PacketIdBytes > >)>::return_type `[`get_stored_packets`](#classasync__mqtt_1_1basic__endpoint_1a0f8b85838eb7a05202b3a0f7e7521961)`(CompletionToken && token) const` 

get stored packets sotred packets mean inflight packets.

* PUBLISH packet (QoS1) not received PUBACK packet

* PUBLISH packet (QoS1) not received PUBREC packet

* PUBREL packet not received PUBCOMP packet 
#### Parameters
* `token` the param is std::vector<basic_store_packet_variant<PacketIdBytes>> 

#### Returns
deduced by token

#### `public inline optional< `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` > `[`acquire_unique_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a7d58c77fd13b77afdbc94a6e3c865b36)`()` 

acuire unique packet_id.

#### Returns
optional<packet_id_t> if acquired return acquired packet id, otherwise nullopt 

This function is SYNC function that must only be called in the strand.

#### `public inline bool `[`register_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a91683e5aa2ed234e5c14f79361ff2deb)`(`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` pid)` 

register packet_id.

#### Parameters
* `packet_id` packet_id to register 

#### Returns
If true, success, otherwise the packet_id has already been used. 

This function is SYNC function that must only be called in the strand.

#### `public inline void `[`release_packet_id`](#classasync__mqtt_1_1basic__endpoint_1a90777e5fde27013cc8d308d501a6ead8)`(`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` pid)` 

release packet_id.

#### Parameters
* `packet_id` packet_id to release 

This function is SYNC function that must only be called in the strand.

#### `public inline std::set< `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` > `[`get_qos2_publish_handled_pids`](#classasync__mqtt_1_1basic__endpoint_1a1aaf7274ef58eadf15428071cae9e894)`() const` 

Get processed but not released QoS2 packet ids This function should be called after disconnection.

#### Returns
set of packet_ids 

This function is SYNC function that must only be called in the strand.

#### `public inline void `[`restore_qos2_publish_handled_pids`](#classasync__mqtt_1_1basic__endpoint_1a4696744f07068176e48e12aeb4998fb0)`(std::set< `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` > pids)` 

Restore processed but not released QoS2 packet ids This function should be called before receive the first publish.

#### Parameters
* `pids` packet ids 

This function is SYNC function that must only be called in the strand.

#### `public inline void `[`restore_packets`](#classasync__mqtt_1_1basic__endpoint_1a6dfe47bd9ab1590e66f110e3dbe1087e)`(std::vector< basic_store_packet_variant< PacketIdBytes > > pvs)` 

restore packets the restored packets would automatically send when CONNACK packet is received

#### Parameters
* `pvs` packets to restore 

This function is SYNC function that must only be called in the strand.

#### `public inline std::vector< basic_store_packet_variant< PacketIdBytes > > `[`get_stored_packets`](#classasync__mqtt_1_1basic__endpoint_1a5ed8d45ffcfb114533d8de5ddddb4f92)`() const` 

get stored packets sotred packets mean inflight packets.

* PUBLISH packet (QoS1) not received PUBACK packet

* PUBLISH packet (QoS1) not received PUBREC packet

* PUBREL packet not received PUBCOMP packet 
#### Returns
std::vector<basic_store_packet_variant<PacketIdBytes>> 

This function is SYNC function that must only be called in the strand.

#### `public inline protocol_version `[`get_protocol_version`](#classasync__mqtt_1_1basic__endpoint_1a9cbabd5f427b1cb18d61ac49c7bbf83b)`() const` 

get MQTT protocol version

#### Returns
MQTT protocol version 

This function is SYNC function that must only be called in the strand.

#### `public inline void `[`cancel_all_timers_for_test`](#classasync__mqtt_1_1basic__endpoint_1afd81415fa06d1b4fcafc70273628df9e)`()` 

#### `public inline void `[`set_pingreq_send_interval_ms_for_test`](#classasync__mqtt_1_1basic__endpoint_1a94c953a39c2bacf5467a2bb21d2daac6)`(std::size_t ms)` 

#### `typedef `[`this_type`](#classasync__mqtt_1_1basic__endpoint_1aca223957602c95071273e7a07c3d2d0c) 

#### `typedef `[`stream_type`](#classasync__mqtt_1_1basic__endpoint_1acb36b8af5cc1833dea134c8efd7ce686) 

#### `typedef `[`next_layer_type`](#classasync__mqtt_1_1basic__endpoint_1a045005fba8b583a870cde28a58da70bb) 

#### `typedef `[`strand_type`](#classasync__mqtt_1_1basic__endpoint_1a842403c46cb455e467380f28d3a14289) 

#### `typedef `[`packet_variant_type`](#classasync__mqtt_1_1basic__endpoint_1a5098e22b58adbee93e452ede78ab1adc) 

Type of packet_variant.

#### `typedef `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090) 

Type of MQTT Packet Identifier.

# class `async_mqtt::buffer` 

buffer that has string_view interface This class provides string_view interface. This class hold string_view target's lifetime optionally.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public constexpr `[`buffer`](#classasync__mqtt_1_1buffer_1a322bad7e07196362bf2fa26d9c2b4f4f)`() = default` | 
`public  `[`buffer`](#classasync__mqtt_1_1buffer_1a87934dc330344f1a9751298a3b3ad944)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & other) = default` | 
`public inline constexpr `[`buffer`](#classasync__mqtt_1_1buffer_1a89dadefc440669338abeff8e8f282d45)`(char const * s,std::size_t count)` | 
`public inline constexpr `[`buffer`](#classasync__mqtt_1_1buffer_1a92d4036fb9070e0be851fa133b20c9b9)`(char const * s)` | 
`public template<>`  <br/>`inline constexpr `[`buffer`](#classasync__mqtt_1_1buffer_1a6354f235a64c4441d85d5874f341e2fe)`(It first,End last)` | 
`public inline constexpr explicit `[`buffer`](#classasync__mqtt_1_1buffer_1a780ef979588f850ef6c18a07b53c7a87)`(string_view sv)` | string_view constructor
`public  explicit `[`buffer`](#classasync__mqtt_1_1buffer_1a98e82df41b753fac853332478e12b57b)`(std::string) = delete` | string constructor (deleted)
`public inline  `[`buffer`](#classasync__mqtt_1_1buffer_1adeec7a30a6c57eed3dfd4347b968ad09)`(string_view sv,any life)` | string_view and lifetime constructor
`public inline  `[`buffer`](#classasync__mqtt_1_1buffer_1a94e33596225c837f897164d0ec5d1b4f)`(char const * s,std::size_t count,any life)` | 
`public inline  `[`buffer`](#classasync__mqtt_1_1buffer_1a54225593cab400b32fa7c04692be815f)`(char const * s,any life)` | 
`public template<>`  <br/>`inline  `[`buffer`](#classasync__mqtt_1_1buffer_1a803a5b7ba6e71eef2d00b0076821b97b)`(It first,End last,any life)` | 
`public `[`buffer`](#classasync__mqtt_1_1buffer)` & `[`operator=`](#classasync__mqtt_1_1buffer_1a39fcab01fb2e4b1c8a0823090c8e1c9c)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf) = default` | 
`public inline constexpr const_iterator `[`begin`](#classasync__mqtt_1_1buffer_1a8da7f19412f5299b994d39fba3b7fc52)`() const noexcept` | 
`public inline constexpr const_iterator `[`cbegin`](#classasync__mqtt_1_1buffer_1a8f235c10fb94b0cab026d59908f8285d)`() const noexcept` | 
`public inline constexpr const_iterator `[`end`](#classasync__mqtt_1_1buffer_1a6c5813d17def864882cac42d1f453308)`() const noexcept` | 
`public inline constexpr const_iterator `[`cend`](#classasync__mqtt_1_1buffer_1a6c0fddafd68f9a02bf999e13b46dce50)`() const noexcept` | 
`public inline constexpr const_reverse_iterator `[`rbegin`](#classasync__mqtt_1_1buffer_1a114d7ad7bdf186c19ead1682e8a29ab6)`() const noexcept` | 
`public inline constexpr const_reverse_iterator `[`crbegin`](#classasync__mqtt_1_1buffer_1a92ed7db78da4b1cd5f8de8acab7a7bbe)`() const noexcept` | 
`public inline constexpr const_reverse_iterator `[`rend`](#classasync__mqtt_1_1buffer_1ae474cca6eba0e784c1985bf0e61940e4)`() const noexcept` | 
`public inline constexpr const_reverse_iterator `[`crend`](#classasync__mqtt_1_1buffer_1affa80c740527fd62f738cc271bc9c702)`() const noexcept` | 
`public inline constexpr const_reference `[`operator[]`](#classasync__mqtt_1_1buffer_1a1856703f5cd06e90f23a52281b92b700)`(size_type pos) const` | 
`public inline constexpr const_reference `[`at`](#classasync__mqtt_1_1buffer_1ad876c3edb8c22366e1cbc2c0aee9aa03)`(size_type pos) const` | 
`public inline constexpr const_reference `[`front`](#classasync__mqtt_1_1buffer_1acac343b34db354481dc49050166c399f)`() const` | 
`public inline constexpr const_reference `[`back`](#classasync__mqtt_1_1buffer_1a83cd74134e40075e57dc00cd9e08c703)`() const` | 
`public inline constexpr const_pointer `[`data`](#classasync__mqtt_1_1buffer_1aacbd3a5eefbf1dd4e572fdf1fe9d6a86)`() const noexcept` | 
`public inline constexpr size_type `[`size`](#classasync__mqtt_1_1buffer_1a6e30bb89d7851e7a5568b22ef7912838)`() const noexcept` | 
`public inline constexpr size_type `[`length`](#classasync__mqtt_1_1buffer_1a848e929b62ec91b0a8c3cd902692d944)`() const noexcept` | 
`public inline constexpr size_type `[`max_size`](#classasync__mqtt_1_1buffer_1aabb5b7e300fc71f9e8b7d78400e9f411)`() const noexcept` | 
`public inline constexpr bool `[`empty`](#classasync__mqtt_1_1buffer_1a5a6ec08901878976a702825394f24540)`() const noexcept` | 
`public inline constexpr void `[`remove_prefix`](#classasync__mqtt_1_1buffer_1a32b4cc2dd0ffb3edcf65361f8e12a62d)`(size_type n)` | 
`public inline constexpr void `[`remove_suffix`](#classasync__mqtt_1_1buffer_1a423e1a01c90d85823a943dab3aa364f9)`(size_type n)` | 
`public inline void `[`swap`](#classasync__mqtt_1_1buffer_1a5d5d131740b5cb75e29f174db8c08218)`(`[`buffer`](#classasync__mqtt_1_1buffer)` & buf) noexcept` | 
`public inline size_type `[`copy`](#classasync__mqtt_1_1buffer_1a9554ca7b3e648e0d6a07b3a5c37c95a6)`(char * dest,size_type count,size_type pos) const` | 
`public inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`substr`](#classasync__mqtt_1_1buffer_1a7895963705a5671147e3cfb366f48756)`(size_type pos,size_type count) const` | get substring The returned buffer ragnge is the same as string_view::substr(). In addition the lifetime is shared between returned buffer and this buffer.
`public inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`substr`](#classasync__mqtt_1_1buffer_1ad3d64b3893dfec271b87d8f2e3dd6b65)`(size_type pos,size_type count)` | get substring The returned buffer ragnge is the same as string_view::substr(). In addition the lifetime is moved to returned buffer.
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1aa7b3127002e0a9f2f0641ecad4daea24)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf) const noexcept` | 
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a3d4901baa363cb92e3d01e07b59e1955)`(string_view const & v) const noexcept` | 
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1aa87697e3a25d39f5cf79f86c0ceed63a)`(size_type pos1,size_type count1,`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf) const noexcept` | 
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a4cc03050af472a8949362ae209b6c0ba)`(size_type pos1,size_type count1,string_view const & v) const noexcept` | 
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a54f450efa2f2cd9b99b9eeb734fae6ce)`(size_type pos1,size_type count1,`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos2,size_type count2) const noexcept` | 
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a49967806333698669f754c4c562566cd)`(size_type pos1,size_type count1,string_view const & v,size_type pos2,size_type count2) const noexcept` | 
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1afdc5067111459a6bd4877bf1259a4d5b)`(char const * s) const noexcept` | 
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a33e5f60628b2fd9845e849f4c694f40a)`(size_type pos1,size_type count1,char const * s) const noexcept` | 
`public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a88206280e6d162885e24543f3149537c)`(size_type pos1,size_type count1,char const * s,size_type pos2,size_type count2) const noexcept` | 
`public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1a269bf3927431f407bf18859cf5d4ea16)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1a37a42f8f178e66f0a56a91d1c963890e)`(string_view v,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1abfdcf270dd250f87a467c0a9e68525dc)`(char ch,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1a5421e369b822908736fd3a1f4d77fe5e)`(char const * s,size_type pos,size_type count) const` | 
`public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1a1837a1661fa939b84149c8a9588cc2e0)`(char const * s,size_type pos) const` | 
`public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1a10863870aa464f6a509b5039404e1aa6)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1a25ae854f6e49503d48b5df806a6577b0)`(string_view v,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1ae2379c0aa302383e2b8711b1fed57701)`(char ch,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1a88bdf0c7d13724a34e5e5d87b98b08e0)`(char const * s,size_type pos,size_type count) const` | 
`public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1a71f24316808f123dfc1b1fb82136f553)`(char const * s,size_type pos) const` | 
`public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1af2d0f456beae3bd6c59c744337ffb5a8)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1abbc8daa4d30f47084bb4963382c211c5)`(string_view v,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1ab257a32ccbdbb3b6fc644ce667c5f0da)`(char ch,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1a7fcc276cd25eb222d707c7da90183608)`(char const * s,size_type pos,size_type count) const` | 
`public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1ac14120395e1b853753b61b3458bd87d8)`(char const * s,size_type pos) const` | 
`public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1a95ccbd06d151e57f5d88ae5434d1f057)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1a6a238707cf21d7e8f90f8530c366f24c)`(string_view v,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1a23320f42eb2c652180ba2bf8a93e01f9)`(char ch,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1aa779199f5fdca4f2fb5bdfd33ea00869)`(char const * s,size_type pos,size_type count) const` | 
`public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1a8412369bdf77e8578bab6c04b857aa3a)`(char const * s,size_type pos) const` | 
`public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1add2a12857383c8b812b3b9856d1f0080)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1ae8e84a055b62022b2af19caf9bff06c9)`(string_view v,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1a0e310286c1d17d756753b59f5b1b62a9)`(char ch,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1afe0f4a342d1e87f409dd4ffdac5c9b9e)`(char const * s,size_type pos,size_type count) const` | 
`public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1a04cf72c6074687c8b4ba9e06f5b75dcf)`(char const * s,size_type pos) const` | 
`public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a9b450eade43961525166594c3372d00f)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a0211191f8238b397f57e116ec5dc2db6)`(string_view v,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a4b8d08173e2701fa900b13edd1507333)`(char ch,size_type pos) const noexcept` | 
`public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a875877ea75c6813f05d7a5270ce6ff67)`(char const * s,size_type pos,size_type count) const` | 
`public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a7be6aa333afd34a1f3d1f15e97f8f47e)`(char const * s,size_type pos) const` | 
`public inline any const & `[`get_life`](#classasync__mqtt_1_1buffer_1a73c513e21f0d58f89d4e61b4783ab6a4)`() const` | 
`public inline  `[`operator as::const_buffer`](#classasync__mqtt_1_1buffer_1ae9df5707a65af78f307095c7eae86d49)`() const` | 
`public inline  `[`operator string_view`](#classasync__mqtt_1_1buffer_1aaee4b6c6bb04a9b9413e12424d2efccc)`() const` | 
`typedef `[`traits_type`](#classasync__mqtt_1_1buffer_1add4508263680dd8eeec910a6c371faca) | 
`typedef `[`value_type`](#classasync__mqtt_1_1buffer_1a568663b3541fe41292ee631d0e319794) | 
`typedef `[`pointer`](#classasync__mqtt_1_1buffer_1aa55754e1238a5756abd7a33940b00f8b) | 
`typedef `[`const_pointer`](#classasync__mqtt_1_1buffer_1a22e1f5d7c99aa6ad203e72ce5d79cef9) | 
`typedef `[`reference`](#classasync__mqtt_1_1buffer_1aee326e956bef57a1750ecf03adab1d1f) | 
`typedef `[`const_reference`](#classasync__mqtt_1_1buffer_1a1f7a8b012f88956141e87fe833e043fd) | 
`typedef `[`iterator`](#classasync__mqtt_1_1buffer_1afa8cb2bb7d3d85d307272d7938996466) | 
`typedef `[`const_iterator`](#classasync__mqtt_1_1buffer_1adc4a8a587e893e97edb96e8c3b755fad) | 
`typedef `[`const_reverse_iterator`](#classasync__mqtt_1_1buffer_1a2340e019992e738f974393388046cd50) | 
`typedef `[`reverse_iterator`](#classasync__mqtt_1_1buffer_1aea029dbfc932ad0dbd54ada73ffd733d) | 
`typedef `[`size_type`](#classasync__mqtt_1_1buffer_1a7ffdffa3bcfa761c1ea30f5705a65b28) | 
`typedef `[`difference_type`](#classasync__mqtt_1_1buffer_1acd52b78b9c1e1ad7bd3a4717489f8aea) | 

## Members

#### `public constexpr `[`buffer`](#classasync__mqtt_1_1buffer_1a322bad7e07196362bf2fa26d9c2b4f4f)`() = default` 

#### `public  `[`buffer`](#classasync__mqtt_1_1buffer_1a87934dc330344f1a9751298a3b3ad944)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & other) = default` 

#### `public inline constexpr `[`buffer`](#classasync__mqtt_1_1buffer_1a89dadefc440669338abeff8e8f282d45)`(char const * s,std::size_t count)` 

#### `public inline constexpr `[`buffer`](#classasync__mqtt_1_1buffer_1a92d4036fb9070e0be851fa133b20c9b9)`(char const * s)` 

#### `public template<>`  <br/>`inline constexpr `[`buffer`](#classasync__mqtt_1_1buffer_1a6354f235a64c4441d85d5874f341e2fe)`(It first,End last)` 

#### `public inline constexpr explicit `[`buffer`](#classasync__mqtt_1_1buffer_1a780ef979588f850ef6c18a07b53c7a87)`(string_view sv)` 

string_view constructor

#### Parameters
* `sv` string_view This constructor doesn't hold the sv target's lifetime. It behaves as string_view. Caller needs to manage the target lifetime.

#### `public  explicit `[`buffer`](#classasync__mqtt_1_1buffer_1a98e82df41b753fac853332478e12b57b)`(std::string) = delete` 

string constructor (deleted)

#### Parameters
* `string` This constructor is intentionally deleted. Consider `buffer(std::string("ABC"))`, the buffer points to dangling reference.

#### `public inline  `[`buffer`](#classasync__mqtt_1_1buffer_1adeec7a30a6c57eed3dfd4347b968ad09)`(string_view sv,any life)` 

string_view and lifetime constructor

#### Parameters
* `sv` string_view 

* `sp` shared_ptr_array that holds sv target's lifetime If user creates buffer via this constructor, sp's lifetime is held by the buffer.

#### `public inline  `[`buffer`](#classasync__mqtt_1_1buffer_1a94e33596225c837f897164d0ec5d1b4f)`(char const * s,std::size_t count,any life)` 

#### `public inline  `[`buffer`](#classasync__mqtt_1_1buffer_1a54225593cab400b32fa7c04692be815f)`(char const * s,any life)` 

#### `public template<>`  <br/>`inline  `[`buffer`](#classasync__mqtt_1_1buffer_1a803a5b7ba6e71eef2d00b0076821b97b)`(It first,End last,any life)` 

#### `public `[`buffer`](#classasync__mqtt_1_1buffer)` & `[`operator=`](#classasync__mqtt_1_1buffer_1a39fcab01fb2e4b1c8a0823090c8e1c9c)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf) = default` 

#### `public inline constexpr const_iterator `[`begin`](#classasync__mqtt_1_1buffer_1a8da7f19412f5299b994d39fba3b7fc52)`() const noexcept` 

#### `public inline constexpr const_iterator `[`cbegin`](#classasync__mqtt_1_1buffer_1a8f235c10fb94b0cab026d59908f8285d)`() const noexcept` 

#### `public inline constexpr const_iterator `[`end`](#classasync__mqtt_1_1buffer_1a6c5813d17def864882cac42d1f453308)`() const noexcept` 

#### `public inline constexpr const_iterator `[`cend`](#classasync__mqtt_1_1buffer_1a6c0fddafd68f9a02bf999e13b46dce50)`() const noexcept` 

#### `public inline constexpr const_reverse_iterator `[`rbegin`](#classasync__mqtt_1_1buffer_1a114d7ad7bdf186c19ead1682e8a29ab6)`() const noexcept` 

#### `public inline constexpr const_reverse_iterator `[`crbegin`](#classasync__mqtt_1_1buffer_1a92ed7db78da4b1cd5f8de8acab7a7bbe)`() const noexcept` 

#### `public inline constexpr const_reverse_iterator `[`rend`](#classasync__mqtt_1_1buffer_1ae474cca6eba0e784c1985bf0e61940e4)`() const noexcept` 

#### `public inline constexpr const_reverse_iterator `[`crend`](#classasync__mqtt_1_1buffer_1affa80c740527fd62f738cc271bc9c702)`() const noexcept` 

#### `public inline constexpr const_reference `[`operator[]`](#classasync__mqtt_1_1buffer_1a1856703f5cd06e90f23a52281b92b700)`(size_type pos) const` 

#### `public inline constexpr const_reference `[`at`](#classasync__mqtt_1_1buffer_1ad876c3edb8c22366e1cbc2c0aee9aa03)`(size_type pos) const` 

#### `public inline constexpr const_reference `[`front`](#classasync__mqtt_1_1buffer_1acac343b34db354481dc49050166c399f)`() const` 

#### `public inline constexpr const_reference `[`back`](#classasync__mqtt_1_1buffer_1a83cd74134e40075e57dc00cd9e08c703)`() const` 

#### `public inline constexpr const_pointer `[`data`](#classasync__mqtt_1_1buffer_1aacbd3a5eefbf1dd4e572fdf1fe9d6a86)`() const noexcept` 

#### `public inline constexpr size_type `[`size`](#classasync__mqtt_1_1buffer_1a6e30bb89d7851e7a5568b22ef7912838)`() const noexcept` 

#### `public inline constexpr size_type `[`length`](#classasync__mqtt_1_1buffer_1a848e929b62ec91b0a8c3cd902692d944)`() const noexcept` 

#### `public inline constexpr size_type `[`max_size`](#classasync__mqtt_1_1buffer_1aabb5b7e300fc71f9e8b7d78400e9f411)`() const noexcept` 

#### `public inline constexpr bool `[`empty`](#classasync__mqtt_1_1buffer_1a5a6ec08901878976a702825394f24540)`() const noexcept` 

#### `public inline constexpr void `[`remove_prefix`](#classasync__mqtt_1_1buffer_1a32b4cc2dd0ffb3edcf65361f8e12a62d)`(size_type n)` 

#### `public inline constexpr void `[`remove_suffix`](#classasync__mqtt_1_1buffer_1a423e1a01c90d85823a943dab3aa364f9)`(size_type n)` 

#### `public inline void `[`swap`](#classasync__mqtt_1_1buffer_1a5d5d131740b5cb75e29f174db8c08218)`(`[`buffer`](#classasync__mqtt_1_1buffer)` & buf) noexcept` 

#### `public inline size_type `[`copy`](#classasync__mqtt_1_1buffer_1a9554ca7b3e648e0d6a07b3a5c37c95a6)`(char * dest,size_type count,size_type pos) const` 

#### `public inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`substr`](#classasync__mqtt_1_1buffer_1a7895963705a5671147e3cfb366f48756)`(size_type pos,size_type count) const` 

get substring The returned buffer ragnge is the same as string_view::substr(). In addition the lifetime is shared between returned buffer and this buffer.

#### Parameters
* `offset` offset point of the buffer 

* `length` length of the buffer, If the length is string_view::npos then the length is from offset to the end of string.

#### `public inline `[`buffer`](#classasync__mqtt_1_1buffer)` `[`substr`](#classasync__mqtt_1_1buffer_1ad3d64b3893dfec271b87d8f2e3dd6b65)`(size_type pos,size_type count)` 

get substring The returned buffer ragnge is the same as string_view::substr(). In addition the lifetime is moved to returned buffer.

#### Parameters
* `offset` offset point of the buffer 

* `length` length of the buffer, If the length is string_view::npos then the length is from offset to the end of string.

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1aa7b3127002e0a9f2f0641ecad4daea24)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf) const noexcept` 

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a3d4901baa363cb92e3d01e07b59e1955)`(string_view const & v) const noexcept` 

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1aa87697e3a25d39f5cf79f86c0ceed63a)`(size_type pos1,size_type count1,`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf) const noexcept` 

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a4cc03050af472a8949362ae209b6c0ba)`(size_type pos1,size_type count1,string_view const & v) const noexcept` 

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a54f450efa2f2cd9b99b9eeb734fae6ce)`(size_type pos1,size_type count1,`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos2,size_type count2) const noexcept` 

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a49967806333698669f754c4c562566cd)`(size_type pos1,size_type count1,string_view const & v,size_type pos2,size_type count2) const noexcept` 

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1afdc5067111459a6bd4877bf1259a4d5b)`(char const * s) const noexcept` 

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a33e5f60628b2fd9845e849f4c694f40a)`(size_type pos1,size_type count1,char const * s) const noexcept` 

#### `public inline constexpr int `[`compare`](#classasync__mqtt_1_1buffer_1a88206280e6d162885e24543f3149537c)`(size_type pos1,size_type count1,char const * s,size_type pos2,size_type count2) const noexcept` 

#### `public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1a269bf3927431f407bf18859cf5d4ea16)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1a37a42f8f178e66f0a56a91d1c963890e)`(string_view v,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1abfdcf270dd250f87a467c0a9e68525dc)`(char ch,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1a5421e369b822908736fd3a1f4d77fe5e)`(char const * s,size_type pos,size_type count) const` 

#### `public inline constexpr size_type `[`find`](#classasync__mqtt_1_1buffer_1a1837a1661fa939b84149c8a9588cc2e0)`(char const * s,size_type pos) const` 

#### `public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1a10863870aa464f6a509b5039404e1aa6)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1a25ae854f6e49503d48b5df806a6577b0)`(string_view v,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1ae2379c0aa302383e2b8711b1fed57701)`(char ch,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1a88bdf0c7d13724a34e5e5d87b98b08e0)`(char const * s,size_type pos,size_type count) const` 

#### `public inline constexpr size_type `[`rfind`](#classasync__mqtt_1_1buffer_1a71f24316808f123dfc1b1fb82136f553)`(char const * s,size_type pos) const` 

#### `public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1af2d0f456beae3bd6c59c744337ffb5a8)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1abbc8daa4d30f47084bb4963382c211c5)`(string_view v,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1ab257a32ccbdbb3b6fc644ce667c5f0da)`(char ch,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1a7fcc276cd25eb222d707c7da90183608)`(char const * s,size_type pos,size_type count) const` 

#### `public inline constexpr size_type `[`find_first_of`](#classasync__mqtt_1_1buffer_1ac14120395e1b853753b61b3458bd87d8)`(char const * s,size_type pos) const` 

#### `public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1a95ccbd06d151e57f5d88ae5434d1f057)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1a6a238707cf21d7e8f90f8530c366f24c)`(string_view v,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1a23320f42eb2c652180ba2bf8a93e01f9)`(char ch,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1aa779199f5fdca4f2fb5bdfd33ea00869)`(char const * s,size_type pos,size_type count) const` 

#### `public inline constexpr size_type `[`find_last_of`](#classasync__mqtt_1_1buffer_1a8412369bdf77e8578bab6c04b857aa3a)`(char const * s,size_type pos) const` 

#### `public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1add2a12857383c8b812b3b9856d1f0080)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1ae8e84a055b62022b2af19caf9bff06c9)`(string_view v,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1a0e310286c1d17d756753b59f5b1b62a9)`(char ch,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1afe0f4a342d1e87f409dd4ffdac5c9b9e)`(char const * s,size_type pos,size_type count) const` 

#### `public inline constexpr size_type `[`find_first_not_of`](#classasync__mqtt_1_1buffer_1a04cf72c6074687c8b4ba9e06f5b75dcf)`(char const * s,size_type pos) const` 

#### `public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a9b450eade43961525166594c3372d00f)`(`[`buffer`](#classasync__mqtt_1_1buffer)` const & buf,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a0211191f8238b397f57e116ec5dc2db6)`(string_view v,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a4b8d08173e2701fa900b13edd1507333)`(char ch,size_type pos) const noexcept` 

#### `public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a875877ea75c6813f05d7a5270ce6ff67)`(char const * s,size_type pos,size_type count) const` 

#### `public inline constexpr size_type `[`find_last_not_of`](#classasync__mqtt_1_1buffer_1a7be6aa333afd34a1f3d1f15e97f8f47e)`(char const * s,size_type pos) const` 

#### `public inline any const & `[`get_life`](#classasync__mqtt_1_1buffer_1a73c513e21f0d58f89d4e61b4783ab6a4)`() const` 

#### `public inline  `[`operator as::const_buffer`](#classasync__mqtt_1_1buffer_1ae9df5707a65af78f307095c7eae86d49)`() const` 

#### `public inline  `[`operator string_view`](#classasync__mqtt_1_1buffer_1aaee4b6c6bb04a9b9413e12424d2efccc)`() const` 

#### `typedef `[`traits_type`](#classasync__mqtt_1_1buffer_1add4508263680dd8eeec910a6c371faca) 

#### `typedef `[`value_type`](#classasync__mqtt_1_1buffer_1a568663b3541fe41292ee631d0e319794) 

#### `typedef `[`pointer`](#classasync__mqtt_1_1buffer_1aa55754e1238a5756abd7a33940b00f8b) 

#### `typedef `[`const_pointer`](#classasync__mqtt_1_1buffer_1a22e1f5d7c99aa6ad203e72ce5d79cef9) 

#### `typedef `[`reference`](#classasync__mqtt_1_1buffer_1aee326e956bef57a1750ecf03adab1d1f) 

#### `typedef `[`const_reference`](#classasync__mqtt_1_1buffer_1a1f7a8b012f88956141e87fe833e043fd) 

#### `typedef `[`iterator`](#classasync__mqtt_1_1buffer_1afa8cb2bb7d3d85d307272d7938996466) 

#### `typedef `[`const_iterator`](#classasync__mqtt_1_1buffer_1adc4a8a587e893e97edb96e8c3b755fad) 

#### `typedef `[`const_reverse_iterator`](#classasync__mqtt_1_1buffer_1a2340e019992e738f974393388046cd50) 

#### `typedef `[`reverse_iterator`](#classasync__mqtt_1_1buffer_1aea029dbfc932ad0dbd54ada73ffd733d) 

#### `typedef `[`size_type`](#classasync__mqtt_1_1buffer_1a7ffdffa3bcfa761c1ea30f5705a65b28) 

#### `typedef `[`difference_type`](#classasync__mqtt_1_1buffer_1acd52b78b9c1e1ad7bd3a4717489f8aea) 

# class `async_mqtt::packet_id_manager` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline optional< packet_id_type > `[`acquire_unique_id`](#classasync__mqtt_1_1packet__id__manager_1a931a775f4a44645a6051b2946cb5de44)`()` | Acquire the new unique packet id. If all packet ids are already in use, then returns nullopt After acquiring the packet id, you can call acquired_* functions. The ownership of packet id is moved to the library. Or you can call release_packet_id to release it.
`public inline bool `[`register_id`](#classasync__mqtt_1_1packet__id__manager_1a7fac5dedb7074ba366d1365b90a66a5f)`(packet_id_type packet_id)` | Register packet_id to the library. After registering the packet_id, you can call acquired_* functions. The ownership of packet id is moved to the library. Or you can call release_packet_id to release it.
`public inline bool `[`is_used_id`](#classasync__mqtt_1_1packet__id__manager_1af8617ead0b4c033dc3cf06909302ee03)`(packet_id_type packet_id) const` | Check packet_id is used.
`public inline void `[`release_id`](#classasync__mqtt_1_1packet__id__manager_1a941cad5643549f346373257a206cbfde)`(packet_id_type packet_id)` | Release packet_id.
`public inline void `[`clear`](#classasync__mqtt_1_1packet__id__manager_1a1f565a006d566ce3965dce5ab190e9d0)`()` | Clear all packet ids.

## Members

#### `public inline optional< packet_id_type > `[`acquire_unique_id`](#classasync__mqtt_1_1packet__id__manager_1a931a775f4a44645a6051b2946cb5de44)`()` 

Acquire the new unique packet id. If all packet ids are already in use, then returns nullopt After acquiring the packet id, you can call acquired_* functions. The ownership of packet id is moved to the library. Or you can call release_packet_id to release it.

#### Returns
packet id

#### `public inline bool `[`register_id`](#classasync__mqtt_1_1packet__id__manager_1a7fac5dedb7074ba366d1365b90a66a5f)`(packet_id_type packet_id)` 

Register packet_id to the library. After registering the packet_id, you can call acquired_* functions. The ownership of packet id is moved to the library. Or you can call release_packet_id to release it.

#### Returns
If packet_id is successfully registerd then return true, otherwise return false.

#### `public inline bool `[`is_used_id`](#classasync__mqtt_1_1packet__id__manager_1af8617ead0b4c033dc3cf06909302ee03)`(packet_id_type packet_id) const` 

Check packet_id is used.

#### Returns
If packet_id is used then return true, otherwise return false.

#### `public inline void `[`release_id`](#classasync__mqtt_1_1packet__id__manager_1a941cad5643549f346373257a206cbfde)`(packet_id_type packet_id)` 

Release packet_id.

#### Parameters
* `packet_id` packet id to release. only the packet_id gotten by acquire_unique_packet_id, or register_packet_id is permitted.

#### `public inline void `[`clear`](#classasync__mqtt_1_1packet__id__manager_1a1f565a006d566ce3965dce5ab190e9d0)`()` 

Clear all packet ids.

# class `async_mqtt::store` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  `[`store`](#classasync__mqtt_1_1store_1a73048759602a86ad44414b0083ff77e4)`(Executor exe)` | 
`public template<>`  <br/>`inline bool `[`add`](#classasync__mqtt_1_1store_1aadcca8183cde38a1783fcd799a5700de)`(Packet const & packet)` | 
`public inline bool `[`erase`](#classasync__mqtt_1_1store_1a4b5d0d775cb451597cfc6be0e1d52f03)`(response_packet r,packet_id_t packet_id)` | 
`public inline void `[`clear`](#classasync__mqtt_1_1store_1a9636c6bb333e7fa8b0e950f70cc09be6)`()` | 
`public template<>`  <br/>`inline void `[`for_each`](#classasync__mqtt_1_1store_1ae8e1fc98f1d25a2fdd00a0dbad45416b)`(Func const & func)` | 
`public inline std::vector< store_packet_t > `[`get_stored`](#classasync__mqtt_1_1store_1aa40dc8b4db1fe1e94cf0961b2685b741)`() const` | 
`typedef `[`packet_id_t`](#classasync__mqtt_1_1store_1abdd6a1fb15dc173ec4b6b2e7232e2718) | 
`typedef `[`store_packet_t`](#classasync__mqtt_1_1store_1ac89e9d0c919cedfeb1a89440b41ade61) | 

## Members

#### `public inline  `[`store`](#classasync__mqtt_1_1store_1a73048759602a86ad44414b0083ff77e4)`(Executor exe)` 

#### `public template<>`  <br/>`inline bool `[`add`](#classasync__mqtt_1_1store_1aadcca8183cde38a1783fcd799a5700de)`(Packet const & packet)` 

#### `public inline bool `[`erase`](#classasync__mqtt_1_1store_1a4b5d0d775cb451597cfc6be0e1d52f03)`(response_packet r,packet_id_t packet_id)` 

#### `public inline void `[`clear`](#classasync__mqtt_1_1store_1a9636c6bb333e7fa8b0e950f70cc09be6)`()` 

#### `public template<>`  <br/>`inline void `[`for_each`](#classasync__mqtt_1_1store_1ae8e1fc98f1d25a2fdd00a0dbad45416b)`(Func const & func)` 

#### `public inline std::vector< store_packet_t > `[`get_stored`](#classasync__mqtt_1_1store_1aa40dc8b4db1fe1e94cf0961b2685b741)`() const` 

#### `typedef `[`packet_id_t`](#classasync__mqtt_1_1store_1abdd6a1fb15dc173ec4b6b2e7232e2718) 

#### `typedef `[`store_packet_t`](#classasync__mqtt_1_1store_1ac89e9d0c919cedfeb1a89440b41ade61) 

# class `async_mqtt::stream` 

```
class async_mqtt::stream
  : public std::enable_shared_from_this< stream< NextLayer > >
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public template<>`  <br/>`inline  explicit `[`stream`](#classasync__mqtt_1_1stream_1a8d81fdb71caf669977c1fc8553647231)`(T && t,Args &&... args)` | 
`public inline  `[`~stream`](#classasync__mqtt_1_1stream_1a9f6b4cd7164f3d9ebebaf6c52787218d)`()` | 
`public  `[`stream`](#classasync__mqtt_1_1stream_1a0122cdd138df3b5d076ec7b17fe5b8bb)`(this_type &&) = delete` | 
`public  `[`stream`](#classasync__mqtt_1_1stream_1a96185eea561f9dc323d082613aecd678)`(this_type const &) = delete` | 
`public this_type & `[`operator=`](#classasync__mqtt_1_1stream_1a92ffed882a9ee0704708b635d00fe824)`(this_type &&) = delete` | 
`public this_type & `[`operator=`](#classasync__mqtt_1_1stream_1a954b37a9d9d79c7c86222470278042b3)`(this_type const &) = delete` | 
`public inline auto const & `[`next_layer`](#classasync__mqtt_1_1stream_1a15fde7ee2f209b98217c356e3aac41b6)`() const` | 
`public inline auto & `[`next_layer`](#classasync__mqtt_1_1stream_1a79895064092db13081e2e256d8b05c4e)`()` | 
`public inline auto const & `[`lowest_layer`](#classasync__mqtt_1_1stream_1a2a91077b014641f89d87b728ac928c59)`() const` | 
`public inline auto & `[`lowest_layer`](#classasync__mqtt_1_1stream_1a9afba7bdc94e3ccbd591793898982cc7)`()` | 
`public inline auto `[`get_executor`](#classasync__mqtt_1_1stream_1a17e047fc4e32eedc1c52883e8e72cb29)`() const` | 
`public inline auto `[`get_executor`](#classasync__mqtt_1_1stream_1aa89a460ab9caeb9179e45f2d6662f8ed)`()` | 
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(error_code, `[`buffer`](#classasync__mqtt_1_1buffer))`>::return_type `[`read_packet`](#classasync__mqtt_1_1stream_1a98c9c72d7b3bb01524c1cc1812e4670f)`(CompletionToken && token)` | 
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(error_code, std::size_t)>::return_type `[`write_packet`](#classasync__mqtt_1_1stream_1af38f933080b2fe07fd60b1252d01aadd)`(Packet packet,CompletionToken && token)` | 
`public inline strand_type const & `[`strand`](#classasync__mqtt_1_1stream_1a006da432e3c644c83a63603c79f5f8c5)`() const` | 
`public inline strand_type & `[`strand`](#classasync__mqtt_1_1stream_1a34107fdead3596458b607818e12f40c9)`()` | 
`public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(error_code)>::return_type `[`close`](#classasync__mqtt_1_1stream_1a78ad237a90293d7c0161e9a82f078856)`(CompletionToken && token)` | 
`typedef `[`this_type`](#classasync__mqtt_1_1stream_1aa5aae24c681aa955e0ff3f7ffab0cd96) | 
`typedef `[`this_type_sp`](#classasync__mqtt_1_1stream_1aecc9552ae8f6bb9775f36e38386676f8) | 
`typedef `[`next_layer_type`](#classasync__mqtt_1_1stream_1a14b47fe56cd26a9d4183eee7ab0c5766) | 
`typedef `[`executor_type`](#classasync__mqtt_1_1stream_1aa0b3d7d66307f0d974a4f08eb0787531) | 
`typedef `[`strand_type`](#classasync__mqtt_1_1stream_1a6bcc7703c1a55f196367d101a1c4568a) | 

## Members

#### `public template<>`  <br/>`inline  explicit `[`stream`](#classasync__mqtt_1_1stream_1a8d81fdb71caf669977c1fc8553647231)`(T && t,Args &&... args)` 

#### `public inline  `[`~stream`](#classasync__mqtt_1_1stream_1a9f6b4cd7164f3d9ebebaf6c52787218d)`()` 

#### `public  `[`stream`](#classasync__mqtt_1_1stream_1a0122cdd138df3b5d076ec7b17fe5b8bb)`(this_type &&) = delete` 

#### `public  `[`stream`](#classasync__mqtt_1_1stream_1a96185eea561f9dc323d082613aecd678)`(this_type const &) = delete` 

#### `public this_type & `[`operator=`](#classasync__mqtt_1_1stream_1a92ffed882a9ee0704708b635d00fe824)`(this_type &&) = delete` 

#### `public this_type & `[`operator=`](#classasync__mqtt_1_1stream_1a954b37a9d9d79c7c86222470278042b3)`(this_type const &) = delete` 

#### `public inline auto const & `[`next_layer`](#classasync__mqtt_1_1stream_1a15fde7ee2f209b98217c356e3aac41b6)`() const` 

#### `public inline auto & `[`next_layer`](#classasync__mqtt_1_1stream_1a79895064092db13081e2e256d8b05c4e)`()` 

#### `public inline auto const & `[`lowest_layer`](#classasync__mqtt_1_1stream_1a2a91077b014641f89d87b728ac928c59)`() const` 

#### `public inline auto & `[`lowest_layer`](#classasync__mqtt_1_1stream_1a9afba7bdc94e3ccbd591793898982cc7)`()` 

#### `public inline auto `[`get_executor`](#classasync__mqtt_1_1stream_1a17e047fc4e32eedc1c52883e8e72cb29)`() const` 

#### `public inline auto `[`get_executor`](#classasync__mqtt_1_1stream_1aa89a460ab9caeb9179e45f2d6662f8ed)`()` 

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(error_code, `[`buffer`](#classasync__mqtt_1_1buffer))`>::return_type `[`read_packet`](#classasync__mqtt_1_1stream_1a98c9c72d7b3bb01524c1cc1812e4670f)`(CompletionToken && token)` 

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(error_code, std::size_t)>::return_type `[`write_packet`](#classasync__mqtt_1_1stream_1af38f933080b2fe07fd60b1252d01aadd)`(Packet packet,CompletionToken && token)` 

#### `public inline strand_type const & `[`strand`](#classasync__mqtt_1_1stream_1a006da432e3c644c83a63603c79f5f8c5)`() const` 

#### `public inline strand_type & `[`strand`](#classasync__mqtt_1_1stream_1a34107fdead3596458b607818e12f40c9)`()` 

#### `public template<>`  <br/>`inline as::async_result< std::decay_t< CompletionToken >, void(error_code)>::return_type `[`close`](#classasync__mqtt_1_1stream_1a78ad237a90293d7c0161e9a82f078856)`(CompletionToken && token)` 

#### `typedef `[`this_type`](#classasync__mqtt_1_1stream_1aa5aae24c681aa955e0ff3f7ffab0cd96) 

#### `typedef `[`this_type_sp`](#classasync__mqtt_1_1stream_1aecc9552ae8f6bb9775f36e38386676f8) 

#### `typedef `[`next_layer_type`](#classasync__mqtt_1_1stream_1a14b47fe56cd26a9d4183eee7ab0c5766) 

#### `typedef `[`executor_type`](#classasync__mqtt_1_1stream_1aa0b3d7d66307f0d974a4f08eb0787531) 

#### `typedef `[`strand_type`](#classasync__mqtt_1_1stream_1a6bcc7703c1a55f196367d101a1c4568a) 

# class `async_mqtt::topic_alias_recv` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  `[`topic_alias_recv`](#classasync__mqtt_1_1topic__alias__recv_1af4afaa44f918d07493f81c2e2e36a7f1)`(topic_alias_t max)` | 
`public inline void `[`insert_or_update`](#classasync__mqtt_1_1topic__alias__recv_1aab0f09fa093083ec07f218617fe49ea5)`(string_view topic,topic_alias_t alias)` | 
`public inline std::string `[`find`](#classasync__mqtt_1_1topic__alias__recv_1a3be208a199fb1558b015ef897c92a788)`(topic_alias_t alias) const` | 
`public inline void `[`clear`](#classasync__mqtt_1_1topic__alias__recv_1ab7445d987693c09f0da24b2c482e395d)`()` | 
`public inline topic_alias_t `[`max`](#classasync__mqtt_1_1topic__alias__recv_1ac16c4760b2c617fadce2da370b67c6e4)`() const` | 

## Members

#### `public inline  `[`topic_alias_recv`](#classasync__mqtt_1_1topic__alias__recv_1af4afaa44f918d07493f81c2e2e36a7f1)`(topic_alias_t max)` 

#### `public inline void `[`insert_or_update`](#classasync__mqtt_1_1topic__alias__recv_1aab0f09fa093083ec07f218617fe49ea5)`(string_view topic,topic_alias_t alias)` 

#### `public inline std::string `[`find`](#classasync__mqtt_1_1topic__alias__recv_1a3be208a199fb1558b015ef897c92a788)`(topic_alias_t alias) const` 

#### `public inline void `[`clear`](#classasync__mqtt_1_1topic__alias__recv_1ab7445d987693c09f0da24b2c482e395d)`()` 

#### `public inline topic_alias_t `[`max`](#classasync__mqtt_1_1topic__alias__recv_1ac16c4760b2c617fadce2da370b67c6e4)`() const` 

# class `async_mqtt::topic_alias_send` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  `[`topic_alias_send`](#classasync__mqtt_1_1topic__alias__send_1a593168b4395f2d64776e1ec957f6e79a)`(topic_alias_t max)` | 
`public inline void `[`insert_or_update`](#classasync__mqtt_1_1topic__alias__send_1a49ece76be7ff0d8171ed95bb01ef68d0)`(string_view topic,topic_alias_t alias)` | 
`public inline std::string `[`find`](#classasync__mqtt_1_1topic__alias__send_1a2becca6b7b014718fc9e45e7bb451125)`(topic_alias_t alias)` | 
`public inline optional< topic_alias_t > `[`find`](#classasync__mqtt_1_1topic__alias__send_1a3d667fddf0f27587eafb8876e3d67fa2)`(string_view topic) const` | 
`public inline void `[`clear`](#classasync__mqtt_1_1topic__alias__send_1ad121962bccae9131888443769fcfe72b)`()` | 
`public inline topic_alias_t `[`get_lru_alias`](#classasync__mqtt_1_1topic__alias__send_1a71817e7d482d8fcc99967084cf9c7082)`() const` | 
`public inline topic_alias_t `[`max`](#classasync__mqtt_1_1topic__alias__send_1a99f98f083a60a5bfff36871ebd834d22)`() const` | 

## Members

#### `public inline  `[`topic_alias_send`](#classasync__mqtt_1_1topic__alias__send_1a593168b4395f2d64776e1ec957f6e79a)`(topic_alias_t max)` 

#### `public inline void `[`insert_or_update`](#classasync__mqtt_1_1topic__alias__send_1a49ece76be7ff0d8171ed95bb01ef68d0)`(string_view topic,topic_alias_t alias)` 

#### `public inline std::string `[`find`](#classasync__mqtt_1_1topic__alias__send_1a2becca6b7b014718fc9e45e7bb451125)`(topic_alias_t alias)` 

#### `public inline optional< topic_alias_t > `[`find`](#classasync__mqtt_1_1topic__alias__send_1a3d667fddf0f27587eafb8876e3d67fa2)`(string_view topic) const` 

#### `public inline void `[`clear`](#classasync__mqtt_1_1topic__alias__send_1ad121962bccae9131888443769fcfe72b)`()` 

#### `public inline topic_alias_t `[`get_lru_alias`](#classasync__mqtt_1_1topic__alias__send_1a71817e7d482d8fcc99967084cf9c7082)`() const` 

#### `public inline topic_alias_t `[`max`](#classasync__mqtt_1_1topic__alias__send_1a99f98f083a60a5bfff36871ebd834d22)`() const` 

# struct `async_mqtt::channel` 

```
struct async_mqtt::channel
  : public std::string
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::is_buffer_sequence` 

```
struct async_mqtt::is_buffer_sequence
  : public std::conditional::type
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::is_buffer_sequence< buffer >` 

```
struct async_mqtt::is_buffer_sequence< buffer >
  : public std::true_type
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::is_strand_template` 

```
struct async_mqtt::is_strand_template
  : public std::false_type
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::is_strand_template< as::strand< T > >` 

```
struct async_mqtt::is_strand_template< as::strand< T > >
  : public std::true_type
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::is_tls` 

```
struct async_mqtt::is_tls
  : public std::false_type
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::is_ws` 

```
struct async_mqtt::is_ws
  : public std::false_type
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::system_error` 

```
struct async_mqtt::system_error
  : public boost::system::system_error
  : private boost::totally_ordered< system_error >
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  `[`system_error`](#structasync__mqtt_1_1system__error_1a2f691933af71ba2765eccf42db1f99be)`(error_code const & ec)` | 
`public inline std::string `[`message`](#structasync__mqtt_1_1system__error_1a5d1b40582f8c5421ce2e5d49b1614ab0)`() const` | 
`public inline  `[`operator bool`](#structasync__mqtt_1_1system__error_1a483bb35b389524613c62e6e424c2e782)`() const` | 
`typedef `[`base_type`](#structasync__mqtt_1_1system__error_1a105453ec1ef156c86b742bab9b951437) | 

## Members

#### `public inline  `[`system_error`](#structasync__mqtt_1_1system__error_1a2f691933af71ba2765eccf42db1f99be)`(error_code const & ec)` 

#### `public inline std::string `[`message`](#structasync__mqtt_1_1system__error_1a5d1b40582f8c5421ce2e5d49b1614ab0)`() const` 

#### `public inline  `[`operator bool`](#structasync__mqtt_1_1system__error_1a483bb35b389524613c62e6e424c2e782)`() const` 

#### `typedef `[`base_type`](#structasync__mqtt_1_1system__error_1a105453ec1ef156c86b742bab9b951437) 

# namespace `async_mqtt::detail` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public template<>`  <br/>`char `[`buffer_sequence_begin_helper`](#buffer_8hpp_1a3739ec98a04c53938d49c9f9cb84f04d)`(...)`            | 
`public template<>`  <br/>`char(& `[`buffer_sequence_begin_helper`](#buffer_8hpp_1ac9f5084396c082bee0ab0d37bc7becd5)`(T * t,typename std::enable_if< !std::is_same< decltype(buffer_sequence_begin(*t)), void >::value >::type *)`            | 
`public template<>`  <br/>`char `[`buffer_sequence_end_helper`](#buffer_8hpp_1a153c55018d4ffff886c2f99ce9857d2f)`(...)`            | 
`public template<>`  <br/>`char(& `[`buffer_sequence_end_helper`](#buffer_8hpp_1aad23266a5784ceff4c02ae1e4a029862)`(T * t,typename std::enable_if< !std::is_same< decltype(buffer_sequence_end(*t)), void >::value >::type *)`            | 
`public template<>`  <br/>`char(& `[`buffer_sequence_element_type_helper`](#buffer_8hpp_1abde4b0d12a9311264fe212ab039d1c55)`(...)`            | 
`public template<>`  <br/>`char `[`buffer_sequence_element_type_helper`](#buffer_8hpp_1ab267ab00c83939ee6d12a0c4650967e3)`(T * t,typename std::enable_if< std::is_convertible< decltype(*buffer_sequence_begin(*t)), Buffer >::value >::type *)`            | 
`public template<>`  <br/>`inline constexpr null_log const & `[`operator<<`](#log_8hpp_1ac05d773f69099fc2cd0bc149bbcac71a)`(null_log const & o,T const &)`            | 
`struct `[`async_mqtt::detail::async_read_impl`](#structasync__mqtt_1_1detail_1_1async__read__impl) | 
`struct `[`async_mqtt::detail::is_buffer_sequence_class`](#structasync__mqtt_1_1detail_1_1is__buffer__sequence__class) | 
`struct `[`async_mqtt::detail::null_log`](#structasync__mqtt_1_1detail_1_1null__log) | 

## Members

#### `public template<>`  <br/>`char `[`buffer_sequence_begin_helper`](#buffer_8hpp_1a3739ec98a04c53938d49c9f9cb84f04d)`(...)` 

#### `public template<>`  <br/>`char(& `[`buffer_sequence_begin_helper`](#buffer_8hpp_1ac9f5084396c082bee0ab0d37bc7becd5)`(T * t,typename std::enable_if< !std::is_same< decltype(buffer_sequence_begin(*t)), void >::value >::type *)` 

#### `public template<>`  <br/>`char `[`buffer_sequence_end_helper`](#buffer_8hpp_1a153c55018d4ffff886c2f99ce9857d2f)`(...)` 

#### `public template<>`  <br/>`char(& `[`buffer_sequence_end_helper`](#buffer_8hpp_1aad23266a5784ceff4c02ae1e4a029862)`(T * t,typename std::enable_if< !std::is_same< decltype(buffer_sequence_end(*t)), void >::value >::type *)` 

#### `public template<>`  <br/>`char(& `[`buffer_sequence_element_type_helper`](#buffer_8hpp_1abde4b0d12a9311264fe212ab039d1c55)`(...)` 

#### `public template<>`  <br/>`char `[`buffer_sequence_element_type_helper`](#buffer_8hpp_1ab267ab00c83939ee6d12a0c4650967e3)`(T * t,typename std::enable_if< std::is_convertible< decltype(*buffer_sequence_begin(*t)), Buffer >::value >::type *)` 

#### `public template<>`  <br/>`inline constexpr null_log const & `[`operator<<`](#log_8hpp_1ac05d773f69099fc2cd0bc149bbcac71a)`(null_log const & o,T const &)` 

# struct `async_mqtt::detail::async_read_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public Stream & `[`stream`](#structasync__mqtt_1_1detail_1_1async__read__impl_1a001fd92a77c0e88d32a97e0ec9197c90) | 
`public MutableBufferSequence `[`mb`](#structasync__mqtt_1_1detail_1_1async__read__impl_1abbc655e4ccf8e43f12ee7ab0fa75ab23) | 
`public std::size_t `[`received`](#structasync__mqtt_1_1detail_1_1async__read__impl_1a332d566e624fd4504290e91fd25a4f00) | 
`public as::executor_work_guard< typename Stream::executor_type > `[`wg`](#structasync__mqtt_1_1detail_1_1async__read__impl_1af5e5f472d9f13d8c05ae75760e94c738) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1detail_1_1async__read__impl_1a271f6419b7c5e81c876495947d3f196b)`(Self & self,boost::system::error_code ec,std::size_t bytes_transferred)` | 

## Members

#### `public Stream & `[`stream`](#structasync__mqtt_1_1detail_1_1async__read__impl_1a001fd92a77c0e88d32a97e0ec9197c90) 

#### `public MutableBufferSequence `[`mb`](#structasync__mqtt_1_1detail_1_1async__read__impl_1abbc655e4ccf8e43f12ee7ab0fa75ab23) 

#### `public std::size_t `[`received`](#structasync__mqtt_1_1detail_1_1async__read__impl_1a332d566e624fd4504290e91fd25a4f00) 

#### `public as::executor_work_guard< typename Stream::executor_type > `[`wg`](#structasync__mqtt_1_1detail_1_1async__read__impl_1af5e5f472d9f13d8c05ae75760e94c738) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1detail_1_1async__read__impl_1a271f6419b7c5e81c876495947d3f196b)`(Self & self,boost::system::error_code ec,std::size_t bytes_transferred)` 

# struct `async_mqtt::detail::is_buffer_sequence_class` 

```
struct async_mqtt::detail::is_buffer_sequence_class
  : public std::integral_constant< bool, sizeof(buffer_sequence_begin_helper< T >(0, 0)) !=1 &&sizeof(buffer_sequence_end_helper< T >(0, 0)) !=1 &&sizeof(buffer_sequence_element_type_helper< T, Buffer >(0, 0))==1 >
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::detail::null_log` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public template<>`  <br/>`inline constexpr `[`null_log`](#structasync__mqtt_1_1detail_1_1null__log_1ae6228254b4c5cbb1449fb99dfc314f6a)`(Params && ...)` | 

## Members

#### `public template<>`  <br/>`inline constexpr `[`null_log`](#structasync__mqtt_1_1detail_1_1null__log_1ae6228254b4c5cbb1449fb99dfc314f6a)`(Params && ...)` 

# namespace `boost::asio` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline const_buffer `[`buffer`](#buffer_8hpp_1afccb94aa6be5a8e224e1b900e9522b4f)`(`[`async_mqtt::buffer`](#classasync__mqtt_1_1buffer)` const & data)`            | create boost::asio::const_buffer from the [async_mqtt::buffer](#classasync__mqtt_1_1buffer) boost::asio::const_buffer is a kind of view class. So the class doesn't hold any lifetimes. The caller needs to manage data's lifetime.

## Members

#### `public inline const_buffer `[`buffer`](#buffer_8hpp_1afccb94aa6be5a8e224e1b900e9522b4f)`(`[`async_mqtt::buffer`](#classasync__mqtt_1_1buffer)` const & data)` 

create boost::asio::const_buffer from the [async_mqtt::buffer](#classasync__mqtt_1_1buffer) boost::asio::const_buffer is a kind of view class. So the class doesn't hold any lifetimes. The caller needs to manage data's lifetime.

#### Parameters
* `data` source [async_mqtt::buffer](#classasync__mqtt_1_1buffer)

#### Returns
boost::asio::const_buffer

# namespace `std` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[`std::hash< async_mqtt::buffer >`](#classstd_1_1hash_3_01async__mqtt_1_1buffer_01_4) | 

# class `std::hash< async_mqtt::buffer >` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline std::uint64_t `[`operator()`](#classstd_1_1hash_3_01async__mqtt_1_1buffer_01_4_1a9c62100a96131fc22c1a52561d3d52cc)`(`[`async_mqtt::buffer`](#classasync__mqtt_1_1buffer)` const & v) const noexcept` | 

## Members

#### `public inline std::uint64_t `[`operator()`](#classstd_1_1hash_3_01async__mqtt_1_1buffer_01_4_1a9c62100a96131fc22c1a52561d3d52cc)`(`[`async_mqtt::buffer`](#classasync__mqtt_1_1buffer)` const & v) const noexcept` 

# struct `async_mqtt::basic_endpoint::acquire_unique_packet_id_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl_1ac096a554f7c0aaed8ed1baf677046089) | 
`public enum async_mqtt::basic_endpoint::acquire_unique_packet_id_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl_1aca706ca993b967fb708ce3dd1f6905d6) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl_1aa337a300d43d2633e437985c5e2b1883)`(Self & self)` | 
`enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl_1a77c4768c4c09d24d81c2274db34eafd8) | 

## Members

#### `public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl_1ac096a554f7c0aaed8ed1baf677046089) 

#### `public enum async_mqtt::basic_endpoint::acquire_unique_packet_id_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl_1aca706ca993b967fb708ce3dd1f6905d6) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl_1aa337a300d43d2633e437985c5e2b1883)`(Self & self)` 

#### `enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1acquire__unique__packet__id__impl_1a77c4768c4c09d24d81c2274db34eafd8) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
complete            | 

# struct `async_mqtt::basic_endpoint::close_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1close__impl_1a993cea3caea6a0be896d3ac8865e196a) | 
`public enum async_mqtt::basic_endpoint::close_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1close__impl_1a94e912490d71d336049348edd5257d89) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1close__impl_1a4a7902e1ce58d1f935fc06290313ac0e)`(Self & self,error_code const &)` | 
`enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1close__impl_1add2353694852cd84c0c80afa4f415f94) | 

## Members

#### `public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1close__impl_1a993cea3caea6a0be896d3ac8865e196a) 

#### `public enum async_mqtt::basic_endpoint::close_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1close__impl_1a94e912490d71d336049348edd5257d89) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1close__impl_1a4a7902e1ce58d1f935fc06290313ac0e)`(Self & self,error_code const &)` 

#### `enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1close__impl_1add2353694852cd84c0c80afa4f415f94) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
initiate            | 
complete            | 

# struct `async_mqtt::stream::close_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public this_type & `[`strm`](#structasync__mqtt_1_1stream_1_1close__impl_1af8123b1bcac4e1107d8b554eadc90025) | 
`public enum async_mqtt::stream::close_impl `[`state`](#structasync__mqtt_1_1stream_1_1close__impl_1acddf9e4dc702732b72be80a10a35b23c) | 
`public error_code `[`last_ec`](#structasync__mqtt_1_1stream_1_1close__impl_1a0f85f1cbf97ec2b064cd174048198f0e) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1close__impl_1a7a2f4dc555d97cd9f954fb7e1caf248f)`(Self & self)` | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1close__impl_1ad9ef45b01fc0a76a7966c5e7d620ba24)`(Self & self,error_code const & ec,std::reference_wrapper< Stream > stream)` | 
`enum `[``](#structasync__mqtt_1_1stream_1_1close__impl_1a446aa8552c64563493cb9a04e01970bb) | 

## Members

#### `public this_type & `[`strm`](#structasync__mqtt_1_1stream_1_1close__impl_1af8123b1bcac4e1107d8b554eadc90025) 

#### `public enum async_mqtt::stream::close_impl `[`state`](#structasync__mqtt_1_1stream_1_1close__impl_1acddf9e4dc702732b72be80a10a35b23c) 

#### `public error_code `[`last_ec`](#structasync__mqtt_1_1stream_1_1close__impl_1a0f85f1cbf97ec2b064cd174048198f0e) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1close__impl_1a7a2f4dc555d97cd9f954fb7e1caf248f)`(Self & self)` 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1close__impl_1ad9ef45b01fc0a76a7966c5e7d620ba24)`(Self & self,error_code const & ec,std::reference_wrapper< Stream > stream)` 

#### `enum `[``](#structasync__mqtt_1_1stream_1_1close__impl_1a446aa8552c64563493cb9a04e01970bb) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
close            | 
drop1            | 
drop2            | 
complete            | 

# struct `async_mqtt::store::elem_t` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public store_packet_t `[`packet`](#structasync__mqtt_1_1store_1_1elem__t_1a2d520267437089330c063ea78c388be8) | 
`public std::shared_ptr< as::steady_timer > `[`tim`](#structasync__mqtt_1_1store_1_1elem__t_1a28d03595ce8a5360ce7ea900af8280da) | 
`public inline  `[`elem_t`](#structasync__mqtt_1_1store_1_1elem__t_1ae81cf66f8ea6364f1fdcf1681e606665)`(store_packet_t packet,std::shared_ptr< as::steady_timer > tim)` | 
`public inline packet_id_t `[`packet_id`](#structasync__mqtt_1_1store_1_1elem__t_1a3cad923f87f0c5d01ac58458e301a988)`() const` | 
`public inline response_packet `[`response_packet_type`](#structasync__mqtt_1_1store_1_1elem__t_1a0a76e216af45f5da02273648b0f34b57)`() const` | 
`public inline void const * `[`tim_address`](#structasync__mqtt_1_1store_1_1elem__t_1a8b20a71a1899ae261596747ae0c11c3a)`() const` | 

## Members

#### `public store_packet_t `[`packet`](#structasync__mqtt_1_1store_1_1elem__t_1a2d520267437089330c063ea78c388be8) 

#### `public std::shared_ptr< as::steady_timer > `[`tim`](#structasync__mqtt_1_1store_1_1elem__t_1a28d03595ce8a5360ce7ea900af8280da) 

#### `public inline  `[`elem_t`](#structasync__mqtt_1_1store_1_1elem__t_1ae81cf66f8ea6364f1fdcf1681e606665)`(store_packet_t packet,std::shared_ptr< as::steady_timer > tim)` 

#### `public inline packet_id_t `[`packet_id`](#structasync__mqtt_1_1store_1_1elem__t_1a3cad923f87f0c5d01ac58458e301a988)`() const` 

#### `public inline response_packet `[`response_packet_type`](#structasync__mqtt_1_1store_1_1elem__t_1a0a76e216af45f5da02273648b0f34b57)`() const` 

#### `public inline void const * `[`tim_address`](#structasync__mqtt_1_1store_1_1elem__t_1a8b20a71a1899ae261596747ae0c11c3a)`() const` 

# struct `async_mqtt::topic_alias_recv::entry` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public std::string `[`topic`](#structasync__mqtt_1_1topic__alias__recv_1_1entry_1af16ce1662c9d53e16ca069015ca018ba) | 
`public topic_alias_t `[`alias`](#structasync__mqtt_1_1topic__alias__recv_1_1entry_1a422870481db4c2f0f973f3c45e52175d) | 
`public inline  `[`entry`](#structasync__mqtt_1_1topic__alias__recv_1_1entry_1a4a768f7542b4417e84a3aeaaa6142325)`(std::string topic,topic_alias_t alias)` | 

## Members

#### `public std::string `[`topic`](#structasync__mqtt_1_1topic__alias__recv_1_1entry_1af16ce1662c9d53e16ca069015ca018ba) 

#### `public topic_alias_t `[`alias`](#structasync__mqtt_1_1topic__alias__recv_1_1entry_1a422870481db4c2f0f973f3c45e52175d) 

#### `public inline  `[`entry`](#structasync__mqtt_1_1topic__alias__recv_1_1entry_1a4a768f7542b4417e84a3aeaaa6142325)`(std::string topic,topic_alias_t alias)` 

# struct `async_mqtt::topic_alias_send::entry` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public std::string `[`topic`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1aab525c8620c23149321a507bee8739e4) | 
`public topic_alias_t `[`alias`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1a27404a5941bfd8c94115fe71cfa4eb00) | 
`public time_point_t `[`tp`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1a44aa0a342145e89317bb4dd122b22882) | 
`public inline  `[`entry`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1ae169e52baed64176f97ab6d20feb0b9c)`(std::string topic,topic_alias_t alias,time_point_t tp)` | 
`public inline string_view `[`get_topic_as_view`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1afda18af322589327d996f647a4f6d950)`() const` | 

## Members

#### `public std::string `[`topic`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1aab525c8620c23149321a507bee8739e4) 

#### `public topic_alias_t `[`alias`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1a27404a5941bfd8c94115fe71cfa4eb00) 

#### `public time_point_t `[`tp`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1a44aa0a342145e89317bb4dd122b22882) 

#### `public inline  `[`entry`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1ae169e52baed64176f97ab6d20feb0b9c)`(std::string topic,topic_alias_t alias,time_point_t tp)` 

#### `public inline string_view `[`get_topic_as_view`](#structasync__mqtt_1_1topic__alias__send_1_1entry_1afda18af322589327d996f647a4f6d950)`() const` 

# struct `async_mqtt::basic_endpoint::get_stored_packets_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` const  & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl_1a7d41335f22f5be03ead325ccbb6b55f4) | 
`public enum async_mqtt::basic_endpoint::get_stored_packets_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl_1ae76c2a0cd4275d1109a35d389a390c13) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl_1a4a9168d2d071cf71979bd4a395a2e283)`(Self & self)` | 
`enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl_1a3b3beb05cbd4071316023cbeffa7d945) | 

## Members

#### `public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` const  & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl_1a7d41335f22f5be03ead325ccbb6b55f4) 

#### `public enum async_mqtt::basic_endpoint::get_stored_packets_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl_1ae76c2a0cd4275d1109a35d389a390c13) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl_1a4a9168d2d071cf71979bd4a395a2e283)`(Self & self)` 

#### `enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1get__stored__packets__impl_1a3b3beb05cbd4071316023cbeffa7d945) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
complete            | 

# struct `async_mqtt::stream::read_packet_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public this_type & `[`strm`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1aef38905cf6faceedd71b463f981a9a79) | 
`public std::size_t `[`received`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1afd8cf215428af382e82201e6f9042ea5) | 
`public std::uint32_t `[`mul`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a7cb89f8700c39acaf2e7a0ada7a8c953) | 
`public std::uint32_t `[`rl`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a8f588ede941ac03891784ebe1b629bae) | 
`public shared_ptr_array `[`spa`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a7258d1ffd46eb03517f9240d4493af42) | 
`public error_code `[`last_ec`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a8bbf940485708007174121e1d4766217) | 
`public enum async_mqtt::stream::read_packet_impl `[`state`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1ada6148a245840a35c009b88850930fbf) | 
`public inline  `[`read_packet_impl`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a78367abe7bbe0770fbc7aea6a2b257bb)`(this_type & strm)` | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a8aa36ff44c0780fd73b82108634ac161)`(Self & self)` | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1ad1594a4982080d902c0b17dc552a8be8)`(Self & self,error_code const & ec,std::size_t bytes_transferred)` | 
`enum `[``](#structasync__mqtt_1_1stream_1_1read__packet__impl_1aec0594c9460c112b4cdb42a5a5109e9d) | 

## Members

#### `public this_type & `[`strm`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1aef38905cf6faceedd71b463f981a9a79) 

#### `public std::size_t `[`received`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1afd8cf215428af382e82201e6f9042ea5) 

#### `public std::uint32_t `[`mul`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a7cb89f8700c39acaf2e7a0ada7a8c953) 

#### `public std::uint32_t `[`rl`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a8f588ede941ac03891784ebe1b629bae) 

#### `public shared_ptr_array `[`spa`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a7258d1ffd46eb03517f9240d4493af42) 

#### `public error_code `[`last_ec`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a8bbf940485708007174121e1d4766217) 

#### `public enum async_mqtt::stream::read_packet_impl `[`state`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1ada6148a245840a35c009b88850930fbf) 

#### `public inline  `[`read_packet_impl`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a78367abe7bbe0770fbc7aea6a2b257bb)`(this_type & strm)` 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1a8aa36ff44c0780fd73b82108634ac161)`(Self & self)` 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1read__packet__impl_1ad1594a4982080d902c0b17dc552a8be8)`(Self & self,error_code const & ec,std::size_t bytes_transferred)` 

#### `enum `[``](#structasync__mqtt_1_1stream_1_1read__packet__impl_1aec0594c9460c112b4cdb42a5a5109e9d) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
header            | 
remaining_length            | 
bind            | 
complete            | 

# struct `async_mqtt::basic_endpoint::recv_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a6c2874a56f75d06be312059e94eccf06) | 
`public optional< system_error > `[`decided_error`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a7e25dab0fe7a7381fcb58ae79d568b22) | 
`public enum async_mqtt::basic_endpoint::recv_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a2e5369ebe1acf0c796f64358593c529e) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a6fa0844f91f2d086714448f42cbbfea9)`(Self & self,error_code const & ec,`[`buffer`](#classasync__mqtt_1_1buffer)` buf)` | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1aa79a389e7fe12dde17de62dd8319a7e1)`(Self & self,system_error const &)` | 
`public inline void `[`send_publish_from_queue`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a2dd46a4d0d7777f4ebe2cff54b43e4d5)`()` | 
`public template<>`  <br/>`inline bool `[`process_qos2_publish`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1ae2f8318810f3d293847ba1785c9360f4)`(Self & self,protocol_version ver,`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` packet_id)` | 
`enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a1976b846e698787d845c0c493dc460df) | 

## Members

#### `public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a6c2874a56f75d06be312059e94eccf06) 

#### `public optional< system_error > `[`decided_error`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a7e25dab0fe7a7381fcb58ae79d568b22) 

#### `public enum async_mqtt::basic_endpoint::recv_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a2e5369ebe1acf0c796f64358593c529e) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a6fa0844f91f2d086714448f42cbbfea9)`(Self & self,error_code const & ec,`[`buffer`](#classasync__mqtt_1_1buffer)` buf)` 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1aa79a389e7fe12dde17de62dd8319a7e1)`(Self & self,system_error const &)` 

#### `public inline void `[`send_publish_from_queue`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a2dd46a4d0d7777f4ebe2cff54b43e4d5)`()` 

#### `public template<>`  <br/>`inline bool `[`process_qos2_publish`](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1ae2f8318810f3d293847ba1785c9360f4)`(Self & self,protocol_version ver,`[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` packet_id)` 

#### `enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1recv__impl_1a1976b846e698787d845c0c493dc460df) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
initiate            | 
disconnect            | 
close            | 
complete            | 

# struct `async_mqtt::basic_endpoint::register_packet_id_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1a3536e355979b973aaa1da54af9f5dc6f) | 
`public `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` `[`packet_id`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1a3d0e64e39c3d8342c42e7bb5e38d5f82) | 
`public enum async_mqtt::basic_endpoint::register_packet_id_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1a435575267166851977bb1d1b607eaa4c) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1aef564fc8ce2b94f6cde44d92e1f00781)`(Self & self)` | 
`enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1abdb5d9f2f7182f7043108e56a09f6061) | 

## Members

#### `public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1a3536e355979b973aaa1da54af9f5dc6f) 

#### `public `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` `[`packet_id`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1a3d0e64e39c3d8342c42e7bb5e38d5f82) 

#### `public enum async_mqtt::basic_endpoint::register_packet_id_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1a435575267166851977bb1d1b607eaa4c) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1aef564fc8ce2b94f6cde44d92e1f00781)`(Self & self)` 

#### `enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1register__packet__id__impl_1abdb5d9f2f7182f7043108e56a09f6061) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
complete            | 

# struct `async_mqtt::basic_endpoint::release_packet_id_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1ade72b1b1bdba7e80180661f575753963) | 
`public `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` `[`packet_id`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1a4bec080279365b59d47afc6a6e630094) | 
`public enum async_mqtt::basic_endpoint::release_packet_id_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1a62d5681de1c248e01ce1bbad1c17280d) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1aa7b127f91f125272fe81602dba6a4040)`(Self & self)` | 
`enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1ad2782703c7fc4ac6db58d1ac53fefa1f) | 

## Members

#### `public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1ade72b1b1bdba7e80180661f575753963) 

#### `public `[`packet_id_t`](#classasync__mqtt_1_1basic__endpoint_1a5fc1a9fe62d017b44fcc94a7fd9ab090)` `[`packet_id`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1a4bec080279365b59d47afc6a6e630094) 

#### `public enum async_mqtt::basic_endpoint::release_packet_id_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1a62d5681de1c248e01ce1bbad1c17280d) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1aa7b127f91f125272fe81602dba6a4040)`(Self & self)` 

#### `enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1release__packet__id__impl_1ad2782703c7fc4ac6db58d1ac53fefa1f) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
complete            | 

# struct `async_mqtt::basic_endpoint::restore_packets_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1a35aff96ba197a92a21ed1358f901076a) | 
`public std::vector< basic_store_packet_variant< PacketIdBytes > > `[`pvs`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1aafa57d26dd00dedf4ab2496bcd517f48) | 
`public enum async_mqtt::basic_endpoint::restore_packets_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1a67a5f48ed0dfe8ffac79f294a97018ee) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1a1efd0b4c21d3885d14207a9d75400678)`(Self & self)` | 
`enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1a10430a6479e7fd2f8fe8fc3f37403095) | 

## Members

#### `public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1a35aff96ba197a92a21ed1358f901076a) 

#### `public std::vector< basic_store_packet_variant< PacketIdBytes > > `[`pvs`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1aafa57d26dd00dedf4ab2496bcd517f48) 

#### `public enum async_mqtt::basic_endpoint::restore_packets_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1a67a5f48ed0dfe8ffac79f294a97018ee) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1a1efd0b4c21d3885d14207a9d75400678)`(Self & self)` 

#### `enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1restore__packets__impl_1a10430a6479e7fd2f8fe8fc3f37403095) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
complete            | 

# struct `async_mqtt::basic_endpoint::send_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1aaa11865128a053f2ff84f42b1355c4af) | 
`public Packet `[`packet`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a2bbeed9e74320315c131670d5ed662ae) | 
`public bool `[`from_queue`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1aca4e2c0e817cab7cc842580d597bd3cc) | 
`public enum async_mqtt::basic_endpoint::send_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a04cca1668db0fab154190863433fd64e) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1afd11ede8bfc38fdda490700360ce9a25)`(Self & self,error_code const & ec,std::size_t)` | 
`public template<>`  <br/>`inline bool `[`process_send_packet`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a23fcb1d45c419495c9940156d0a155a1)`(Self & self,ActualPacket & actual_packet)` | 
`public template<>`  <br/>`inline bool `[`validate_topic_alias_range`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a8a1d487598a0fadb33e529ad25b71784)`(Self & self,topic_alias_t ta)` | 
`public template<>`  <br/>`inline std::optional< std::string > `[`validate_topic_alias`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1adbaef3ecc76a3a9d8bd5ec65bc0648a0)`(Self & self,optional< topic_alias_t > ta_opt)` | 
`public template<>`  <br/>`inline bool `[`validate_maximum_packet_size`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a7d5de7002350960dfe19b57a7ba9bd4f)`(Self & self,PacketArg const & packet_arg)` | 
`enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1aa809b0726925e40abbb7cd966bd78a93) | 

## Members

#### `public `[`this_type`](#classasync__mqtt_1_1basic__endpoint)` & `[`ep`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1aaa11865128a053f2ff84f42b1355c4af) 

#### `public Packet `[`packet`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a2bbeed9e74320315c131670d5ed662ae) 

#### `public bool `[`from_queue`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1aca4e2c0e817cab7cc842580d597bd3cc) 

#### `public enum async_mqtt::basic_endpoint::send_impl `[`state`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a04cca1668db0fab154190863433fd64e) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1afd11ede8bfc38fdda490700360ce9a25)`(Self & self,error_code const & ec,std::size_t)` 

#### `public template<>`  <br/>`inline bool `[`process_send_packet`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a23fcb1d45c419495c9940156d0a155a1)`(Self & self,ActualPacket & actual_packet)` 

#### `public template<>`  <br/>`inline bool `[`validate_topic_alias_range`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a8a1d487598a0fadb33e529ad25b71784)`(Self & self,topic_alias_t ta)` 

#### `public template<>`  <br/>`inline std::optional< std::string > `[`validate_topic_alias`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1adbaef3ecc76a3a9d8bd5ec65bc0648a0)`(Self & self,optional< topic_alias_t > ta_opt)` 

#### `public template<>`  <br/>`inline bool `[`validate_maximum_packet_size`](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1a7d5de7002350960dfe19b57a7ba9bd4f)`(Self & self,PacketArg const & packet_arg)` 

#### `enum `[``](#structasync__mqtt_1_1basic__endpoint_1_1send__impl_1aa809b0726925e40abbb7cd966bd78a93) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
write            | 
complete            | 

# struct `async_mqtt::topic_alias_send::tag_alias` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::store::tag_res_id` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::store::tag_seq` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::store::tag_tim` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::topic_alias_send::tag_topic_name` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::topic_alias_send::tag_tp` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `async_mqtt::stream::write_packet_impl` 

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public this_type & `[`strm`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1ad059238e427d619d0332c1bc53300968) | 
`public std::shared_ptr< Packet > `[`packet`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1a1837cd76b889847497dc26129775f4bd) | 
`public error_code `[`last_ec`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1a63a2b3992b593f396ebf414e3d79a489) | 
`public this_type_sp `[`life_keeper`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1af1fd35b6c80139106f35ccf795111808) | 
`public optional< as::executor_work_guard< as::io_context::executor_type > > `[`queue_work_guard`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1ae32305f436051d8ec2c4ccd6ae89fd83) | 
`public enum async_mqtt::stream::write_packet_impl `[`state`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1ae660d12da347d3ebceee38c3ffeeff4e) | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1ab6d8e2f8a7e03ff48f2deed1f02b704e)`(Self & self)` | 
`public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1a90efca08a3e9ec315795cfb1c35e435e)`(Self & self,error_code const & ec,std::size_t bytes_transferred)` | 
`enum `[``](#structasync__mqtt_1_1stream_1_1write__packet__impl_1a46e2939157fbafcaaf50ae63d509e62c) | 

## Members

#### `public this_type & `[`strm`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1ad059238e427d619d0332c1bc53300968) 

#### `public std::shared_ptr< Packet > `[`packet`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1a1837cd76b889847497dc26129775f4bd) 

#### `public error_code `[`last_ec`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1a63a2b3992b593f396ebf414e3d79a489) 

#### `public this_type_sp `[`life_keeper`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1af1fd35b6c80139106f35ccf795111808) 

#### `public optional< as::executor_work_guard< as::io_context::executor_type > > `[`queue_work_guard`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1ae32305f436051d8ec2c4ccd6ae89fd83) 

#### `public enum async_mqtt::stream::write_packet_impl `[`state`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1ae660d12da347d3ebceee38c3ffeeff4e) 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1ab6d8e2f8a7e03ff48f2deed1f02b704e)`(Self & self)` 

#### `public template<>`  <br/>`inline void `[`operator()`](#structasync__mqtt_1_1stream_1_1write__packet__impl_1a90efca08a3e9ec315795cfb1c35e435e)`(Self & self,error_code const & ec,std::size_t bytes_transferred)` 

#### `enum `[``](#structasync__mqtt_1_1stream_1_1write__packet__impl_1a46e2939157fbafcaaf50ae63d509e62c) 

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
dispatch            | 
post            | 
write            | 
bind            | 
complete            | 

Generated by [Moxygen](https://sourcey.com/moxygen)