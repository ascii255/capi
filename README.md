# CAPI

`CAPI` provides lightweight C++ RAII utilities for resources acquired from C APIs.

`unique_res` stores a raw pointer and a release function, then automatically calls
the release function when the wrapper goes out of scope.

## Include

```cpp
#include <capi/unique_res.hpp>
// or
#include <capi>
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

- `unique_res` is move-only (copy operations are deleted).
- A `nullptr` resource is valid and evaluates to `false`.
- Access the underlying pointer with `static_cast<T*>(handle)`.

## CMake

Use the `CAPI::capi` CMake target when linking CAPI.

```cmake
add_subdirectory(path/to/capi)
target_link_libraries(your_target PRIVATE CAPI::capi)
```
