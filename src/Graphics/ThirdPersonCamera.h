#pragma once


#include <Oblivion.h>
#include "Utils/UpdateObject.h"
#include "Interfaces/ICamera.h"


OBLIVION_ALIGN(16)
class ThirdPersonCamera : public UpdateObject, public ICamera
{
public:
    ThirdPersonCamera() = default;
    ~ThirdPersonCamera() = default;

public:
    void Init(unsigned int maxDirtyFrames, unsigned int constantBufferIndex);
    void Create(float aspectRatio,
        float FOV = DirectX::XM_PIDIV4, float nearZ = 0.1f, float farZ = 1000.f,
        float yaw = 0.0f, float pitch = 0.0f);

    void Update(float dt, float mouseHorizontalMove, float mouseVerticalMove);

    void AdjustZoom(float zoomLevel);

    const DirectX::XMMATRIX& __vectorcall GetView() const override;
    const DirectX::XMMATRIX& __vectorcall GetProjection() const override;
    const DirectX::XMVECTOR& __vectorcall GetDirection() const override;
    const DirectX::XMVECTOR& __vectorcall GetRightDirection() const override;

    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT3 GetUpDirection() const;

    void SetTarget(const DirectX::XMVECTOR&);

private:
    DirectX::XMVECTOR mForwardVector = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR mRightVector = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR mUpVector = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    DirectX::XMVECTOR mCameraTarget = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    DirectX::XMVECTOR mPosition;
    DirectX::XMVECTOR mForwadDirection;
    DirectX::XMVECTOR mUpDirection;
    DirectX::XMVECTOR mRightDirection;

    DirectX::XMMATRIX mViewMatrix;
    DirectX::XMMATRIX mProjectionMatrix;

    float mAspectRatio;
    float mNearZ;
    float mFarZ;
    float mFOV;

    float mYaw;
    float mPitch;

    float mDistance= 5.0f;
};


