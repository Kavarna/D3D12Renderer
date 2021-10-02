#pragma once



#include "Oblivion.h"
#include "Vertex.h"
#include "Utils/UpdateObject.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

class Model : public UpdateObject
{
    using Vertex = PositionColorVertex;
public:
    enum class ModelType
    {
        Triangle = 0
    };
    static constexpr const char *ModelTypeString[] =
    {
        "Triangle"
    };

public:
    Model() = delete;
    Model(unsigned int maxDirtyFrames, unsigned int constantBufferIndex);
    ~Model() = default;

public:
    bool Create(ModelType type);
    bool Create(const std::string &path);

public:
    static bool InitBuffers(ID3D12GraphicsCommandList *cmdList, ComPtr<ID3D12Resource> intermediaryResources[2]);
    static void Bind(ID3D12GraphicsCommandList* cmdList);
    static void Destroy();

public:
    uint32_t GetIndexCount() const;
    uint32_t GetVertexCount() const;
    uint32_t GetBaseVertexLocation() const;
    uint32_t GetStartIndexLocation() const;

    const DirectX::XMMATRIX &__vectorcall GetWorld() const;

private:
    bool ProcessNode(aiNode *node, const aiScene *scene, const std::string &path);
    bool ProcessMesh(uint32_t meshId, const aiScene *scene, const std::string &path);

private:
    struct ModelInfo
    {
        uint32_t mVertexCount;
        uint32_t mIndexCount;
        uint32_t mBaseVertexLocation;
        uint32_t mStartIndexLocation;
    };

private:
    static std::vector<Vertex> mVertices;
    static std::vector<uint32_t> mIndices;

    static ComPtr<ID3D12Resource> mVertexBuffer;
    static ComPtr<ID3D12Resource> mIndexBuffer;

    static D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    static D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

    struct RenderParameters
    {
        uint32_t IndexCount;
        uint32_t VertexCount;
        uint32_t BaseVertexLocation;
        uint32_t StartIndexLocation;
    };

    static std::unordered_map<std::string, RenderParameters> mModelsRenderParameters;


private:
    bool CreateTriangle();

private:
    DirectX::XMMATRIX mWorld;
    RenderParameters mInfo;
};
