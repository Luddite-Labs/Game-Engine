#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

namespace bitnum {
template <typename T>
using is_enabled_t = std::enable_if_t<
		std::is_enum_v<T> &&
				!std::is_same_v<decltype(enable_bitset_enum(T{})), int>,
		int>;

/// helper: cast to underlying type
template <typename T>
constexpr auto ut(const T value) {
	return static_cast<std::underlying_type_t<T>>(value);
}

/// helper: cast to enum type
template <typename T>
constexpr T en(const std::underlying_type_t<T> value) {
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

std::string getStatusMessage();
void setStatusMessage(std::string message);
void registerUIDebugCallback(std::string name, std::function<void()> callback);
void replaceUIDebugCallback(std::string name, std::function<void()> callback);
std::vector<std::string> getRegisteredUIDebugCallbacks();
std::function<void()> getUIDebugCallbackByName(std::string name);
template <class T>
inline void hash_combine(std::size_t &seed, const T &v) {
	std::hash<T> hasher;
	// The magic constant is the golden ratio bit pattern
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

#define CHECK_AND_PRINT_SDL_ERROR()                              \
	{                                                            \
		const char *sdl_error = SDL_GetError();                  \
		if (sdl_error != nullptr && SDL_strlen(sdl_error) > 0) { \
			LOG_ERROR("SDL Error: %s",                           \
					sdl_error);                                  \
			SDL_ClearError();                                    \
		}                                                        \
	}
