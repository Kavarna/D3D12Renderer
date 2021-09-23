#pragma once



template <typename ... Args>  struct countParameters;

template <>
struct countParameters<> {
	constexpr static const int value = 0;
};

template <typename T, typename ... Args>
struct countParameters<T, Args...> {
	constexpr static const int value = 1 + countParameters<Args...>::value;
};