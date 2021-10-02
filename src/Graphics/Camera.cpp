#include "Camera.h"

void Camera::Create(const DirectX::XMFLOAT3 &position, const DirectX::XMFLOAT3 &direction, const DirectX::XMFLOAT3 &rightDirection,
                    float aspectRatio, float FOV, float nearZ, float farZ, float yaw, float pitch)
{
    MarkUpdate();

    mPosition = DirectX::XMLoadFloat3(&position);
    mForwadDirection = DirectX::XMLoadFloat3(&direction);
    mRightDirection = DirectX::XMLoadFloat3(&rightDirection);
    mUpDirection = DirectX::XMVector3Cross(mForwadDirection, mRightDirection);

    mAspectRatio = aspectRatio;
    mNearZ = nearZ;
    mFarZ = farZ;
    mFOV = FOV;

    mYaw = yaw;
    mPitch = pitch;

    mProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(mFOV, mAspectRatio, mNearZ, mFarZ);
}

void Camera::Update(float dt, float mouseHorizontalMove, float mouseVerticalMove)
{
    if (mouseHorizontalMove != 0.0f || mouseVerticalMove != 0.0f)
    {
        mYaw += mouseHorizontalMove * dt;
        mPitch += mouseVerticalMove * dt;
        MarkUpdate();
    }

    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0.0f);
    mForwadDirection = DirectX::XMVector3TransformCoord(mForwadDirection, rotationMatrix);
    mForwadDirection = DirectX::XMVector3Normalize(mForwadDirection);

    mRightDirection = DirectX::XMVector3TransformCoord(mRightDirection, rotationMatrix);
    mRightDirection = DirectX::XMVector3Normalize(mRightDirection);

    mUpDirection = DirectX::XMVector3Cross(mForwadDirection, mRightDirection);

    mViewMatrix = DirectX::XMMatrixLookToLH(mPosition, mForwadDirection, mUpDirection);
    
    mYaw = 0.0f;
    mPitch = 0.0f;
}

const DirectX::XMMATRIX &__vectorcall Camera::GetView() const
{
    return mViewMatrix;
}

const DirectX::XMMATRIX &__vectorcall Camera::GetProjection() const
{
    return mProjectionMatrix;
}

DirectX::XMFLOAT3 Camera::GetPosition() const
{
    DirectX::XMFLOAT3 position;
    DirectX::XMStoreFloat3(&position, mPosition);
    return position;
}

DirectX::XMFLOAT3 Camera::GetDirection() const
{
    DirectX::XMFLOAT3 direction;
    DirectX::XMStoreFloat3(&direction, mForwadDirection);
    return direction;
}

DirectX::XMFLOAT3 Camera::GetUpDirection() const
{
    DirectX::XMFLOAT3 upDirection;
    DirectX::XMStoreFloat3(&upDirection, mUpDirection);
    return upDirection;
}

DirectX::XMFLOAT3 Camera::GetRightDirection() const
{
    DirectX::XMFLOAT3 rightDirection;
    DirectX::XMStoreFloat3(&rightDirection, mRightDirection);
    return rightDirection;
}

void Camera::MoveForward(float dt)
{
    mPosition = DirectX::XMVectorMultiplyAdd(mForwadDirection, DirectX::XMVectorSet(dt, dt, dt, 1.0f), mPosition);
    MarkUpdate();
}

void Camera::MoveBackward(float dt)
{
    MoveForward(-dt);
}

void Camera::MoveRight(float dt)
{
    mPosition = DirectX::XMVectorMultiplyAdd(mRightDirection, DirectX::XMVectorSet(dt, dt, dt, 1.0f), mPosition);
    MarkUpdate();
}

void Camera::MoveLeft(float dt)
{
    MoveRight(-dt);
}
