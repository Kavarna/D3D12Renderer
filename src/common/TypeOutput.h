#pragma once

#include <ostream>
#include <DirectXMath.h>

namespace DirectX {
    inline std::ostream& operator << (std::ostream& stream, const DirectX::XMFLOAT2& data) {
        stream << "(" << data.x << ", " << data.y << ")";
        return stream;
    }

    inline std::ostream& operator << (std::ostream& stream, const DirectX::XMFLOAT3& data) {
        stream << "(" << data.x << ", " << data.y << ", " << data.z << ")";
        return stream;
    }

    inline std::ostream& operator << (std::ostream& stream, const DirectX::XMFLOAT4& data) {
        stream << "(" << data.x << ", " << data.y << ", " << data.z << ", " << data.w << ")";
        return stream;
    }
}
