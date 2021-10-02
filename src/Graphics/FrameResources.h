#pragma once


#include <Oblivion.h>
#include "Utils/UploadBuffer.h"


struct PerObjectInfo
{
    DirectX::XMMATRIX World;
};

struct PerPassInfo
{
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Projection;
};


struct FrameResources
{
    FrameResources() = default;
    ~FrameResources() = default;

    bool Init(uint32_t numObjects, uint32_t numPasses);

    ComPtr<ID3D12CommandAllocator> CommandAllocator;

    UploadBuffer<PerObjectInfo> PerObjectBuffers;
    UploadBuffer<PerPassInfo> PerPassBuffers;
    

    uint64_t FenceValue = 0;
};


