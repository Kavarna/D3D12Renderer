#pragma once


#include <optional>


template <typename T>
class Result : protected std::optional<T>
{
public:
    explicit Result() noexcept = default;

    constexpr Result(T const &&t) noexcept
        : std::optional<T>(t)
    {}

    constexpr Result(std::nullopt_t) noexcept
        : std::optional<T>(std::nullopt)
    {}


    constexpr bool Valid() noexcept
    {
        return has_value();
    }

    constexpr bool Invalid() noexcept
    {
        return !has_value();
    }

    constexpr T &&Get() noexcept
    {
        return std::move(this->value());
    }
};

