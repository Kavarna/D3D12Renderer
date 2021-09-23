#pragma once


#include <vector>
#include <array>



template <typename T>
constexpr auto PPTS() {
#if defined(__FUNCSIG__)
# define PP __FUNCSIG__
    int suffix = sizeof ">(void)";
#else
# define PP __PRETTY_FUNCTION__
    int suffix = sizeof "]";
#endif
    return sizeof PP - suffix;
}

template <typename T>
constexpr auto GetTypeName() {
    constexpr int prefix = PPTS<int>() - 3;
    constexpr int N = PPTS<T>() - prefix;
    struct R {
        char name[N + 1]{};
    } r;
    for (int i = 0; i != N; ++i)
        r.name[i] = (PP + prefix)[i];
    return r;
#undef PP
}



namespace Detail {
    template <typename T>
    auto IsIterableImpl(int) -> decltype (
        begin(std::declval<T&>()) != end(std::declval<T&>()), // begin/end and operator !=
        void(), // Handle evil operator ,
        ++std::declval<decltype(begin(std::declval<T&>()))&>(), // operator ++
        void(*begin(std::declval<T&>())), // operator*
        std::true_type{});

    template <typename T>
    std::false_type IsIterableImpl(...);
}

template <typename T>
using IsIterable = decltype(Detail::IsIterableImpl<T>(0));


template <typename T>
struct IsDefaultConstructible {
    template <typename U>
    static constexpr uint32_t SFINAE(decltype(U())*);
    template <typename U>
    static constexpr uint8_t SFINAE(...);

    static const bool value = sizeof(SFINAE<T>(nullptr)) == sizeof(int32_t);
};

template <typename T, typename U>
struct GetTypeInsideGenericList
{

};

// To be added as needed

