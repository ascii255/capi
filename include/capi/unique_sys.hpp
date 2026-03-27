#pragma once

static_assert(__cplusplus >= 202302L, "capi requires C++23");

#include <type_traits>

namespace capi::inline v1_0_5 {

template <auto Init, auto Quit>
  requires std::is_invocable_r_v<bool, decltype(Init)> && std::is_invocable_r_v<void, decltype(Quit)>
struct unique_sys {
  constexpr unique_sys() noexcept(noexcept(Init())) : initialized { Init() } {}
  constexpr ~unique_sys() {
    if (initialized) Quit();
  }
  constexpr unique_sys(const unique_sys&) = delete;
  constexpr unique_sys& operator=(const unique_sys&) = delete;
  constexpr unique_sys(unique_sys&&) = delete;
  constexpr unique_sys& operator=(unique_sys&&) = delete;
  constexpr explicit operator bool() const noexcept { return initialized; }

private:
  bool initialized;
};

} // namespace capi::inline v1_0_5

//
//
//

#ifdef CAPI_TESTING

#include <expect/expect.hpp>

namespace capi::testing {

inline int sys_init_calls = 0;
inline int sys_quit_calls = 0;

inline void reset_sys_state() noexcept {
  sys_init_calls = 0;
  sys_quit_calls = 0;
}

inline bool sys_init() noexcept {
  ++sys_init_calls;
  return true;
}

inline void sys_quit() noexcept { ++sys_quit_calls; }

inline bool failing_sys_init() noexcept {
  ++sys_init_calls;
  return false;
}

using tracked_sys = unique_sys<sys_init, sys_quit>;
using failing_sys = unique_sys<failing_sys_init, sys_quit>;

inline void simulated_c_api_sys_lifecycle() {
  reset_sys_state();
  {
    tracked_sys handle {};
    expect(static_cast<bool>(handle));
    expect(sys_init_calls == 1);
    expect(sys_quit_calls == 0);
  }
  expect(sys_init_calls == 1);
  expect(sys_quit_calls == 1);
}

inline void failed_init_creates_uninitialized_handle() {
  reset_sys_state();
  {
    failing_sys handle {};
    expect(!static_cast<bool>(handle));
    expect(sys_init_calls == 1);
    expect(sys_quit_calls == 0);
  }
  expect(sys_quit_calls == 0);
}

constexpr void run_unique_sys_static_tests() {
  expect(!std::is_move_constructible_v<tracked_sys>);
  expect(!std::is_move_assignable_v<tracked_sys>);
  expect(!std::is_copy_constructible_v<tracked_sys>);
  expect(!std::is_copy_assignable_v<tracked_sys>);
}

inline void run_unique_sys_tests() {
  run_unique_sys_static_tests();

  simulated_c_api_sys_lifecycle();
  failed_init_creates_uninitialized_handle();
}

static_assert([] {
  run_unique_sys_static_tests();
  return true;
}());

} // namespace capi::testing

#endif // CAPI_TESTING
