#pragma once

#include <concepts>

namespace capi::inline v1_0_1 {

template <typename T, auto C>
  requires std::invocable<decltype(C), T>
struct unique_id {
protected:
  T id = T {};

public:
  constexpr explicit unique_id(T id) noexcept : id(id) {}

  constexpr ~unique_id() noexcept {
    if (id) C(id);
  }

  unique_id(const unique_id&) = delete;
  unique_id& operator=(const unique_id&) = delete;
  unique_id(unique_id&& other) = delete;
  unique_id& operator=(unique_id&& other) = delete;

  constexpr explicit operator T() const noexcept { return id; }
  constexpr explicit operator bool() const noexcept { return id != T {}; }
};

} // namespace capi::inline v1_0_1

#ifdef CAPI_TESTING

#include <expect>
#include <type_traits>

namespace capi::testing {

constexpr void tracking_id_deleter(int* id) noexcept { ++(*id); }

constexpr void simulated_c_api_id_lifecycle() {
  int delete_count = 0;
  {
    unique_id<int*, tracking_id_deleter> handle {&delete_count};
    expect(static_cast<bool>(handle));
    expect(static_cast<int*>(handle) == &delete_count);
    expect(delete_count == 0);
  }
  expect(delete_count == 1);
}

constexpr void zero_id_skips_deleter() {
  int delete_count = 0;
  {
    unique_id<int*, tracking_id_deleter> handle {nullptr};
    expect(!static_cast<bool>(handle));
    expect(static_cast<int*>(handle) == nullptr);
  }
  expect(delete_count == 0);
}

constexpr void run_unique_id_tests() {
  expect(!std::is_move_constructible_v<unique_id<int*, tracking_id_deleter>>);
  expect(!std::is_move_assignable_v<unique_id<int*, tracking_id_deleter>>);
  expect(!std::is_copy_constructible_v<unique_id<int*, tracking_id_deleter>>);
  expect(!std::is_copy_assignable_v<unique_id<int*, tracking_id_deleter>>);

  zero_id_skips_deleter();
  simulated_c_api_id_lifecycle();
}

#ifdef CAPI_STATIC_TESTING
static_assert([] {
  run_unique_id_tests();
  return true;
}());
#endif

} // namespace capi::testing

#endif
