#pragma once

static_assert(__cplusplus >= 202302L, "capi requires C++23");

#include <concepts>
#include <type_traits>
#include <utility>

#include <capi/flag.hpp>

namespace capi::inline v1_0_6 {

template <auto Value, auto Init, auto Quit, auto Query, auto Underlying = flag_value(Value),
          typename T = decltype(Underlying)>
  requires std::is_invocable_r_v<bool, decltype(Init), T> && std::is_invocable_r_v<void, decltype(Quit), T> &&
           std::is_invocable_r_v<T, decltype(Query), T> && requires(T a, T b) {
             { a & b } -> std::convertible_to<bool>;
           }
struct unique_flgd {
  constexpr unique_flgd() noexcept(noexcept(Query(Underlying)) && noexcept(Init(Underlying)) &&
                                   std::is_nothrow_move_constructible_v<T>)
    : flag { !(Query(Underlying) & Underlying) && Init(Underlying) ? Underlying : T {} } {}
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

private:
  T flag;
};

} // namespace capi::inline v1_0_6

//
//
//

#ifdef CAPI_TESTING

#include <expect/expect.hpp>

namespace capi::testing {

enum class test_flgd_flag : unsigned {
  none = 0U,
  read = 0x1U,
  write = 0x2U,
};

inline constexpr unsigned test_plain_mask = 0x4U;

capi::flag_enum_tag enable_flag_enum(test_flgd_flag);

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

using tracked_flgd = unique_flgd<test_flgd_flag::read, init_flag, quit_flag, query_flags>;
using failing_flgd = unique_flgd<test_flgd_flag::write, fail_init_flag, quit_flag, query_flags>;
using plain_flgd = unique_flgd<test_plain_mask, init_flag, quit_flag, query_flags>;

template <typename> struct flgd_value_type;

template <auto V, auto I, auto Q, auto R, auto U, typename T>
struct flgd_value_type<capi::unique_flgd<V, I, Q, R, U, T>> {
  using type = T;
};

inline void default_underlying_type_selection_uses_expected_type() {
  expect(std::is_same_v<typename flgd_value_type<tracked_flgd>::type, unsigned>);
  expect(std::is_same_v<typename flgd_value_type<plain_flgd>::type, unsigned>);
}

inline void simulated_c_api_flgd_lifecycle() {
  reset_flagd_state();
  {
    tracked_flgd handle {};
    expect(static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == std::to_underlying(test_flgd_flag::read));
    expect(init_calls == 1U);
    expect(quit_calls == 0U);
    expect((active_flags & std::to_underlying(test_flgd_flag::read)) != 0U);
  }
  expect(init_calls == 1U);
  expect(quit_calls == 1U);
  expect((active_flags & std::to_underlying(test_flgd_flag::read)) == 0U);
}

inline void already_active_flag_skips_init() {
  reset_flagd_state();
  active_flags = std::to_underlying(test_flgd_flag::read);
  {
    tracked_flgd handle {};
    expect(!static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == 0U);
    expect(init_calls == 0U);
  }
  expect(quit_calls == 0U);
  expect((active_flags & std::to_underlying(test_flgd_flag::read)) != 0U);
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

inline void plain_value_path_acquires_and_releases() {
  reset_flagd_state();
  {
    plain_flgd handle {};
    expect(static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == test_plain_mask);
    expect((active_flags & test_plain_mask) != 0U);
  }
  expect(quit_calls == 1U);
  expect((active_flags & test_plain_mask) == 0U);
}

inline void move_constructor_transfers_flag() {
  reset_flagd_state();
  {
    tracked_flgd source {};
    tracked_flgd target { std::move(source) };
    expect(!static_cast<bool>(source));
    expect(static_cast<bool>(target));
    expect(static_cast<unsigned>(target) == std::to_underlying(test_flgd_flag::read));
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
    expect(static_cast<unsigned>(target) == std::to_underlying(test_flgd_flag::read));
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
    expect(static_cast<unsigned>(handle) == std::to_underlying(test_flgd_flag::read));

    auto& same = handle;
    handle = std::move(same);

    expect(static_cast<bool>(handle));
    expect(static_cast<unsigned>(handle) == std::to_underlying(test_flgd_flag::read));
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
  default_underlying_type_selection_uses_expected_type();

  simulated_c_api_flgd_lifecycle();
  already_active_flag_skips_init();
  failed_init_creates_empty_handle();
  plain_value_path_acquires_and_releases();
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
