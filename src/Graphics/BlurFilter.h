#pragma once


#include <Oblivion.h>
#include "Utils/UploadBuffer.h"


class BlurFilter
{
    static constexpr const uint32_t GROUP_SIZE = 256;
public:
    static constexpr const uint32_t MAX_RADIUS = 5;

public:
    BlurFilter() = default;

public:
    bool Init(uint32_t width, uint32_t height, float sigma);

    bool Apply(ID3D12GraphicsCommandList *cmdList, uint32_t textureIndex,
               D3D12_RESOURCE_STATES finalState, uint32_t passCount);
    bool OnResize(uint32_t width, uint32_t height);

private:
    Result<std::vector<float>> GetGaussWeights(float sigma);

private:
    struct BlurInfo
    {
        int blurRadius;
        float weights[11];
    };

    uint32_t mIntermediaryTextureIndex;

    int32_t mBlurRadius;

    uint32_t mWidth;
    uint32_t mHeight;

    UploadBuffer<BlurInfo> mBlurInfoCB;
};
