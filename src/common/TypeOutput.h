#pragma once

#include <ostream>
#include <DirectXMath.h>
#include <fmt/format.h>


#define FLOAT_PRECISION ".4f"


namespace DirectX
{
    inline std::ostream &operator << (std::ostream &stream, const DirectX::XMFLOAT2 &data)
    {
        stream << "(" << data.x << ", " << data.y << ")";
        return stream;
    }

    inline std::ostream &operator << (std::ostream &stream, const DirectX::XMFLOAT3 &data)
    {
        stream << "(" << data.x << ", " << data.y << ", " << data.z << ")";
        return stream;
    }

    inline std::ostream &operator << (std::ostream &stream, const DirectX::XMFLOAT4 &data)
    {
        stream << "(" << data.x << ", " << data.y << ", " << data.z << ", " << data.w << ")";
        return stream;
    }
}

template <>
struct fmt::formatter<DirectX::XMFLOAT2>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin(), end = ctx.end();
        it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const DirectX::XMFLOAT2 &p, FormatContext &ctx) -> decltype(ctx.out())
    {
        return format_to(
            ctx.out(),
            "({}, {})",
            p.x, p.y);
    }
};

template <>
struct fmt::formatter<DirectX::XMFLOAT3>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin(), end = ctx.end();
        it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const DirectX::XMFLOAT3 &p, FormatContext &ctx) -> decltype(ctx.out())
    {
        return format_to(
            ctx.out(),
            "({}, {}, {})",
            p.x, p.y, p.z);
    }
};


template <>
struct fmt::formatter<DirectX::XMFLOAT4>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin(), end = ctx.end();
        it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const DirectX::XMFLOAT4 &p, FormatContext &ctx) -> decltype(ctx.out())
    {
        return format_to(
            ctx.out(),
            "({}, {}, {}, {})",
            p.x, p.y, p.z, p.w);
    }
};
