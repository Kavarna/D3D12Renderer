#include "OrthographicCamera.h"

void OrthographicCamera::Init(unsigned int maxDirtyFrames, unsigned int constantBufferIndex)
{
    return UpdateObject::Init(maxDirtyFrames, constantBufferIndex);
}

void OrthographicCamera::Create(const DirectX::XMFLOAT3 &position, float width, float height,
                                float nearZ, float farZ, float yaw, float pitch)
{
    MarkUpdate();

    mPosition = DirectX::XMLoadFloat3(&position);
    mForwadDirection = mForwardVector;
    mRightDirection = mRightVector;
    mUpDirection = DirectX::XMVector3Cross(mForwadDirection, mRightDirection);

    mNearZ = nearZ;
    mFarZ = farZ;

    mWidth = width;
    mHeight = height;

    mYaw = yaw;
    mPitch = pitch;

    mProjectionMatrix = DirectX::XMMatrixOrthographicLH(mWidth, mHeight, nearZ, farZ);

    Update(0.0f, 0.0f, 0.0f);
}

void OrthographicCamera::Update(float dt, float mouseHorizontalMove, float mouseVerticalMove)
{
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0.0f);
    mForwadDirection = DirectX::XMVector3TransformCoord(mForwardVector, rotationMatrix);
    mForwadDirection = DirectX::XMVector3Normalize(mForwadDirection);

    mRightDirection = DirectX::XMVector3TransformCoord(mRightVector, rotationMatrix);
    mRightDirection = DirectX::XMVector3Normalize(mRightDirection);

    mUpDirection = DirectX::XMVector3Cross(mForwadDirection, mRightDirection);

    mViewMatrix = DirectX::XMMatrixLookToLH(mPosition, mForwadDirection, mUpDirection);
}

DirectX::XMFLOAT2 OrthographicCamera::ConvertCoordinatesToNDC(const DirectX::XMFLOAT2 &coordinates)
{
    DirectX::XMFLOAT2 result = {};

    result.x = (coordinates.x / mWidth * 2) - 1.f;
    result.y = -((coordinates.y / mHeight * 2) - 1.f);

    return result;
}

DirectX::XMFLOAT2 OrthographicCamera::ConvertCoordinatesToNDCAdapted(const DirectX::XMFLOAT2 &coordinates)
{
    DirectX::XMFLOAT2 result = ConvertCoordinatesToNDC(coordinates);
    result.x *= (mWidth / 2.f);
    result.y *= (mHeight / 2.f);

    return result;
}


const DirectX::XMMATRIX &__vectorcall OrthographicCamera::GetView() const
{
    return mViewMatrix;
}

const DirectX::XMMATRIX &__vectorcall OrthographicCamera::GetProjection() const
{
    return mProjectionMatrix;
}

const DirectX::XMVECTOR& __vectorcall OrthographicCamera::GetDirection() const
{
    return mForwadDirection;
}

const DirectX::XMVECTOR& __vectorcall OrthographicCamera::GetRightDirection() const
{
    return mRightDirection;
}

const DirectX::XMVECTOR& __vectorcall OrthographicCamera::GetPosition() const
{
    return mPosition;
}
