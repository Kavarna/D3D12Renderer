#include "ThirdPersonCamera.h"


using namespace DirectX;

void ThirdPersonCamera::Init(unsigned int maxDirtyFrames, unsigned int constantBufferIndex)
{
    return UpdateObject::Init(maxDirtyFrames, constantBufferIndex);
}

void ThirdPersonCamera::Create(float aspectRatio, float FOV, float nearZ, float farZ, float yaw, float pitch)
{

    MarkUpdate();

    mPosition = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
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

void ThirdPersonCamera::Update(float dt, float mouseHorizontalMove, float mouseVerticalMove)
{
    if (mouseHorizontalMove != 0.0f || mouseVerticalMove != 0.0f)
    {
        mYaw += dt * mouseHorizontalMove;
        mPitch += dt * mouseVerticalMove;
        MarkUpdate();
    }

    // Avoid gimbal lock
    XMVECTOR rotationQuaternion = XMQuaternionRotationRollPitchYaw(mPitch, mYaw, 0.0f);

    mForwadDirection = XMVector3Rotate(mForwardVector, rotationQuaternion);
    mRightDirection = XMVector3Rotate(mRightVector, rotationQuaternion);
    mUpDirection = XMVector3Cross(mForwadDirection, mRightDirection);

    XMVECTOR cameraTarget = mCameraTarget;
    XMVECTOR cameraPosition = cameraTarget - mForwadDirection * mDistance;

    mViewMatrix = DirectX::XMMatrixLookToLH(cameraPosition, mForwadDirection, mUpDirection);
}


void ThirdPersonCamera::AdjustZoom(float zoomLevel)
{
    mDistance -= zoomLevel;
    mDistance = std::max(3.f, mDistance);
    mDistance = std::min(mDistance, 15.f);
}

const DirectX::XMMATRIX& __vectorcall ThirdPersonCamera::GetView() const
{
    return mViewMatrix;
}

const DirectX::XMMATRIX& __vectorcall ThirdPersonCamera::GetProjection() const
{
    return mProjectionMatrix;
}

DirectX::XMFLOAT3 ThirdPersonCamera::GetPosition() const
{
    DirectX::XMFLOAT3 position;
    DirectX::XMStoreFloat3(&position, mPosition);
    return position;
}

const DirectX::XMVECTOR& __vectorcall ThirdPersonCamera::GetDirection() const
{
    return mForwadDirection;
}

DirectX::XMFLOAT3 ThirdPersonCamera::GetUpDirection() const
{
    DirectX::XMFLOAT3 upDirection;
    DirectX::XMStoreFloat3(&upDirection, mUpDirection);
    return upDirection;
}

const DirectX::XMVECTOR& __vectorcall ThirdPersonCamera::GetRightDirection() const
{
    return mRightDirection;
}

void ThirdPersonCamera::SetTarget(const DirectX::XMVECTOR& target)
{
    mCameraTarget = target;
}
