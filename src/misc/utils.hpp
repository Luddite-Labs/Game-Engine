#pragma once
#include <cstdint>
#include <type_traits>

/** \file

Bitset operations for any enum type can be enabled by declaring a function
<code>void enable_bitset_enum(T);</code> (no definition required).
*/
namespace bitnum {
template <typename T>
using is_enabled_t = std::enable_if_t<
    std::is_enum_v<T> &&
        !std::is_same_v<decltype(enable_bitset_enum(T{})), int>,
    int>;

/// helper: cast to underlying type
template <typename T> constexpr auto ut(const T value) {
  return static_cast<std::underlying_type_t<T>>(value);
}

/// helper: cast to enum type
template <typename T> constexpr T en(const std::underlying_type_t<T> value) {
  return static_cast<T>(value);
}
} // namespace bitnum

// ----- operator |, |=

template <typename T, typename bitnum::is_enabled_t<T> = 0>
constexpr T operator|(const T lhs, const T rhs) {
  using namespace bitnum;
  return en<T>(ut(lhs) | ut(rhs));
}

template <typename T, typename bitnum::is_enabled_t<T> = 0>
constexpr T &operator|=(T &lhs, const T rhs) {
  lhs = lhs | rhs;
  return lhs;
}

// ----- operator &, &=

template <typename T, typename bitnum::is_enabled_t<T> = 0>
constexpr T operator&(const T lhs, const T rhs) {
  using namespace bitnum;
  return en<T>(ut(lhs) & ut(rhs));
}

template <typename T, typename bitnum::is_enabled_t<T> = 0>
constexpr T &operator&=(T &lhs, const T rhs) {
  lhs = lhs & rhs;
  return lhs;
}

// ----- operator ^, =

template <typename T, typename bitnum::is_enabled_t<T> = 0>
constexpr T operator^(const T lhs, const T rhs) {
  using namespace bitnum;
  return en<T>(ut(lhs) ^ ut(rhs));
}

template <typename T, typename bitnum::is_enabled_t<T> = 0>
constexpr T &operator^=(T &lhs, const T rhs) {
  lhs = lhs ^ rhs;
  return lhs;
}

// ----- operator -, -= (implementing &~, &=~)

template <typename T, typename bitnum::is_enabled_t<T> = 0>
constexpr T operator-(const T lhs, const T rhs) {
  using namespace bitnum;
  return en<T>(ut(lhs) & ~ut(rhs));
}

template <typename T, typename bitnum::is_enabled_t<T> = 0>
constexpr T &operator-=(T &lhs, const T rhs) {
  lhs = lhs - rhs;
  return lhs;
}
struct Handle {
  uint32_t slot_index;
  uint32_t generation;
};