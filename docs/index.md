## Usage

First, you'll need to obtain an endpoint from <https://pubsub.ing/endpoints.json>.

### Subscriber

Establish a WebSocket connection to the endpoint with path "/sub". Send a message starting with `+`, followed by the topic. For example, `+test` to subscribe to topic `test`.

You can use the `-` prefix to remove a subscription.

### Publisher

Send a HTTP request to the endpoint with path "/pub" and a "topic" query argument. For example, `curl notls.pubsub.ing/pub?topic=test --data "Hello"` to send "Hello" to every subscriber of `test`.
