# CAPI

`CAPI` provides lightweight C++ RAII utilities for resources acquired from C APIs.

`unique_id` stores a plain C handle (for example: `int`, `FILE*`, or platform
handle types) and automatically invokes a cleanup function in its destructor.

`unique_res` stores a raw pointer and a release function, then automatically calls
the release function when the wrapper goes out of scope.

## Include

```cpp
#include <capi/unique_id.hpp>
#include <capi/unique_res.hpp>
// or
#include <capi>
```

## Example: wrap a C API ID with `unique_id`

```cpp
extern "C" {
	int c_socket_open(const char* endpoint);
	void c_socket_close(int socket_id);
	int c_socket_send(int socket_id, const char* payload);
}

#include <capi/unique_id.hpp>

using socket_id = capi::unique_id<int, &c_socket_close>;

socket_id open_socket(const char* endpoint) {
	return socket_id { c_socket_open(endpoint) };
}

int send_ping() {
	auto socket = open_socket("127.0.0.1:9000");

	if (!static_cast<bool>(socket)) {
		return -1;
	}

	return c_socket_send(static_cast<int>(socket), "ping");
} // c_socket_close is called automatically here (unless id == 0)
```

## Example: inherit from `unique_id`

```cpp
extern "C" {
	int c_channel_open();
	void c_channel_close(int channel_id);
	int c_channel_publish(int channel_id, const char* msg);
}

#include <capi/unique_id.hpp>

struct channel : capi::unique_id<int, &c_channel_close> {
	using capi::unique_id<int, &c_channel_close>::unique_id;

	static channel open() {
		return channel { c_channel_open() };
	}

	int publish(const char* msg) const {
		auto raw = static_cast<int>(*this);
		return raw == 0 ? -1 : c_channel_publish(raw, msg);
	}
};

int publish_ping() {
	auto ch = channel::open();
	return ch.publish("ping");
} // c_channel_close is called automatically here (unless id == 0)
```

## Example: wrap a C API handle

```cpp
extern "C" {
	struct c_client;

	c_client* c_client_open(const char* endpoint);
	void c_client_close(c_client* client);
	int c_client_send(c_client* client, const char* payload);
}

#include <capi/unique_res.hpp>

using client_handle = capi::unique_res<c_client>;

client_handle open_client(const char* endpoint) {
	return client_handle { c_client_open(endpoint), &c_client_close };
}

int send_ping() {
	auto client = open_client("127.0.0.1:9000");

	if (!static_cast<bool>(client)) {
		return -1;
	}

	return c_client_send(static_cast<c_client*>(client), "ping");
} // c_client_close is called automatically here
```

## Example: inherit from `unique_res`

```cpp
extern "C" {
	struct c_client;

	c_client* c_client_open(const char* endpoint);
	void c_client_close(c_client* client);
	int c_client_send(c_client* client, const char* payload);
}

#include <capi/unique_res.hpp>

struct client : capi::unique_res<c_client> {
	using capi::unique_res<c_client>::unique_res;

	static client open(const char* endpoint) {
		return client { c_client_open(endpoint), &c_client_close };
	}

	int send(const char* payload) const {
		auto* raw = static_cast<c_client*>(*this);
		return raw == nullptr ? -1 : c_client_send(raw, payload);
	}
};

int send_ping_v2() {
	auto client_handle = client::open("127.0.0.1:9000");
	return client_handle.send("ping");
} // c_client_close is called automatically here
```

## Notes

- `unique_id` is non-copyable and non-movable.
- A zero/default-initialized ID is valid and evaluates to `false`.
- Access the underlying ID with `static_cast<T>(id_handle)`.
- `unique_res` is move-only (copy operations are deleted).
- A `nullptr` resource is valid and evaluates to `false`.
- Access the underlying pointer with `static_cast<T*>(handle)`.

## CMake

Use the `CAPI::capi` CMake target when linking CAPI.

```cmake
add_subdirectory(path/to/capi)
target_link_libraries(your_target PRIVATE CAPI::capi)
```
