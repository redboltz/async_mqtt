## 5.0.0
### breaking changes
- Removed core sub directory and move file to upper directory. #158
- Added null strand support. #153
### other updates
- Removed redundant locks from internal queue. #157
- Added tests. #154, #156
- Refined CI. #155

## 4.1.0
- Re-designed unique_scope_guard. #146, #148, #149
- Fixed moved from object access. #144
- Removed code repeat. #140
- Added acquire_unique_packet_id_wait_until(). #138, #139, #141, #142, #151
- Relaxed epsp_wrap constructor for broker. #137
- Supported no matching subscribers reason code for broker. #133
- Added all.hpp generator. #131
- Refined client_cli. #130
- Added print payload option. #129
- Added keep_alive settiong to bench. #125
- Fixed num_of_const_buffer_sequence. #120, #121
- Refined tests. #120, #122, #123, #127, #128, #132, #134, #136, #145
- Refined packet comparison. #119
- Replaced return type with auto. #110
- Added UTF-8 checking. #107
- Replaced callback with CompletionToken on broker. #106
- Refined C++20 couroutune example. #105
- Used any_io_executor as the base of predefined mqtt protocol. #104
- Refined documentation. #103


## 4.0.0
### breaking changes
- Fixed multiple close problem. In order to do that endpoint become shared_ptr based design. #98, #100, #101, #102
### other updates
- Refined documents. #97
- Added TLS async_shutdown timeout. #99

## 3.0.0
### breaking changes
- Fixed inconsistent function names. #84, #89
  - get_stored() => get_stored_packets()
  - set_ping_resp_recv_timeout_ms() => set_pingresp_recv_timeout_ms()
### other updates
- Improved buffer implementation to support various compilers. #87
- Improved packet_id management. #85
- Fixed packet_id length checking. #20

## 2.0.0
### breaking changes
#### endpoint
- Made endpoint non movable. #79.
  - It is designed non copyable and non movable but the code was able to movable invalidly,
    so this is a bug fix. However some of test, broker, and bench code had been used move constructor.
    Hence I categolize the fix to breaking changes.
#### broker
- Added enable_shared_from_this to session_state. #67, #68
### other updates
- Added to_buffer function for std::vector<buffer>. #77
- Refined CI. #75
- Fixed invalid sendable packet checking. #74
- Added fixed CPU core map by ioc for broker. #69, #70
- Fixed endpoint's internal queue operation. #66
- Refined documents. #62
- Refined examples. #61
- Refined bench. #60, #63, #64, #65

## 1.0.9
- Removed debung print. #59

## 1.0.8
- Fixed invalid async_write queue operation. #57
- Improved bench tool. #53, #54, #56

## 1.0.7
- Removed zlib dependency. #51
- Refined topic alias. #48
- Refined broker's CA certificate checking. #45, #46
- Fixed recv() with filter compile error. #44

## 1.0.6
- Fixed docker launch bash scripts. #40
- Refined docker images. #39

## 1.0.5
- Fixed missing PINGRESP timeout cancel. #37
- Refined CI. #33

## 1.0.4
- Fixed deliver authorization for broker. #30
- Refined client_cli. #29
- Fixed creating packets from buffer process. #28

## 1.0.3
- Fixed receive packet error processing.  #28
- Fixed multiple definition linker error. #25, #26

## 1.0.2
- Fixed installed cmake configuration. #23
- Fixed offline client inheritance on broker. #22
- Fixed PUBREL(v5) reason_code on broker. #21
- Moved SHA256 from OpenSSL to picosha2. #19
- Refined tests. #19, #20

## 1.0.1
- Fixed broker's PUBREL rc. #12
- Removed redundant codes. #15
- Added CLI MQTT client. #12
- Added docker support. #11, #13, #14

## 1.0.0
- Initial release.
