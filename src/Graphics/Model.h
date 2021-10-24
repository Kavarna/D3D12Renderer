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
        Triangle = 0, Square
    };
    static constexpr const char *ModelTypeString[] =
    {
        "Triangle", "Square"
    };

public:
    Model() = default;
    Model(unsigned int maxDirtyFrames, unsigned int constantBufferIndex);
    ~Model() = default;

public:
    bool Create(ModelType type);
    bool Create(const std::string &path);
    bool Create(unsigned int maxDirtyFrames, unsigned int constantBufferIndex, ModelType type);
    bool Create(unsigned int maxDirtyFrames, unsigned int constantBufferIndex, const std::string &path);

    Result<uint32_t> AddInstance(const DirectX::XMMATRIX &worldMatrix = DirectX::XMMatrixIdentity(),
                                 const DirectX::XMMATRIX &texMatrix = DirectX::XMMatrixIdentity(),
                                 void* Context = nullptr);
    void ClearInstances();
    uint32_t GetInstanceCount() const;

    uint32_t PrepareInstances(std::function<bool(DirectX::XMMATRIX &, DirectX::XMMATRIX &)>,
                              std::unordered_map<void *, UploadBuffer<InstanceInfo>> &);
    uint32_t PrepareInstances(std::function<bool(DirectX::XMMATRIX &, DirectX::XMMATRIX &, void* Context)>,
                              std::unordered_map<void *, UploadBuffer<InstanceInfo>> &);
    void BindInstancesBuffer(ID3D12GraphicsCommandList *cmdList, uint32_t instanceCount,
                             const std::unordered_map<void *, UploadBuffer<InstanceInfo>> &instancesBuffer);

    void CloseAddingInstances();

    bool ShouldRender() const;
    void SetShouldRender(bool shouldRender);

public:
    static bool InitBuffers(ID3D12GraphicsCommandList *cmdList, ComPtr<ID3D12Resource> intermediaryResources[2]);
    static void Bind(ID3D12GraphicsCommandList* cmdList);
    static void Destroy();

public:
    uint32_t GetIndexCount() const;
    uint32_t GetVertexCount() const;
    uint32_t GetBaseVertexLocation() const;
    uint32_t GetStartIndexLocation() const;

    void SetMaterial(MaterialManager::Material *);
    MaterialManager::Material const *GetMaterial() const;

    const DirectX::XMMATRIX &__vectorcall GetWorld(unsigned int instanceID = 0) const;
    const DirectX::XMMATRIX &__vectorcall GetTexWorld(unsigned int instanceID = 0) const;

    void Identity(unsigned int instanceID = 0);
    void Translate(float x, float y, float z, unsigned int instanceID = 0);
    void RotateX(float theta, unsigned int instanceID = 0);
    void RotateY(float theta, unsigned int instanceID = 0);
    void RotateZ(float theta, unsigned int instanceID = 0);
    void Scale(float scaleFactor, unsigned int instanceID = 0);
    void Scale(float scaleFactorX, float scaleFactorY, float scaleFactorZ, unsigned int instanceID = 0);

private:
    bool ProcessNode(aiNode *node, const aiScene *scene, const std::string &path);
    bool ProcessMesh(uint32_t meshId, const aiScene *scene, const std::string &path);
    Result<std::tuple<std::string, MaterialConstants>> ProcessMaterialFromMesh(const aiMesh *mesh, const aiScene *scene);

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
    bool CreateSquare();

private:
    bool mCanAddInstances = true;
    
    struct ModelInstanceInfo
    {
        ModelInstanceInfo(const InstanceInfo &instanceInfo, void *Context = nullptr):
            instanceInfo(instanceInfo), Context(Context)
        {
        }

        InstanceInfo instanceInfo;
        void *Context;
    };

    std::vector<ModelInstanceInfo> mInstancesInfo;

    RenderParameters mInfo;
    MaterialManager::Material *mMaterial = nullptr;
    bool mShouldRender = false;
};
