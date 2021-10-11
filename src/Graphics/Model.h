#pragma once



#include "Oblivion.h"
#include "Vertex.h"
#include "Utils/UpdateObject.h"
#include "MaterialManager.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>
#include <assimp\mesh.h>

class Model : public UpdateObject
{
    using Vertex = PositionNormalTexCoordVertex;
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

    MaterialManager::Material const *GetMaterial() const;

    const DirectX::XMMATRIX &__vectorcall GetWorld() const;
    const DirectX::XMMATRIX &__vectorcall GetTexWorld() const;

    void Identity();
    void Translate(float x, float y, float z);
    void RotateX(float theta);
    void RotateY(float theta);
    void RotateZ(float theta);
    void Scale(float scaleFactor);
    void Scale(float scaleFactorX, float scaleFactorY, float scaleFactorZ);

private:
    bool ProcessNode(aiNode *node, const aiScene *scene, const std::string &path);
    bool ProcessMesh(uint32_t meshId, const aiScene *scene, const std::string &path);
    Result<std::tuple<std::string, MaterialConstants, uint64_t>> ProcessMaterialFromMesh(const aiMesh *mesh, const aiScene *scene);

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
    DirectX::XMMATRIX mWorld = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX mTexWorld = DirectX::XMMatrixIdentity();
    RenderParameters mInfo;
    MaterialManager::Material *mMaterial;

};
