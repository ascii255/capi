#pragma once

#include <memory>
#include <utility>

namespace capi::inline v1_0_1 {

template <typename T, typename D = void (*)(T*)> struct unique_res {
protected:
  std::unique_ptr<T, D> resource;

public:
  constexpr explicit unique_res(T* ptr, D deleter) noexcept : resource {ptr, std::move(deleter)} {}
  unique_res(const unique_res&) = delete;
  unique_res& operator=(const unique_res&) = delete;
  constexpr unique_res(unique_res&&) noexcept = default;
  constexpr unique_res& operator=(unique_res&&) noexcept = default;

  constexpr explicit operator T*() const noexcept { return resource.get(); }
  constexpr explicit operator bool() const noexcept { return resource != nullptr; }
};

} // namespace capi::inline v1_0_1

#ifdef CAPI_TESTING

#include <expect>
#include <type_traits>

namespace capi::testing {

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

constexpr void simulated_c_api_resource_lifecycle() {
  int release_count = 0;
  bool released = false;
  fake_c_api_resource resource {&release_count, &released};

  {
    unique_res<fake_c_api_resource> handle {c_api_acquire(&resource), &c_api_release};
    expect(static_cast<bool>(handle));
    expect(static_cast<fake_c_api_resource*>(handle) == &resource);
    expect(release_count == 0);
    expect(!released);
  }

  expect(release_count == 1);
  expect(released);
}

constexpr void empty_resource_behaves_as_expected() {
  unique_res<int> resource {nullptr, &no_op_int_deleter};
  expect(!static_cast<bool>(resource));
  expect(static_cast<int*>(resource) == nullptr);
}

constexpr void move_constructor_transfers_resource() {
  int value = 0;
  unique_res<int> source {&value, &no_op_int_deleter};
  unique_res<int> target {std::move(source)};
  expect(static_cast<int*>(target) == &value);
  expect(!static_cast<bool>(source));
}

constexpr void move_assignment_transfers_resource() {
  int value = 0;
  unique_res<int> source {&value, &no_op_int_deleter};
  unique_res<int> target {nullptr, &no_op_int_deleter};
  target = std::move(source);
  expect(static_cast<int*>(target) == &value);
  expect(!static_cast<bool>(source));
}

constexpr void run_unique_res_tests() {
  expect(std::is_move_constructible_v<unique_res<int>>);
  expect(std::is_move_assignable_v<unique_res<int>>);
  expect(!std::is_copy_constructible_v<unique_res<int>>);
  expect(!std::is_copy_assignable_v<unique_res<int>>);

  empty_resource_behaves_as_expected();
  move_constructor_transfers_resource();
  move_assignment_transfers_resource();
  simulated_c_api_resource_lifecycle();
}

#ifdef CAPI_STATIC_TESTING
static_assert([] {
  run_unique_res_tests();
  return true;
}());
#endif

} // namespace capi::testing

#endif
