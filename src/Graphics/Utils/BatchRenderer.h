#pragma once


#include <Oblivion.h>
#include <ISingletone.h>
#include <Utils/UploadBuffer.h>
#include "../Vertex.h"


class BatchRenderer
{
public:
    BatchRenderer() = default;
    ~BatchRenderer() = default;

public:
    bool Create(uint32_t maxVertices = 10000);

    void Begin();
    bool Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4 color);
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

