#include "Camera.h"

void Camera::Init(unsigned int maxDirtyFrames, unsigned int constantBufferIndex)
{
    return UpdateObject::Init(maxDirtyFrames, constantBufferIndex);
}

void Camera::Create(const DirectX::XMFLOAT3 &position, float aspectRatio, float FOV, float nearZ, float farZ, float yaw, float pitch)
{

    MarkUpdate();

    mPosition = DirectX::XMLoadFloat3(&position);
    mForwadDirection = mForwardVector;
    mRightDirection = mRightVector;
    mUpDirection = DirectX::XMVector3Cross(mForwadDirection, mRightDirection);

    mAspectRatio = aspectRatio;
    mNearZ = nearZ;
    mFarZ = farZ;
    mFOV = FOV;

    mYaw = yaw;
    mPitch = pitch;

    mProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(mFOV, mAspectRatio, mNearZ, mFarZ);
    Update(0.0f, 0.0f, 0.0f);
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
    mForwadDirection = DirectX::XMVector3TransformCoord(mForwardVector, rotationMatrix);
    mForwadDirection = DirectX::XMVector3Normalize(mForwadDirection);

    mRightDirection = DirectX::XMVector3TransformCoord(mRightVector, rotationMatrix);
    mRightDirection = DirectX::XMVector3Normalize(mRightDirection);

    mUpDirection = DirectX::XMVector3Cross(mForwadDirection, mRightDirection);

    mViewMatrix = DirectX::XMMatrixLookToLH(mPosition, mForwadDirection, mUpDirection);
    
}

const DirectX::XMMATRIX &__vectorcall Camera::GetView() const
{
    return mViewMatrix;
}

const DirectX::XMMATRIX &__vectorcall Camera::GetProjection() const
{
    return mProjectionMatrix;
}

const DirectX::XMVECTOR& __vectorcall Camera::GetPosition() const
{
    return mPosition;
}

const DirectX::XMVECTOR& __vectorcall Camera::GetDirection() const
{
    return mForwadDirection;
}

DirectX::XMFLOAT3 Camera::GetUpDirection() const
{
    DirectX::XMFLOAT3 upDirection;
    DirectX::XMStoreFloat3(&upDirection, mUpDirection);
    return upDirection;
}

void __vectorcall Camera::SetPosition(const DirectX::XMVECTOR& position)
{
    mPosition = position;
}

const DirectX::XMVECTOR& __vectorcall Camera::GetRightDirection() const
{
    return mRightDirection;
}

void Camera::MoveForward(float dt)
{
    dt *= 5.0f;
    mPosition = DirectX::XMVectorMultiplyAdd(mForwadDirection, DirectX::XMVectorSet(dt, dt, dt, 1.0f), mPosition);
    MarkUpdate();
}

void Camera::MoveBackward(float dt)
{
    MoveForward(-dt);
}

void Camera::MoveRight(float dt)
{
    dt *= 5.0f;
    mPosition = DirectX::XMVectorMultiplyAdd(mRightDirection, DirectX::XMVectorSet(dt, dt, dt, 1.0f), mPosition);
    MarkUpdate();
}

void Camera::MoveLeft(float dt)
{
    MoveRight(-dt);
}
