#pragma once

static_assert(__cplusplus >= 202302L, "capi requires C++23");

#include <memory>
#include <utility>

namespace capi::inline v1_0_5 {

template <typename T, auto Create, auto Destroy> struct unique_res {
  template <typename... Args>
    requires std::is_invocable_r_v<T*, decltype(Create), Args...>
  constexpr explicit unique_res(Args&&... args) noexcept(noexcept(Create(std::forward<Args>(args)...)))
    : resource { Create(std::forward<Args>(args)...), Destroy } {}
  unique_res(const unique_res&) = delete;
  unique_res& operator=(const unique_res&) = delete;
  constexpr unique_res(unique_res&&) noexcept = default;
  constexpr unique_res& operator=(unique_res&&) noexcept = default;

  constexpr explicit operator T*() const noexcept { return resource.get(); }
  constexpr explicit operator bool() const noexcept { return resource != nullptr; }

protected:
  std::unique_ptr<T, decltype(Destroy)> resource;
};

} // namespace capi::inline v1_0_5

//
//
//

#ifdef CAPI_TESTING

#include <type_traits>

#include <expect/expect.hpp>

namespace capi::testing {

constexpr int* c_api_acquire_int(int* value) noexcept { return value; }
constexpr void no_op_int_deleter(int*) {}

struct fake_c_api_resource {
  int* release_count;
  bool* released;
};

constexpr fake_c_api_resource* c_api_acquire(fake_c_api_resource* resource) noexcept { return resource; }

constexpr void c_api_release(fake_c_api_resource* resource) noexcept {
  if (resource == nullptr) {
    return;
  }

  ++(*resource->release_count);
  *resource->released = true;
}

using tracked_resource = unique_res<fake_c_api_resource, c_api_acquire, c_api_release>;
using tracked_int = unique_res<int, c_api_acquire_int, no_op_int_deleter>;

constexpr void simulated_c_api_resource_lifecycle() {
  int release_count = 0;
  bool released = false;
  fake_c_api_resource resource { &release_count, &released };

  {
    tracked_resource handle { &resource };
    expect(static_cast<bool>(handle));
    expect(static_cast<fake_c_api_resource*>(handle) == &resource);
    expect(release_count == 0);
    expect(!released);
  }

  expect(release_count == 1);
  expect(released);
}

constexpr void empty_resource_behaves_as_expected() {
  tracked_int resource { nullptr };
  expect(!static_cast<bool>(resource));
  expect(static_cast<int*>(resource) == nullptr);
}

constexpr void move_constructor_transfers_resource() {
  int value = 0;
  tracked_int source { &value };
  tracked_int target { std::move(source) };
  expect(static_cast<int*>(target) == &value);
  expect(!static_cast<bool>(source));
}

constexpr void move_assignment_transfers_resource() {
  int value = 0;
  tracked_int source { &value };
  tracked_int target { nullptr };
  target = std::move(source);
  expect(static_cast<int*>(target) == &value);
  expect(!static_cast<bool>(source));
}

constexpr void nullptr_release_is_noop() {
  c_api_release(nullptr);
  // If we get here without crashing, the nullptr check worked
}

constexpr void run_unique_res_tests() {
  expect(std::is_move_constructible_v<tracked_int>);
  expect(std::is_move_assignable_v<tracked_int>);
  expect(!std::is_copy_constructible_v<tracked_int>);
  expect(!std::is_copy_assignable_v<tracked_int>);

  empty_resource_behaves_as_expected();
  move_constructor_transfers_resource();
  move_assignment_transfers_resource();
  nullptr_release_is_noop();
  simulated_c_api_resource_lifecycle();
}

static_assert([] {
  run_unique_res_tests();
  return true;
}());

} // namespace capi::testing

#endif // CAPI_TESTING
