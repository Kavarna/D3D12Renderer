#pragma once


#include <Oblivion.h>
#include "Utils/UploadBuffer.h"


struct PerObjectInfo
{
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX TexWorld;
};

struct PerPassInfo
{
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Projection;
};

#define MAX_LIGHTS 10

struct LightCB
{
    DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
    float FalloffStart = 1.0f;                          // point/spot light only
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0 }; // directional/spot only
    float FalloffEnd = 10.0f;                           // point/spot light only
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
    float SpotPower = 64.0f;                            // Spotlight only
};

struct LightsBuffer
{
    DirectX::XMFLOAT4 AmbientColor = { 0.0f, 0.0f, 0.0f, 0.0f };

    LightCB Lights[MAX_LIGHTS];

    unsigned int NumDirectionalLights = 0;
    unsigned int NumPointLights = 0;
    unsigned int NumSpotLights = 0;
};

struct MaterialConstants
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Shininess = 0.75f;

    DirectX::XMMATRIX MaterialTransform = DirectX::XMMatrixIdentity();

    int textureIndex;
};

struct FrameResources
{
    FrameResources() = default;
    ~FrameResources() = default;

    bool Init(uint32_t numObjects, uint32_t numPasses, uint32_t numMaterials, uint32_t width, uint32_t height);

    ComPtr<ID3D12CommandAllocator> CommandAllocator;

    UploadBuffer<PerObjectInfo> PerObjectBuffers;
    UploadBuffer<PerPassInfo> PerPassBuffers;
    UploadBuffer<MaterialConstants> MaterialsBuffers;
    UploadBuffer<LightsBuffer> LightsBuffer;

    uint32_t BlurRenderTargetIndex;

    uint64_t FenceValue = 0;
};


