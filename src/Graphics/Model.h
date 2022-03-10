#pragma once



#include "Oblivion.h"
#include "Vertex.h"
#include "Utils/UpdateObject.h"
#include "MaterialManager.h"
#include "Utils/UploadBuffer.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>
#include <assimp\mesh.h>

class Model : public UpdateObject, public D3DObject
{
    using Vertex = PositionNormalTexCoordVertex;
public:
    enum class ModelType
    {
        Triangle = 0, Square, Grid
    };
    static constexpr const char* ModelTypeString[] =
    {
        "Triangle", "Square", "Grid"
    };

    struct GridInitializationInfo
    {

        uint32_t N;
        uint32_t M;
        float width;
        float depth;
    };

public:
    Model() = default;
    Model(unsigned int maxDirtyFrames, unsigned int constantBufferIndex);
    ~Model() = default;

public:
    bool Create(ModelType type);
    template <typename InitializationInfo>
    bool CreatePrimitive(const InitializationInfo&);
    bool Create(const std::string& path);
    bool Create(unsigned int maxDirtyFrames, unsigned int constantBufferIndex, ModelType type);
    bool Create(unsigned int maxDirtyFrames, unsigned int constantBufferIndex, const std::string& path);

    bool BuildBottomLevelAccelerationStructure(ID3D12GraphicsCommandList4* cmdList);

    Result<uint32_t> AddInstance(const InstanceInfo& info,
        void* Context = nullptr);
    void ClearInstances();
    uint32_t GetInstanceCount() const;

    uint32_t PrepareInstances(std::function<bool(InstanceInfo&)>,
        std::unordered_map<uuids::uuid, UploadBuffer<InstanceInfo>>&);
    uint32_t PrepareInstances(std::function<bool(InstanceInfo&, void* Context)>,
        std::unordered_map<uuids::uuid, UploadBuffer<InstanceInfo>>&);
    uint32_t PrepareInstances(std::unordered_map<uuids::uuid, UploadBuffer<InstanceInfo>>&);
    void BindInstancesBuffer(ID3D12GraphicsCommandList* cmdList, uint32_t instanceCount,
        const std::unordered_map<void*, UploadBuffer<InstanceInfo>>& instancesBuffer);

    void CloseAddingInstances();

    const DirectX::BoundingBox& GetBoundingBox() const;
    const DirectX::BoundingSphere& GetBoundingSphere() const;

public:
    static bool InitBuffers(ID3D12GraphicsCommandList* cmdList, ComPtr<ID3D12Resource> intermediaryResources[2]);
    static bool BuildTopLevelAccelerationStructure(ID3D12GraphicsCommandList4* cmdList);
    static void Bind(ID3D12GraphicsCommandList* cmdList);
    static void Destroy();

    static uint32_t GetTotalInstanceCount();

    static ComPtr<ID3D12Resource> GetTLASBuffer();
public:
    void ResetCurrentInstances();
    void AddCurrentInstance(uint32_t index);

public:
    uint32_t GetIndexCount() const;
    uint32_t GetVertexCount() const;
    uint32_t GetBaseVertexLocation() const;
    uint32_t GetStartIndexLocation() const;

    void SetMaterial(const MaterialManager::Material*);
    MaterialManager::Material const* GetMaterial() const;

    const InstanceInfo& __vectorcall GetInstanceInfo(unsigned int instanceID = 0) const;
    InstanceInfo& __vectorcall GetInstanceInfo(unsigned int instanceID = 0);

    void Identity(unsigned int instanceID = 0);
    void Translate(float x, float y, float z, unsigned int instanceID = 0);
    void RotateX(float theta, unsigned int instanceID = 0);
    void RotateY(float theta, unsigned int instanceID = 0);
    void RotateZ(float theta, unsigned int instanceID = 0);
    void Scale(float scaleFactor, unsigned int instanceID = 0);
    void Scale(float scaleFactorX, float scaleFactorY, float scaleFactorZ, unsigned int instanceID = 0);

private:
    bool ProcessNode(aiNode* node, const aiScene* scene, const std::string& path);
    bool ProcessMesh(uint32_t meshId, const aiScene* scene, const std::string& path);
    Result<std::tuple<std::string, MaterialConstants>> ProcessMaterialFromMesh(const aiMesh* mesh, const aiScene* scene);

private:
    static std::unordered_set<Model*> mModels;

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
        MaterialManager::Material const* Material = nullptr;
    };
    static std::unordered_map<std::string, RenderParameters> mModelsRenderParameters;

    struct AccelerationStructureBuffers
    {
        ComPtr<ID3D12Resource> scratchBuffer;
        ComPtr<ID3D12Resource> resultBuffer;
    };
    static std::vector<AccelerationStructureBuffers> mBottomLevelAccelerationStructures;

    static AccelerationStructureBuffers mTopLevelBuffers;
    static UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC> mRaytracingInstancingBuffer;
    static uint32_t mSizeTLAS;

private:
    bool CreateTriangle();
    bool CreateSquare();
    bool CreateGrid(const GridInitializationInfo&);

private:
    bool mCanAddInstances = true;

    struct ModelInstanceInfo
    {
        ModelInstanceInfo(const InstanceInfo& instanceInfo, void* Context = nullptr) :
            instanceInfo(instanceInfo), Context(Context)
        {
        }

        InstanceInfo instanceInfo;
        void* Context;
    };

    std::vector<ModelInstanceInfo> mInstancesInfo;
    std::vector<ModelInstanceInfo*> mCurrentInstances;

    DirectX::BoundingBox mBoundingBox;
    DirectX::BoundingSphere mBoundingSphere;

    AccelerationStructureBuffers mBLASBuffers;

    RenderParameters mInfo;
};
