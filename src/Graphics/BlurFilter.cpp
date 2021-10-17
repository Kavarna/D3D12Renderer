#include "BlurFilter.h"

#include "TextureManager.h"
#include "PipelineManager.h"
#include "FrameResources.h"

bool BlurFilter::Init(uint32_t width, uint32_t height, float sigma)
{
    auto textureManager = TextureManager::Get();
    
    CHECK(mBlurInfoCB.Init(1, true), false, "Unable to initialize blur info cb");

    CD3DX12_RESOURCE_DESC backbufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        Direct3D::kBackbufferFormat, width / FrameResources::kBlurScale, height / FrameResources::kBlurScale, 1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    CD3DX12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    auto indexResult = TextureManager::Get()->AddTexture(backbufferDesc, defaultHeap, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                         D3D12_HEAP_FLAG_NONE);
    CHECK(indexResult.Valid(), false, "Cannot created render target views");
    mIntermediaryTextureIndex = indexResult.Get();

    mWidth = width; mHeight = height;
    auto gaussWeightsResult = GetGaussWeights(sigma);
    CHECK(gaussWeightsResult.Valid(), false, "Unable to initialize Gauss Weights");
    auto gaussWeights = gaussWeightsResult.Get();

    auto mappedBlurInfo = mBlurInfoCB.GetMappedMemory();
    mappedBlurInfo->blurRadius = mBlurRadius;
    memcpy(mappedBlurInfo->weights, gaussWeights.data(), sizeof(gaussWeights[0]) * gaussWeights.size());

    return true;
}

bool BlurFilter::Apply(ID3D12GraphicsCommandList *cmdList, uint32_t textureIndex,
                       D3D12_RESOURCE_STATES finalState, uint32_t passCount)
{
    auto d3d = Direct3D::Get();
    auto textureManager = TextureManager::Get();
    auto pipelineManager = PipelineManager::Get();

    auto pipelineResult = PipelineManager::Get()->GetPipeline(PipelineType::HorizontalBlur);
    CHECK(pipelineResult.Valid(), false, "Unable to retrieve horizontal pipeline and root signature");
    auto [horizontalPipeline, horizontalRootSignature] = pipelineResult.Get();

    pipelineResult = PipelineManager::Get()->GetPipeline(PipelineType::VerticalBlur);
    CHECK(pipelineResult.Valid(), false, "Unable to retrieve vertical pipeline and root signature");
    auto [verticalPipeline, verticalRootSignature] = pipelineResult.Get();

    CHECK(horizontalRootSignature == verticalRootSignature, false, "Horizontal & vertical root signature should be the same");

    cmdList->SetComputeRootSignature(horizontalRootSignature);

    UINT groupsX = (UINT)ceilf((float)mWidth / (float)GROUP_SIZE);
    UINT groupsY = (UINT)ceilf((float)mHeight / (float)GROUP_SIZE);
    
    cmdList->SetDescriptorHeaps(1, textureManager->GetSrvUavDescriptorHeap().GetAddressOf());
    cmdList->SetComputeRootConstantBufferView(2, mBlurInfoCB.GetGPUVirtualAddress());
    

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle, intermediaryUavHandle, textureUavHandle, intermediarySrvHandle;

    auto gpuHandleResult = textureManager->GetGPUDescriptorSrvHandleForTextureIndex(textureIndex);
    CHECK(gpuHandleResult.Valid(), false, "Unable to get SRV handle for texture index {}", textureIndex);
    textureSrvHandle = gpuHandleResult.Get();
    
    gpuHandleResult = textureManager->GetGPUDescriptorSrvHandleForTextureIndex(mIntermediaryTextureIndex);
    CHECK(gpuHandleResult.Valid(), false, "Unable to get UAV handle for texture index {}", mIntermediaryTextureIndex);
    intermediaryUavHandle = gpuHandleResult.Get();

    gpuHandleResult = textureManager->GetGPUDescriptorSrvHandleForTextureIndex(textureIndex);
    CHECK(gpuHandleResult.Valid(), false, "Unable to get UAV handle for texture index {}", textureIndex);
    textureUavHandle = gpuHandleResult.Get();

    gpuHandleResult = textureManager->GetGPUDescriptorSrvHandleForTextureIndex(mIntermediaryTextureIndex);
    CHECK(gpuHandleResult.Valid(), false, "Unable to get SRV handle for texture index {}", mIntermediaryTextureIndex);
    intermediarySrvHandle = gpuHandleResult.Get();

    for (uint32_t i = 0; i < passCount; ++i)
    {
        // Horizontal pass
        cmdList->SetPipelineState(horizontalPipeline);

        textureManager->Transition(cmdList, textureIndex, D3D12_RESOURCE_STATE_GENERIC_READ);
        textureManager->Transition(cmdList, mIntermediaryTextureIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        cmdList->SetComputeRootDescriptorTable(0, textureSrvHandle);
        cmdList->SetComputeRootDescriptorTable(1, intermediaryUavHandle);

        cmdList->Dispatch(groupsX, mHeight, 1);


        // Vertical pass
        cmdList->SetPipelineState(verticalPipeline);

        textureManager->Transition(cmdList, mIntermediaryTextureIndex, D3D12_RESOURCE_STATE_GENERIC_READ);
        textureManager->Transition(cmdList, textureIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        cmdList->SetComputeRootDescriptorTable(0, intermediarySrvHandle);
        cmdList->SetComputeRootDescriptorTable(1, textureUavHandle);

        cmdList->Dispatch(mWidth, groupsY, 1);
    }

    textureManager->Transition(cmdList, textureIndex, finalState);
    return true;
}

Result<std::vector<float>> BlurFilter::GetGaussWeights(float sigma)
{
    float twoSigmaSq = 2 * sigma * sigma;

    mBlurRadius = (int32_t)ceil(2.0f * sigma);
    CHECK(mBlurRadius <= MAX_RADIUS, std::nullopt, "Radius too big {}. Try using a smaller sigma", mBlurRadius);

    std::vector<float> weights;
    weights.resize(2 * mBlurRadius + 1);

    float weightSum = 0.0f;

    for (int32_t i = -mBlurRadius; i <= mBlurRadius; ++i)
    {
        float x = (float)i;

        weights[i + mBlurRadius] = expf(-x * x / twoSigmaSq);

        weightSum += weights[i + mBlurRadius];
    }

    for (auto &it : weights)
    {
        it /= weightSum;
    }

    return weights;
}


