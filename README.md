# CAPI /[ˈseɪpaɪ](https://ipa-reader.com/?text=%CB%88se%C9%AApa%C9%AA&voice=Matthew)/

[![CI](https://github.com/ascii255/capi/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ascii255/capi/actions/workflows/ci.yml) [![Latest Tag](https://img.shields.io/github/v/tag/ascii255/capi?sort=semver)](https://github.com/ascii255/capi/tags) [![License](https://img.shields.io/badge/license-Unlicense-blue)](https://github.com/ascii255/capi/blob/main/UNLICENSE)

`CAPI` provides lightweight C++ RAII utilities for resources acquired from C APIs.

`unique_id` stores a plain C handle (for example: `int`, `FILE*`, or platform
handle types), runs an `Open` step at construction, and invokes `Close` in its
destructor.

`unique_res` acquires a pointer through `Create(args...)` and automatically
releases it with `Destroy(ptr)` when the wrapper goes out of scope.

`flag` utilities in `flag.hpp` provide typed bitwise operators for opted-in
unsigned enums and `flag_value(value)`, which returns `std::to_underlying(value)`
for `capi::flag` enums and the original value otherwise.

`unique_flgd` manages a single C-style bit flag: it checks whether the flag is
already active, initializes it when needed, and clears it in the destructor.

`unique_sys` manages process-wide C API lifecycle setup/teardown pairs, calling
`Init()` on construction and `Quit()` on destruction when initialization
succeeds.

## Include

```cpp
#include <capi/flag.hpp>
#include <capi/unique_flgd.hpp>
#include <capi/unique_id.hpp>
#include <capi/unique_res.hpp>
#include <capi/unique_sys.hpp>
// or
#include <capi>
```

## Example: wrap a C API ID with `unique_id`

```cpp
extern "C" {
	int c_socket_open(int socket_id, const char* endpoint) noexcept;
	void c_socket_close(int socket_id) noexcept;
	int c_socket_send(int socket_id, const char* payload) noexcept;
}

#include <capi/unique_id.hpp>

using socket_id = capi::unique_id<int, c_socket_open, c_socket_close>;

socket_id open_socket(const char* endpoint) {
	return socket_id { 1, endpoint };
}

int send_ping() {
	auto socket = open_socket("127.0.0.1:9000");

	if (!static_cast<bool>(socket)) {
		return -1;
	}

	return c_socket_send(static_cast<int>(socket), "ping");
} // c_socket_close is called automatically here (unless id is uninitialized)
```

## Example: inherit from `unique_id`

```cpp
extern "C" {
	int c_channel_open(int channel_id) noexcept;
	void c_channel_close(int channel_id) noexcept;
	int c_channel_publish(int channel_id, const char* msg) noexcept;
}

#include <capi/unique_id.hpp>

struct channel : capi::unique_id<int, c_channel_open, c_channel_close> {
	using capi::unique_id<int, c_channel_open, c_channel_close>::unique_id;

	int publish(const char* msg) const {
		if (!static_cast<bool>(*this)) {
			return -1;
		}

		return c_channel_publish(static_cast<int>(*this), msg);
	}
};

int publish_ping() {
	auto ch = channel { 1 };
	return ch.publish("ping");
} // c_channel_close is called automatically here (unless id is uninitialized)
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

using client_handle = capi::unique_res<c_client, c_client_open, c_client_close>;

client_handle open_client(const char* endpoint) {
	return client_handle { endpoint };
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

struct client : capi::unique_res<c_client, c_client_open, c_client_close> {
	using capi::unique_res<c_client, c_client_open, c_client_close>::unique_res;

	static client open(const char* endpoint) {
		return client { endpoint };
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

## Example: wrap a C API capability flag with `unique_flgd`

```cpp
extern "C" {
	using c_flags_t = unsigned;

	inline constexpr c_flags_t C_FLAG_MONITOR = 0x1U;

	bool c_flag_enable(c_flags_t flag) noexcept;
	void c_flag_disable(c_flags_t flag) noexcept;
	c_flags_t c_flag_query(c_flags_t mask) noexcept;
}

#include <capi/unique_flgd.hpp>

using monitor_flag =
	capi::unique_flgd<C_FLAG_MONITOR, c_flag_enable, c_flag_disable, c_flag_query>;

int run_with_monitoring() {
	monitor_flag monitor {};

	if (!static_cast<bool>(monitor)) {
		return -1;
	}

	return 0;
} // c_flag_disable(C_FLAG_MONITOR) is called automatically if acquired
```

## Example: enable `flag.hpp` operators for an `enum class`

Opt in your enum with an `enable_flag_enum` declaration in the same namespace
as the enum, then use bitwise operators and `capi::flag_value(...)`:

```cpp
#include <capi/flag.hpp>

enum class app_flag : unsigned {
	none = 0U,
	monitor = 1U << 0U,
	trace = 1U << 1U,
};

capi::flag_enum_tag enable_flag_enum(app_flag);

static_assert(capi::flag<app_flag>);
static_assert(std::is_same_v<decltype(capi::flag_value(app_flag::monitor)), unsigned>);

constexpr app_flag combined = app_flag::monitor | app_flag::trace;
static_assert(capi::flag_value(combined) ==
	(static_cast<unsigned>(app_flag::monitor) | static_cast<unsigned>(app_flag::trace)));
```

## Example: wrap global C API init/shutdown with `unique_sys`

```cpp
extern "C" {
	bool c_runtime_init() noexcept;
	void c_runtime_shutdown() noexcept;
}

#include <capi/unique_sys.hpp>

using runtime_guard = capi::unique_sys<c_runtime_init, c_runtime_shutdown>;

int run_program() {
	runtime_guard runtime {};

	if (!static_cast<bool>(runtime)) {
		return -1;
	}

	return 0;
} // c_runtime_shutdown() is called automatically if init succeeded
```

## Notes

- `unique_id` is move-only (copy operations are deleted).
- `unique_id` is defined as `capi::unique_id<T, Open, Close, Uninitialized = T {}>`.
- `Open(raw_id, args...)` must return `T`. `Close(id)` only needs to be invocable.
- By default, `T {}` is the uninitialized ID value and evaluates to `false`.
- Access the underlying ID with `static_cast<T>(id_handle)`.
- `unique_res` is move-only (copy operations are deleted).
- `unique_res` is defined as `capi::unique_res<T, Create, Destroy>`.
- `Create(args...)` must return `T*`.
- A `nullptr` result from `Create(...)` is valid and evaluates to `false`.
- Access the underlying pointer with `static_cast<T*>(handle)`.
- `flag.hpp` defines `capi::flag_enum`, `capi::flag`, bitwise operators for `capi::flag`, and `capi::flag_value(value)`.
- Opt an enum into `capi::flag_enum` by declaring `capi::flag_enum_tag enable_flag_enum(your_enum_type);`.
- `unique_flgd` is move-only (copy operations are deleted).
- `unique_flgd` is defined as `capi::unique_flgd<Value, Init, Quit, Query, Underlying = capi::flag_value(Value), T = decltype(Underlying)>`.
- `Init(flag)` must return `bool`; `Quit(flag)` must be invocable.
- `Query(mask)` must return `T`; `operator bool()` checks `Query(flag) & flag` and expects that expression to be convertible to `bool`.
- `unique_sys` is non-copyable and non-movable.
- `unique_sys` is defined as `capi::unique_sys<Init, Quit>`.
- `Init()` must return `bool`; `Quit()` must be invocable.
- `Quit()` is called only when initialization succeeded.

## CMake

Use the `CAPI::capi` CMake target when linking CAPI.

```cmake
add_subdirectory(path/to/capi)
target_link_libraries(your_target PRIVATE CAPI::capi)
```
