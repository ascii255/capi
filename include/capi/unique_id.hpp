#pragma once

static_assert(__cplusplus >= 202302L, "capi requires C++23");

#include <concepts>
#include <type_traits>
#include <utility>

namespace capi::inline v1_0_5 {

template <typename T, auto Open, auto Close, T Uninitialized = T {}>
  requires std::invocable<decltype(Close), T>
struct unique_id {
protected:
  const T id;

public:
  const bool initialized;
  template <typename... Args>
    requires std::is_invocable_r_v<bool, decltype(Open), T, Args...>
  constexpr explicit unique_id(T raw_id, Args&&... args) noexcept(noexcept(Open(raw_id, std::forward<Args>(args)...)))
      : id { raw_id }, initialized { Open(raw_id, std::forward<Args>(args)...) } {}
  constexpr ~unique_id() noexcept(noexcept(Close(std::declval<T>()))) {
    if (id != Uninitialized) Close(id);
  }
  constexpr unique_id(const unique_id&) = delete;
  constexpr unique_id& operator=(const unique_id&) = delete;
  constexpr unique_id(unique_id&& other) = delete;
  constexpr unique_id& operator=(unique_id&& other) = delete;

  constexpr explicit operator T() const noexcept { return id; }
};

} // namespace capi::inline v1_0_5

//
//
//

#ifdef CAPI_TESTING

#include <expect/expect.hpp>

namespace capi::testing {

constexpr bool tracking_id_opener(int* id) noexcept { return id != nullptr; }
constexpr void tracking_id_closer(int* id) noexcept { ++(*id); }

using tracking_id = unique_id<int*, tracking_id_opener, tracking_id_closer>;

constexpr void simulated_c_api_id_lifecycle() {
  int close_count = 0;
  {
    tracking_id handle { &close_count };
    expect(handle.initialized);
    expect(static_cast<int*>(handle) == &close_count);
    expect(close_count == 0);
  }
  expect(close_count == 1);
}

constexpr void zero_id_skips_closer() {
  int close_count = 0;
  {
    tracking_id handle { nullptr };
    expect(!handle.initialized);
    expect(static_cast<int*>(handle) == nullptr);
  }
  expect(close_count == 0);
}

constexpr void run_unique_id_tests() {
  expect(!std::is_move_constructible_v<tracking_id>);
  expect(!std::is_move_assignable_v<tracking_id>);
  expect(!std::is_copy_constructible_v<tracking_id>);
  expect(!std::is_copy_assignable_v<tracking_id>);

  zero_id_skips_closer();
  simulated_c_api_id_lifecycle();
}

static_assert([] {
  run_unique_id_tests();
  return true;
}());

} // namespace capi::testing

#endif // CAPI_TESTING
