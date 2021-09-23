#pragma once


#include <DirectXMath.h>

namespace DirectX {
	inline DirectX::XMFLOAT2 operator- (const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs) {
		return DirectX::XMFLOAT2(lhs.x - rhs.x, lhs.y - rhs.y);
	}

	inline DirectX::XMFLOAT3 operator- (const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
		return DirectX::XMFLOAT3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
	}

	inline DirectX::XMFLOAT4 operator- (const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs) {
		return DirectX::XMFLOAT4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
	}

	inline DirectX::XMFLOAT2 operator+ (const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs) {
		return DirectX::XMFLOAT2(lhs.x + rhs.x, lhs.y + rhs.y);
	}

	inline DirectX::XMFLOAT3 operator+ (const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
		return DirectX::XMFLOAT3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
	}

	inline DirectX::XMFLOAT4 operator+ (const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs) {
		return DirectX::XMFLOAT4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
	}

	inline DirectX::XMFLOAT2 operator/ (const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs) {
		return DirectX::XMFLOAT2(lhs.x / rhs.x, lhs.y / rhs.y);
	}

	inline DirectX::XMFLOAT3 operator/ (const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
		return DirectX::XMFLOAT3(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
	}

	inline DirectX::XMFLOAT4 operator/ (const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs) {
		return DirectX::XMFLOAT4(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
	}

	inline DirectX::XMFLOAT2 operator/ (const DirectX::XMFLOAT2& lhs, float rhs) {
		return DirectX::XMFLOAT2(lhs.x / rhs, lhs.y / rhs);
	}

	inline DirectX::XMFLOAT3 operator/ (const DirectX::XMFLOAT3& lhs, float rhs) {
		return DirectX::XMFLOAT3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
	}

	inline DirectX::XMFLOAT4 operator/ (const DirectX::XMFLOAT4& lhs, float rhs) {
		return DirectX::XMFLOAT4(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs);
	}

	inline DirectX::XMFLOAT2 operator* (const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs) {
		return DirectX::XMFLOAT2(lhs.x * rhs.x, lhs.y * rhs.y);
	}

	inline DirectX::XMFLOAT3 operator* (const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
		return DirectX::XMFLOAT3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
	}

	inline DirectX::XMFLOAT4 operator* (const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs) {
		return DirectX::XMFLOAT4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
	}

	inline DirectX::XMFLOAT2 operator* (const DirectX::XMFLOAT2& lhs, float rhs) {
		return DirectX::XMFLOAT2(lhs.x * rhs, lhs.y * rhs);
	}

	inline DirectX::XMFLOAT3 operator* (const DirectX::XMFLOAT3& lhs, float rhs) {
		return DirectX::XMFLOAT3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
	}

	inline DirectX::XMFLOAT4 operator* (const DirectX::XMFLOAT4& lhs, float rhs) {
		return DirectX::XMFLOAT4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
	}
}
