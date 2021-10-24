#pragma once


#include "Interfaces/ICamera.h"
#include "Utils/UpdateObject.h"


OBLIVION_ALIGN(16)
class OrthographicCamera : public UpdateObject, public ICamera
{
public:
    OrthographicCamera() = default;
    ~OrthographicCamera() = default;

public:
    void Init(unsigned int maxDirtyFrames, unsigned int constantBufferIndex);
    void Create(const DirectX::XMFLOAT3 &position, float width, float height,
                float nearZ = 0.1f, float farZ = 1000.f,
                float yaw = 0.0f, float pitch = 0.0f);


    void Update(float dt, float mouseHorizontalMove, float mouseVerticalMove);

    DirectX::XMFLOAT2 ConvertCoordinatesToNDC(const DirectX::XMFLOAT2& coordinates);
    DirectX::XMFLOAT2 ConvertCoordinatesToNDCAdapted(const DirectX::XMFLOAT2 &coordinates);

    const DirectX::XMMATRIX &__vectorcall GetView() const;
    const DirectX::XMMATRIX &__vectorcall GetProjection() const;

private:

    DirectX::XMVECTOR mForwardVector = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR mRightVector = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR mUpVector = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    DirectX::XMVECTOR mPosition;
    DirectX::XMVECTOR mForwadDirection;
    DirectX::XMVECTOR mUpDirection;
    DirectX::XMVECTOR mRightDirection;

    DirectX::XMMATRIX mViewMatrix;
    DirectX::XMMATRIX mProjectionMatrix;

    float mNearZ;
    float mFarZ;
    float mWidth;
    float mHeight;

    float mYaw;
    float mPitch;

};

