#include "BatchRenderer.h"
#include "../PipelineManager.h"


bool BatchRenderer::Create(uint32_t maxVertices)
{
    mMaxVertices = maxVertices;
    CHECKSIMPLE(Reconstruct(mMaxVertices));
    return true;
}

void BatchRenderer::Begin()
{
    mCurrentIndex = 0;
}

bool BatchRenderer::Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color)
{
    CHECK(mCurrentIndex < mMaxVertices, false,
        "Cannot add vertex (Position = {}, Color = {}) to batch renderer, cause it's already full (size = {}, capacity = {}).",
        position, color, mCurrentIndex, mMaxVertices);
    auto* vertex = mVertexBuffer.GetMappedMemory(mCurrentIndex);
    vertex->Position = position;
    vertex->Color = color;

    mCurrentIndex++;
    if (mCurrentIndex >= mMaxVertices)
    {
        CHECK(ReconstructAndCopy(mMaxVertices * 10), true,
            "Cannot reconstruct & copy after adding vertex (Position = {}, Color = {}). Future Vertex() calls will fail.", position, color);
    }
    return true;
}

bool BatchRenderer::BoundingBox(const DirectX::BoundingBox& bb, const DirectX::XMFLOAT4& color)
{
    const auto& center = bb.Center;
    const auto& extents = bb.Extents;

    DirectX::XMFLOAT3 backBottomLeft = {
        center.x - extents.x,
        center.y - extents.y,
        center.z - extents.z,
    };

    DirectX::XMFLOAT3 frontBottomLeft = {
        center.x - extents.x,
        center.y - extents.y,
        center.z + extents.z,
    };

    DirectX::XMFLOAT3 backTopLeft = {
        center.x - extents.x,
        center.y + extents.y,
        center.z - extents.z,
    };

    DirectX::XMFLOAT3 frontTopLeft = {
        center.x - extents.x,
        center.y + extents.y,
        center.z + extents.z,
    };

    DirectX::XMFLOAT3 backBottomRight = {
        center.x + extents.x,
        center.y - extents.y,
        center.z - extents.z,
    };

    DirectX::XMFLOAT3 frontBottomRight = {
        center.x + extents.x,
        center.y - extents.y,
        center.z + extents.z,
    };

    DirectX::XMFLOAT3 backTopRight = {
        center.x + extents.x,
        center.y + extents.y,
        center.z - extents.z,
    };

    DirectX::XMFLOAT3 frontTopRight = {
        center.x + extents.x,
        center.y + extents.y,
        center.z + extents.z,
    };

    // Lower base
    Vertex(backBottomLeft, color);
    Vertex(frontBottomLeft, color);

    Vertex(frontBottomLeft, color);
    Vertex(frontBottomRight, color);

    Vertex(frontBottomRight, color);
    Vertex(backBottomRight, color);

    Vertex(backBottomRight, color);
    Vertex(backBottomLeft, color);

    // Upper base
    Vertex(backTopLeft, color);
    Vertex(frontTopLeft, color);

    Vertex(frontTopLeft, color);
    Vertex(frontTopRight, color);

    Vertex(frontTopRight, color);
    Vertex(backTopRight, color);

    Vertex(backTopRight, color);
    Vertex(backTopLeft, color);

    // Connect the bases
    Vertex(backBottomLeft, color);
    Vertex(backTopLeft, color);

    Vertex(frontBottomLeft, color);
    Vertex(frontTopLeft, color);

    Vertex(backBottomRight, color);
    Vertex(backTopRight, color);

    Vertex(frontBottomRight, color);
    Vertex(frontTopRight, color);

    return true;
}

bool BatchRenderer::End(ID3D12GraphicsCommandList* cmdList)
{
    if (mCurrentIndex == 0)
    {
        // No rendering needed
        return true;
    }
    auto pipelineSignatureResult = PipelineManager::Get()->GetPipelineAndRootSignature(PipelineType::DebugPipeline);
    CHECK(pipelineSignatureResult.Valid(), false, "Unable to get pipeline for debug rendering");

    auto [pipeline, rootSignature] = pipelineSignatureResult.Get();
    
    cmdList->SetPipelineState(pipeline);
    cmdList->SetGraphicsRootSignature(rootSignature);

    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    vbView.BufferLocation = mVertexBuffer.GetGPUVirtualAddress();
    vbView.SizeInBytes = sizeof(PositionColorVertex) * mCurrentIndex;
    vbView.StrideInBytes = sizeof(PositionColorVertex);
    cmdList->IASetVertexBuffers(0, 1, &vbView);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    cmdList->DrawInstanced(mCurrentIndex, 1, 0, 0);
    
    return true;
}

bool BatchRenderer::Reconstruct(uint32_t maxVertices)
{
    CHECK(mVertexBuffer.Init(maxVertices), false, "Unable to initialize vertex buffer for batch renderer");
    return true;
}

bool BatchRenderer::ReconstructAndCopy(uint32_t maxVertices)
{
    CHECK(maxVertices >= mMaxVertices, true, "Batch renderer already at size {} or more ({})", maxVertices, mMaxVertices);
    mMaxVertices = maxVertices;
    if (mVertexBuffer.GetElementCount() > 0)
    {
        CHECKSIMPLE(Reconstruct(maxVertices));

        auto mappedMemory = mVertexBuffer.GetMappedMemory();
        std::vector<PositionColorVertex> vertices(mappedMemory, mappedMemory + mCurrentIndex);
        for (uint32_t i = 0; i < mCurrentIndex; ++i)
        {
            auto* destination = mVertexBuffer.GetMappedMemory(i);
            *destination = vertices[i];
        }
    }
    else
    {
        CHECKSIMPLE(Reconstruct(maxVertices));
    }


    return true;
}


