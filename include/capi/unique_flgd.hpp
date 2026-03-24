#pragma once

static_assert(__cplusplus >= 202302L, "capi requires C++23");

#include <type_traits>
#include <utility>

namespace capi::inline v1_0_5 {

template <auto Value, auto Init, auto Quit, auto Query>
  requires std::is_invocable_r_v<bool, decltype(Init), decltype(Value)> &&
           std::is_invocable_r_v<void, decltype(Quit), decltype(Value)> &&
           std::is_invocable_r_v<decltype(Value), decltype(Query), decltype(Value)> &&
           requires(decltype(Value) a, decltype(Value) b) { a & b; }
struct unique_flgd {
  using T = decltype(Value);

private:
  T flag;

public:
  constexpr unique_flgd() noexcept(noexcept(Query(Value)) && noexcept(Init(Value)) &&
                                   std::is_nothrow_move_constructible_v<T>)
      : flag { !(Query(Value) & Value) && Init(Value) ? Value : T {} } {}
  constexpr ~unique_flgd() {
    if (static_cast<bool>(*this)) Quit(flag);
  }
  constexpr unique_flgd(const unique_flgd&) = delete;
  constexpr unique_flgd& operator=(const unique_flgd&) = delete;
  constexpr unique_flgd(unique_flgd&& other) noexcept : flag { std::exchange(other.flag, T {}) } {}
  constexpr unique_flgd& operator=(unique_flgd&& other) noexcept(std::is_nothrow_swappable_v<T>) {
    std::swap(flag, other.flag);
    return *this;
  }
  
  constexpr explicit operator T() const noexcept { return flag; }
  constexpr explicit operator bool() const noexcept(noexcept(Query(std::declval<T>()))) { return Query(flag) & flag; }
};

} // namespace capi::inline v1_0_5

//
//
//

#ifdef CAPI_TESTING

#include <expect/expect.hpp>

namespace capi::testing {

inline constexpr unsigned test_flag_read = 0x1U;
inline constexpr unsigned test_flag_write = 0x2U;

inline unsigned active_flags = 0U;
inline unsigned init_calls = 0U;
inline unsigned quit_calls = 0U;

inline void reset_flagd_state() noexcept {
  active_flags = 0U;
  init_calls = 0U;
  quit_calls = 0U;
}

inline unsigned query_flags(unsigned) noexcept { return active_flags; }

inline bool init_flag(unsigned flag) noexcept {
  ++init_calls;
  active_flags |= flag;
  return true;
}

inline bool fail_init_flag(unsigned) noexcept {
  ++init_calls;
  return false;
}

inline void quit_flag(unsigned flag) noexcept {
  ++quit_calls;
  active_flags &= ~flag;
}

using tracked_flgd = unique_flgd<test_flag_read, init_flag, quit_flag, query_flags>;
using failing_flgd = unique_flgd<test_flag_write, fail_init_flag, quit_flag, query_flags>;

inline void simulated_c_api_flgd_lifecycle() {
  reset_flagd_state();
  {
    tracked_flgd handle {};
    expect(static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == test_flag_read);
    expect(init_calls == 1U);
    expect(quit_calls == 0U);
    expect((active_flags & test_flag_read) != 0U);
  }
  expect(init_calls == 1U);
  expect(quit_calls == 1U);
  expect((active_flags & test_flag_read) == 0U);
}

inline void already_active_flag_skips_init() {
  reset_flagd_state();
  active_flags = test_flag_read;
  {
    tracked_flgd handle {};
    expect(!static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == 0U);
    expect(init_calls == 0U);
  }
  expect(quit_calls == 0U);
  expect((active_flags & test_flag_read) != 0U);
}

inline void failed_init_creates_empty_handle() {
  reset_flagd_state();
  {
    failing_flgd handle {};
    expect(!static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == 0U);
    expect(init_calls == 1U);
    expect(quit_calls == 0U);
  }
  expect(quit_calls == 0U);
}

inline void move_constructor_transfers_flag() {
  reset_flagd_state();
  {
    tracked_flgd source {};
    tracked_flgd target { std::move(source) };
    expect(!static_cast<bool>(source));
    expect(static_cast<bool>(target));
    expect(static_cast<unsigned>(target) == test_flag_read);
    expect(quit_calls == 0U);
  }
  expect(quit_calls == 1U);
  expect(active_flags == 0U);
}

inline void move_assignment_transfers_flag_to_empty_target() {
  reset_flagd_state();
  {
    tracked_flgd source {};
    tracked_flgd target {};

    expect(init_calls == 1U);
    expect(static_cast<bool>(source));
    expect(!static_cast<bool>(target));

    target = std::move(source);

    expect(!static_cast<bool>(source));
    expect(static_cast<bool>(target));
    expect(static_cast<unsigned>(target) == test_flag_read);
    expect(static_cast<unsigned>(source) == 0U);
    expect(quit_calls == 0U);
  }
  expect(quit_calls == 1U);
  expect(active_flags == 0U);
}

inline void self_move_assignment_keeps_single_release() {
  reset_flagd_state();
  {
    tracked_flgd handle {};
    expect(static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == test_flag_read);

    auto& same = handle;
    handle = std::move(same);

    expect(static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == test_flag_read);
    expect(quit_calls == 0U);
  }
  expect(quit_calls == 1U);
  expect(active_flags == 0U);
}

constexpr void run_unique_flgd_static_tests() {
  expect(std::is_move_constructible_v<tracked_flgd>);
  expect(std::is_move_assignable_v<tracked_flgd>);
  expect(!std::is_copy_constructible_v<tracked_flgd>);
  expect(!std::is_copy_assignable_v<tracked_flgd>);
}

inline void run_unique_flgd_tests() {
  run_unique_flgd_static_tests();

  simulated_c_api_flgd_lifecycle();
  already_active_flag_skips_init();
  failed_init_creates_empty_handle();
  move_constructor_transfers_flag();
  move_assignment_transfers_flag_to_empty_target();
  self_move_assignment_keeps_single_release();
}

static_assert([] {
  run_unique_flgd_static_tests();
  return true;
}());

} // namespace capi::testing

#endif // CAPI_TESTING
