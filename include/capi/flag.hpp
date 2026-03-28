#pragma once

static_assert(__cplusplus >= 202302L, "capi requires C++23");

#include <type_traits>
#include <utility>

namespace capi::inline v1_0_6 {

template <typename T>
concept flag_enum = requires(T type) { enable_flag_enum(type); };

template <typename T>
concept flag = std::is_enum_v<T> && flag_enum<T> && std::is_unsigned_v<std::underlying_type_t<T>>;

constexpr auto flag_value(auto value) noexcept {
  if constexpr (flag<decltype(value)>) {
    return std::to_underlying(value);
  } else {
    return value;
  }
}

template <flag T> constexpr T operator~(T value) noexcept { return static_cast<T>(~std::to_underlying(value)); }

template <flag T> constexpr T operator|(T lhs, T rhs) noexcept {
  return static_cast<T>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

template <flag T> constexpr T operator&(T lhs, T rhs) noexcept {
  return static_cast<T>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

template <flag T> constexpr T operator^(T lhs, T rhs) noexcept {
  return static_cast<T>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

template <flag T> constexpr T& operator|=(T& lhs, T rhs) noexcept { return lhs = lhs | rhs; }
template <flag T> constexpr T& operator&=(T& lhs, T rhs) noexcept { return lhs = lhs & rhs; }
template <flag T> constexpr T& operator^=(T& lhs, T rhs) noexcept { return lhs = lhs ^ rhs; }

#define ENABLE_FLAG_ENUM(T)                                                                                            \
  void enable_flag_enum(T);                                                                                            \
  using capi::operator~;                                                                                               \
  using capi::operator|;                                                                                               \
  using capi::operator&;                                                                                               \
  using capi::operator^;                                                                                               \
  using capi::operator|=;                                                                                              \
  using capi::operator&=;                                                                                              \
  using capi::operator^=;

} // namespace capi::inline v1_0_6

//
//
//

#ifdef CAPI_TESTING

#include <cstdint>

#include <expect/expect.hpp>

namespace capi::testing {

namespace macro_fixture {

enum class macro_flag : unsigned {
  read = 0x1U,
  write = 0x2U,
};

ENABLE_FLAG_ENUM(macro_flag)

} // namespace macro_fixture

enum class test_flag : std::uint32_t {
  none = 0U,
  read = 1U << 0U,
  write = 1U << 1U,
  exec = 1U << 2U,
};

enum class non_flag_enum : std::uint32_t {
  a = 1U,
  b = 2U,
};

enum class signed_flag_enum : std::int32_t {
  x = 1,
  y = 2,
};

enum class external_test_flag : unsigned;

void enable_flag_enum(test_flag);
void enable_flag_enum(signed_flag_enum);
void enable_flag_enum(external_test_flag);

constexpr auto to_underlying(auto value) { return static_cast<std::underlying_type_t<decltype(value)>>(value); }

constexpr void flag_concept_accepts_only_opted_in_unsigned_enums() {
  expect(flag<test_flag>);
  expect(!flag<non_flag_enum>);
  expect(!flag<signed_flag_enum>);
}

constexpr void bitwise_or_operator_combines_flags() {
  constexpr test_flag combined = test_flag::read | test_flag::write;
  expect(to_underlying(combined) == (to_underlying(test_flag::read) | to_underlying(test_flag::write)));
}

constexpr void bitwise_and_operator_intersects_flags() {
  constexpr test_flag value = test_flag::read | test_flag::write;
  constexpr test_flag masked = value & test_flag::write;
  expect(to_underlying(masked) == to_underlying(test_flag::write));
}

constexpr void bitwise_xor_operator_toggles_flags() {
  constexpr test_flag value = test_flag::read | test_flag::write;
  constexpr test_flag toggled = value ^ test_flag::write;
  expect(to_underlying(toggled) == to_underlying(test_flag::read));
}

constexpr void bitwise_not_operator_inverts_bits() {
  constexpr test_flag inverted = ~test_flag::none;
  expect(to_underlying(inverted) == ~to_underlying(test_flag::none));
}

constexpr void bitwise_not_operator_invokes_runtime_overload() {
  test_flag value = test_flag::none;
  const test_flag inverted = ~value;
  expect(to_underlying(inverted) == ~to_underlying(test_flag::none));
}

constexpr void compound_assignment_operators_modify_in_place() {
  test_flag value = test_flag::none;

  value |= test_flag::read;
  expect(to_underlying(value) == to_underlying(test_flag::read));

  value |= test_flag::write;
  expect(to_underlying(value) == (to_underlying(test_flag::read) | to_underlying(test_flag::write)));

  value &= test_flag::write;
  expect(to_underlying(value) == to_underlying(test_flag::write));

  value ^= test_flag::exec;
  expect(to_underlying(value) == (to_underlying(test_flag::write) ^ to_underlying(test_flag::exec)));
}

constexpr void flag_value_converts_flags_and_preserves_plain_values() {
  constexpr std::uint32_t plain_mask = 0x8U;
  constexpr auto external_read = static_cast<external_test_flag>(0x1U);
  expect(capi::flag_value(test_flag::read) == to_underlying(test_flag::read));
  expect(capi::flag_value(external_read) == 0x1U);
  expect(capi::flag_value(plain_mask) == plain_mask);
}

constexpr void enable_flag_enum_macro_opts_in_and_exposes_operators() {
  using macro_fixture::macro_flag;

  expect(flag<macro_flag>);

  macro_flag value = macro_flag::read;
  value |= macro_flag::write;

  expect(to_underlying(value) == (to_underlying(macro_flag::read) | to_underlying(macro_flag::write)));
}

constexpr void run_flag_tests() {
  flag_concept_accepts_only_opted_in_unsigned_enums();
  bitwise_or_operator_combines_flags();
  bitwise_and_operator_intersects_flags();
  bitwise_xor_operator_toggles_flags();
  bitwise_not_operator_inverts_bits();
  bitwise_not_operator_invokes_runtime_overload();
  compound_assignment_operators_modify_in_place();
  flag_value_converts_flags_and_preserves_plain_values();
  enable_flag_enum_macro_opts_in_and_exposes_operators();
}

static_assert([] {
  run_flag_tests();
  return true;
}());

} // namespace capi::testing

#endif // CAPI_TESTING
