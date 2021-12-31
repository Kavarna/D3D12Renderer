#pragma once


#include <Oblivion.h>
#include <ISingletone.h>
#include <Utils/UploadBuffer.h>
#include "../Vertex.h"
#include "PipelineManager.h"


class BatchRenderer
{
public:
    BatchRenderer() = default;
    ~BatchRenderer() = default;

public:
    bool Create(uint32_t maxVertices = 10000);

    void Begin();
    bool Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color);
    bool BoundingBox(const DirectX::BoundingBox& bb, const DirectX::XMFLOAT4& color);
    bool Rectangle(const DirectX::XMFLOAT2& leftBottom, const DirectX::XMFLOAT2& rightTop, const DirectX::XMFLOAT4& color);
    template <PipelineType pipeline, D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST>
    bool SetPipeline(ID3D12GraphicsCommandList* cmdList);
    /// <summary>
    /// It's recommended this is called at the end of a frame, because it resets the current vertex buffer
    /// </summary>
    /// <param name="cmdList">The command list used for rendering</param>
    /// <returns>true if the rendering could be submitted to the command list, false otherwise</returns>
    bool End(ID3D12GraphicsCommandList* cmdList);

private:
    bool Reconstruct(uint32_t maxVertices);
    bool ReconstructAndCopy(uint32_t maxVertices);

public:
    uint32_t mCurrentIndex = 0;
    uint32_t mMaxVertices;
    UploadBuffer<PositionColorVertex> mVertexBuffer;
    

};

template<PipelineType pipeline, D3D12_PRIMITIVE_TOPOLOGY topology>
inline bool BatchRenderer::SetPipeline(ID3D12GraphicsCommandList* cmdList)
{
    static ID3D12PipelineState* pipelineState = nullptr;
    static ID3D12RootSignature* rootSignature = nullptr;

    if (pipelineState == nullptr || rootSignature == nullptr)
    {
        auto pipelineSignatureResult = PipelineManager::Get()->GetPipelineAndRootSignature(pipeline);
        CHECK(pipelineSignatureResult.Valid(), false, "Unable to get pipeline for debug rendering");

        std::tie(pipelineState, rootSignature) = pipelineSignatureResult.Get();
    }

    cmdList->SetPipelineState(pipelineState);
    cmdList->SetGraphicsRootSignature(rootSignature);

    cmdList->IASetPrimitiveTopology(topology);
    return true;
}
