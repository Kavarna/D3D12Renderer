#pragma once

#include <cstdint>
#include <functional>
#include <DirectXMath.h>
#include <directxcollision.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT

namespace Math {
    enum class Axis {
        X = 0, Y = 1, Z = 2
    };

    constexpr float PI = 3.1415926535897932384626433832795f;
    constexpr float _2PI = 2.0f * PI;
    constexpr float EPSILON = 1e-10f;
    // Convert radians to degrees.
    constexpr float Degrees(const float radians) {
        return radians * (180.0f / PI);
    }

    // Convert degrees to radians.
    constexpr float Radians(const float degrees) {
        return degrees * (PI / 180.0f);
    }

    template<typename T>
    inline T Deadzone(T val, T deadzone) {
        if (std::abs(val) < deadzone) {
            return T(0);
        }

        return val;
    }

    // Normalize a value in the range [min - max]
    template<typename T, typename U>
    inline T NormalizeRange(U x, U min, U max) {
        return T(x - min) / T(max - min);
    }

    // Shift and bias a value into another range.
    template<typename T, typename U>
    inline T ShiftBias(U x, U shift, U bias) {
        return T(x * bias) + T(shift);
    }

    /***************************************************************************
    * These functions were taken from the MiniEngine.
    * Source code available here:
    * https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Math/Common.h
    * Retrieved: January 13, 2016
    **************************************************************************/
    template <typename T>
    inline T AlignUpWithMask(T value, size_t mask) {
        return (T)(((size_t)value + mask) & ~mask);
    }

    template <typename T>
    inline T AlignDownWithMask(T value, size_t mask) {
        return (T)((size_t)value & ~mask);
    }

    template <typename T>
    inline T AlignUp(T value, size_t alignment) {
        return AlignUpWithMask(value, alignment - 1);
    }

    template <typename T>
    inline T AlignDown(T value, size_t alignment) {
        return AlignDownWithMask(value, alignment - 1);
    }

    template <typename T>
    inline bool IsAligned(T value, size_t alignment) {
        return 0 == ((size_t)value & (alignment - 1));
    }

    template <typename T>
    inline T DivideByMultiple(T value, size_t alignment) {
        return (T)((value + alignment - 1) / alignment);
    }
    /***************************************************************************/

    /**
    * Round up to the next highest power of 2.
    * @source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    * @retrieved: January 16, 2016
    */
    inline uint32_t NextHighestPow2(uint32_t v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;

        return v;
    }

    /**
    * Round up to the next highest power of 2.
    * @source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    * @retrieved: January 16, 2016
    */
    inline uint64_t NextHighestPow2(uint64_t v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        v++;

        return v;
    }

    inline float GetValueOnAxis(const DirectX::XMFLOAT3& vct, Axis axis) {
        switch (axis) 	{
            case Math::Axis::X:
                return vct.x;
            case Math::Axis::Y:
                return vct.y;
            case Math::Axis::Z:
                return vct.z;
            default:
                throw std::exception("What kind of axis did you pass?");
        }
    }

    /// Clamps a value between lower and upper
    template <class type>
    inline type clamp(type &x, type lower, type upper)
    {
        return std::max(lower, std::min(x, upper));
    }
}
