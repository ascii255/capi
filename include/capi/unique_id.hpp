#pragma once

static_assert(__cplusplus >= 202302L, "capi requires C++23");

#include <concepts>
#include <type_traits>
#include <utility>

namespace capi::inline v1_0_4 {

template <typename T, auto Open, auto Close, T Uninitialized = T {}>
  requires std::invocable<decltype(Close), T>
struct unique_id {
protected:
  T id = Uninitialized;

public:
  template <typename... Args>
    requires std::is_invocable_r_v<T, decltype(Open), T, Args...>
  constexpr explicit unique_id(T raw_id, Args&&... args) noexcept(noexcept(Open(raw_id, std::forward<Args>(args)...)))
      : id { Open(raw_id, std::forward<Args>(args)...) } {}
  constexpr ~unique_id() noexcept(noexcept(Close(std::declval<T>()))) {
    if (id != Uninitialized) Close(id);
  }
  constexpr unique_id(const unique_id&) = delete;
  constexpr unique_id& operator=(const unique_id&) = delete;
  constexpr unique_id(unique_id&& other) noexcept : id { std::exchange(other.id, Uninitialized) } {}
  constexpr unique_id& operator=(unique_id&& other) noexcept {
    std::swap(id, other.id);
    return *this;
  }

  constexpr explicit operator T() const noexcept { return id; }
  constexpr explicit operator bool() const noexcept { return id != Uninitialized; }
};

} // namespace capi::inline v1_0_4

//
//
//

#ifdef CAPI_TESTING

#include <expect/expect.hpp>

namespace capi::testing {

constexpr int* tracking_id_opener(int* id) noexcept { return id; }
constexpr void tracking_id_closer(int* id) noexcept { ++(*id); }

using tracking_id = unique_id<int*, tracking_id_opener, tracking_id_closer>;

constexpr void simulated_c_api_id_lifecycle() {
  int close_count = 0;
  {
    tracking_id handle { &close_count };
    expect(static_cast<bool>(handle));
    expect(static_cast<int*>(handle) == &close_count);
    expect(close_count == 0);
  }
  expect(close_count == 1);
}

constexpr void zero_id_skips_closer() {
  int close_count = 0;
  {
    tracking_id handle { nullptr };
    expect(!static_cast<bool>(handle));
    expect(static_cast<int*>(handle) == nullptr);
  }
  expect(close_count == 0);
}

constexpr void move_constructor_transfers_id() {
  int close_count = 0;
  {
    tracking_id source { &close_count };
    tracking_id target { std::move(source) };
    expect(static_cast<int*>(target) == &close_count);
    expect(!static_cast<bool>(source));
    expect(close_count == 0);
  }
  expect(close_count == 1);
}

constexpr void move_assignment_transfers_id() {
  int close_count = 0;
  {
    tracking_id source { &close_count };
    tracking_id target { nullptr };
    target = std::move(source);
    expect(static_cast<int*>(target) == &close_count);
    expect(!static_cast<bool>(source));
    expect(close_count == 0);
  }
  expect(close_count == 1);
}

constexpr void run_unique_id_tests() {
  expect(std::is_move_constructible_v<tracking_id>);
  expect(std::is_move_assignable_v<tracking_id>);
  expect(!std::is_copy_constructible_v<tracking_id>);
  expect(!std::is_copy_assignable_v<tracking_id>);

  zero_id_skips_closer();
  move_constructor_transfers_id();
  move_assignment_transfers_id();
  simulated_c_api_id_lifecycle();
}

static_assert([] {
  run_unique_id_tests();
  return true;
}());

} // namespace capi::testing

#endif // CAPI_TESTING
