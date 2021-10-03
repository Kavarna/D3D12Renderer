#pragma once



#include <Oblivion.h>
#include "Utils/UpdateObject.h"


OBLIVION_ALIGN(16)
class Camera : public UpdateObject
{
public:
    Camera() = default;

public:
    void Create(const DirectX::XMFLOAT3& position, float aspectRatio,
                float FOV = DirectX::XM_PIDIV4, float nearZ = 0.1f, float farZ = 1000.f,
                float yaw = 0.0f, float pitch = 0.0f);


    void Update(float dt, float mouseHorizontalMove, float mouseVerticalMove);

    const DirectX::XMMATRIX &__vectorcall GetView() const;
    const DirectX::XMMATRIX &__vectorcall GetProjection() const;

    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT3 GetDirection() const;
    DirectX::XMFLOAT3 GetUpDirection() const;
    DirectX::XMFLOAT3 GetRightDirection() const;

public:
    void MoveForward(float dt);
    void MoveBackward(float dt);
    void MoveRight(float dt);
    void MoveLeft(float dt);

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

    float mAspectRatio;
    float mNearZ;
    float mFarZ;
    float mFOV;

    float mYaw;
    float mPitch;
};


