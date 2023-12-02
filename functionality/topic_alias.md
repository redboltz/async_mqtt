# Topic Alias
Topic Alias is a way to reduce PUBLISH packet size.

## Notifying capacity

The client can set `Topic Alias Maximum` property that value is greater than 0 to the CONNECT packet. This means the client can receive the PUBLISH packet with `Topic Alias` property that the value is less than or equal to `Topic Alias Maximum`. The broker could send the PUBLISH packet using `Topic Alias` property.

The broker can set `Topic Alias Maximum` property that value is greater than 0 to the CONNACK packet. This means the broker can receive the PUBLISH packet with `Topic Alias` property that the value is less than or equal to `Topic Alias Maximum`. The client could send the PUBLISH packet using `Topic Alias` property.

These two `Topic Alias Maximum` is independent.
