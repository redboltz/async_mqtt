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
