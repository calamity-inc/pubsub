## Usage

First, you'll need to obtain an endpoint from <https://pubsub.cat/endpoints.json>.

### Subscriber

Establish a WebSocket connection to the endpoint with path "/sub". Send a message starting with `+`, followed by the topic. For example, `+test` to subscribe to topic `test`.

When a message is published, a frame is sent, containing the topic and message, joined by a colon. For example, "Hello" sent to topic `test` would be a frame containing `test:Hello`.

You can use the `-` prefix to remove a subscription.

### Publisher

Send a HTTP request to the endpoint with path "/pub" and a "topic" query argument. For example, `curl notls.pubsub.cat/pub?topic=test --data "Hello"` to send "Hello" to every subscriber of `test`.
