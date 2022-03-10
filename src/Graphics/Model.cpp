#include "Model.h"
#include "Utils/Utils.h"
#include "Direct3D.h"
#include "TextureManager.h"

std::unordered_set<Model*> Model::mModels;
std::vector<Model::Vertex> Model::mVertices;
std::vector<uint32_t> Model::mIndices;
UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC> Model::mRaytracingInstancingBuffer;

ComPtr<ID3D12Resource> Model::mVertexBuffer;
ComPtr<ID3D12Resource> Model::mIndexBuffer;

D3D12_VERTEX_BUFFER_VIEW Model::mVertexBufferView;
D3D12_INDEX_BUFFER_VIEW Model::mIndexBufferView;

std::unordered_map<std::string, Model::RenderParameters> Model::mModelsRenderParameters;
std::vector<Model::AccelerationStructureBuffers> Model::mBottomLevelAccelerationStructures;

Model::AccelerationStructureBuffers Model::mTopLevelBuffers;
uint32_t Model::mSizeTLAS = 0;

using namespace DirectX;

uint32_t Model::GetIndexCount() const
{
	return mInfo.IndexCount;
}

uint32_t Model::GetVertexCount() const
{
	return mInfo.VertexCount;
}

uint32_t Model::GetBaseVertexLocation() const
{
	return mInfo.BaseVertexLocation;
}

uint32_t Model::GetStartIndexLocation() const
{
	return mInfo.StartIndexLocation;
}

void Model::SetMaterial(MaterialManager::Material const *newMaterial)
{
	mInfo.Material = newMaterial;
}

MaterialManager::Material const *Model::GetMaterial() const
{
	return mInfo.Material;
}

const InstanceInfo& __vectorcall Model::GetInstanceInfo(unsigned int instanceID) const
{
    return mInstancesInfo[instanceID].instanceInfo;
}

InstanceInfo& __vectorcall Model::GetInstanceInfo(unsigned int instanceID)
{
	return mInstancesInfo[instanceID].instanceInfo;
}

void Model::Identity(unsigned int instanceID)
{
	mInstancesInfo[instanceID].instanceInfo.WorldMatrix = DirectX::XMMatrixIdentity();
	MarkUpdate();
}

void Model::Translate(float x, float y, float z, unsigned int instanceID)
{
	mInstancesInfo[instanceID].instanceInfo.WorldMatrix *= DirectX::XMMatrixTranslation(x, y, z);
	MarkUpdate();
}

void Model::RotateX(float theta, unsigned int instanceID)
{
	mInstancesInfo[instanceID].instanceInfo.WorldMatrix *= DirectX::XMMatrixRotationX(theta);
	MarkUpdate();
}

void Model::RotateY(float theta, unsigned int instanceID)
{
	mInstancesInfo[instanceID].instanceInfo.WorldMatrix *= DirectX::XMMatrixRotationY(theta);
	MarkUpdate();
}

void Model::RotateZ(float theta, unsigned int instanceID)
{
	mInstancesInfo[instanceID].instanceInfo.WorldMatrix *= DirectX::XMMatrixRotationZ(theta);
	MarkUpdate();
}

void Model::Scale(float scaleFactor, unsigned int instanceID)
{
	Scale(scaleFactor, scaleFactor, scaleFactor);
}

void Model::Scale(float scaleFactorX, float scaleFactorY, float scaleFactorZ, unsigned int instanceID)
{
	mInstancesInfo[instanceID].instanceInfo.WorldMatrix *= DirectX::XMMatrixScaling(scaleFactorX, scaleFactorY, scaleFactorZ);
	MarkUpdate();
}

bool Model::ProcessNode(aiNode *node, const aiScene *scene, const std::string &path)
{
	SHOWINFO("[Loading Model {}] Loading node with {} meshes and {} nodes", path, node->mNumMeshes, node->mNumChildren);
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		CHECK(ProcessMesh(node->mMeshes[i], scene, path),
			  false, "[Loading Model {}] Unable to load mesh at index {}", i);
	}
	SHOWINFO("[Loading Model {}] Done loading {} meshes", path, node->mNumMeshes);

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		CHECK(ProcessNode(node->mChildren[i], scene, path),
			  false, "[Loading Model {}] Unable to load node at index {}", i);
	}
	SHOWINFO("[Loading Model {}] Done loading {} nodes", path, node->mNumChildren);

	return true;
}

bool Model::ProcessMesh(uint32_t meshId, const aiScene *scene, const std::string &path)
{
	auto mesh = scene->mMeshes[meshId];
	std::string meshName = mesh->mName.C_Str();

	CHECK(mesh->HasTextureCoords(0), false, "[Loading Model {}] Mesh {} doesn't have texture coordinates", path, meshName);
	CHECK(mesh->HasNormals(), false, "[Loading Model {}] Mesh {} doesn't have normals", path, meshName);

	unsigned int index = 0;
	do
	{
		if (auto foundMeshIt = mModelsRenderParameters.find(meshName); foundMeshIt != mModelsRenderParameters.end())
		{
			auto renderParameters = (*foundMeshIt).second;

			// If needed, compare vertices & indices one by one
			if (mesh->mNumVertices == renderParameters.VertexCount &&
				(mesh->mNumFaces * 3) == renderParameters.IndexCount)
			{
				SHOWWARNING("[Loading Model {}] Mesh {} loaded once, using that version. "\
							"You should be using instancing instead of loading multiple times the same mesh",
							path, meshName);
				mInfo = mModelsRenderParameters[meshName];
				return true;
			}
			else
			{
				meshName += std::to_string(index++);
			}
		}
		else
			break;
	} while (true);

	auto materialInfoResult = ProcessMaterialFromMesh(mesh, scene);
	CHECK(materialInfoResult.Valid(), false, "[Loading Model {}] Cannot get material from mesh {}", path, meshName);
	auto [materialName, materialInfo] = materialInfoResult.Get();
	auto* material = MaterialManager::Get()->AddMaterial(
		GetMaxDirtyFrames(), materialName, materialInfo);
	CHECK(material != nullptr, false, "[Loading Model {}] Cannot add material {} to material manager ", path, materialName);

	std::vector<Vertex> vertices;
	vertices.reserve(mesh->mNumVertices);
	for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex currentVertex;
		currentVertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		currentVertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		currentVertex.TexCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

		vertices.push_back(std::move(currentVertex));
	}

	mBoundingBox.CreateFromPoints(mBoundingBox, vertices.size(), (DirectX::XMFLOAT3*)vertices.data(), sizeof(vertices[0]));
	mBoundingSphere.CreateFromPoints(mBoundingSphere, vertices.size(), (DirectX::XMFLOAT3*)vertices.data(), sizeof(vertices[0]));

	std::vector<uint32_t> indices;
	indices.reserve(mesh->mNumFaces * 3);
	for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
	{
		CHECK(mesh->mFaces[i].mNumIndices == 3, false,
			  "[Loading Model {}] In mesh {} found at face found invalid number of indices: expected 3, but found {}",
			  path, meshName, mesh->mFaces[i].mNumIndices);

		indices.emplace_back(mesh->mFaces[i].mIndices[0]);
		indices.emplace_back(mesh->mFaces[i].mIndices[1]);
		indices.emplace_back(mesh->mFaces[i].mIndices[2]);
	}

	mModelsRenderParameters[meshName].BaseVertexLocation = (uint32_t)mVertices.size();
	mModelsRenderParameters[meshName].StartIndexLocation = (uint32_t)mIndices.size();
	mModelsRenderParameters[meshName].VertexCount = (uint32_t)vertices.size();
	mModelsRenderParameters[meshName].IndexCount = (uint32_t)indices.size();
	mModelsRenderParameters[meshName].Material = material;

	mVertices.reserve(mVertices.size() + (uint32_t)vertices.size());
	std::move(std::begin(vertices), std::end(vertices), std::back_inserter(mVertices));

	mIndices.reserve(mIndices.size() + (uint32_t)indices.size());
	std::move(std::begin(indices), std::end(indices), std::back_inserter(mIndices));

	SHOWINFO("[Loading Model {}] Done loading mesh {}", path, meshName);

	mInfo = mModelsRenderParameters[meshName];
	return true;
}

Result<std::tuple<std::string, MaterialConstants>> Model::ProcessMaterialFromMesh(const aiMesh *mesh, const aiScene *scene)
{
	CHECK(mesh->mMaterialIndex < scene->mNumMaterials, std::nullopt,
		  "Invalid material index {}. It should be less than {}",
		  mesh->mMaterialIndex, scene->mNumMaterials);

	auto currentMaterial = scene->mMaterials[mesh->mMaterialIndex];
	std::string materialName = currentMaterial->GetName().C_Str();
	MaterialConstants materialInfo = {};
	aiColor3D diffuseColor, fresnel;
	float shininess;
	if (currentMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) != aiReturn::aiReturn_SUCCESS)
	{
		diffuseColor = { 0.0f, 0.0f, 0.0f };
		SHOWWARNING("Unable get diffuse color for material {}. Using black as default", materialName);
	}
	if (currentMaterial->Get(AI_MATKEY_SHININESS, shininess) != aiReturn::aiReturn_SUCCESS)
	{
		shininess = 0.0f;
		SHOWWARNING("Unable get shininess for material {}. Using default 0", materialName);
	}
	// Assimp cannot handle fresnel parameters, so we'll use specular color insted
	if (currentMaterial->Get(AI_MATKEY_COLOR_SPECULAR, fresnel) != aiReturn::aiReturn_SUCCESS)
	{
		fresnel = { 0.25f, 0.25f, 0.25f };
		SHOWWARNING("Unable get specular color for material {}. Using default ( 0.25, 0.25, 0.25 )", materialName);
	}
	aiString texturePath;
	materialInfo.textureIndex = -1;
	if (currentMaterial->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn::aiReturn_SUCCESS)
	{
		auto finalPath = std::filesystem::path("./Resources/");
		finalPath.append(texturePath.C_Str());
		finalPath = std::filesystem::absolute(finalPath);
		auto textureIndexResult = TextureManager::Get()->AddTexture(finalPath.string());
		CHECKSHOW(textureIndexResult.Valid(), "Cannot get texture index for texture {}", texturePath.C_Str());
		materialInfo.textureIndex = textureIndexResult.Get();
	}

	materialInfo.DiffuseAlbedo = { diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0f };
	materialInfo.MaterialTransform = DirectX::XMMatrixIdentity();
	materialInfo.Shininess = shininess / 1000.f;
	materialInfo.FresnelR0 = { fresnel.r, fresnel.g, fresnel.b };

	std::tuple<std::string, MaterialConstants> result = { materialName, materialInfo };
	return result;
}

bool Model::BuildBottomLevelAccelerationStructure(ID3D12GraphicsCommandList4* cmdList)
{
	D3D12_GPU_VIRTUAL_ADDRESS vbStartAddress = mVertexBuffer->GetGPUVirtualAddress() + sizeof(decltype(mVertices[0])) * mInfo.BaseVertexLocation;
	D3D12_GPU_VIRTUAL_ADDRESS ibStartAddress = mIndexBuffer->GetGPUVirtualAddress() + sizeof(decltype(mIndices[0])) * mInfo.StartIndexLocation;

	SHOWINFO("Vertex count = {}", mInfo.VertexCount);
	SHOWINFO("Index count = {}", mInfo.IndexCount);
	SHOWINFO("ibStartAddress = {}", ibStartAddress);
	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geomDesc.Triangles.VertexCount = mInfo.VertexCount;
	geomDesc.Triangles.VertexBuffer.StartAddress = vbStartAddress;
	geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geomDesc.Triangles.IndexBuffer = ibStartAddress;
	geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geomDesc.Triangles.IndexCount = mInfo.IndexCount;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	ComPtr<ID3D12Device5> device5;
	CHECK_HR(mDevice.As(&device5), false);
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
	SHOWINFO("D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO = ({}, {})", info.ResultDataMaxSizeInBytes, info.ScratchDataSizeInBytes);

	ComPtr<ID3D12Resource> intermediaryResource;
	std::tie(mBLASBuffers.scratchBuffer, intermediaryResource) = Utils::CreateDefaultBuffer(device5.Get(), cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr, (uint32_t)info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	mBLASBuffers.scratchBuffer->SetName(L"Model scratch buffer");
	CHECK(!intermediaryResource, false, "Got a valid intermediary pointer when expected an invalid one");
	
	std::tie(mBLASBuffers.resultBuffer, intermediaryResource) = Utils::CreateDefaultBuffer(device5.Get(), cmdList,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, (uint32_t)info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	CHECK(!intermediaryResource, false, "Got a valid intermediary pointer when expected an invalid one");
	mBLASBuffers.resultBuffer->SetName(L"Model result buffer");

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.ScratchAccelerationStructureData = mBLASBuffers.scratchBuffer->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = mBLASBuffers.resultBuffer->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = mBLASBuffers.resultBuffer.Get();
	cmdList->ResourceBarrier(1, &barrier);

	mBottomLevelAccelerationStructures.push_back(mBLASBuffers);

	return true;
}

Model::Model(unsigned int maxDirtyFrames, unsigned int constantBufferIndex) : 
	UpdateObject(maxDirtyFrames, constantBufferIndex), D3DObject()
{
}

bool Model::Create(ModelType type)
{
	CHECK(UpdateObject::Valid(), false, "Cannot create a model that was not properly initialized. "\
		  "Try calling Create(unsigned int, unsigned int, ModelType) instead of this");
	D3DObject::Init();
	mModels.insert(this);
	switch (type)
	{
		case Model::ModelType::Triangle:
			CHECK(CreateTriangle(), false, "Unable to create triangle");
			break;
		case Model::ModelType::Square:
			CHECK(CreateSquare(), false, "Unable to create square");
			break;
		// TODO: add more default shapes
		default:
			SHOWFATAL("Model type {} is not a valid model for Create(ModelType) function. Try using another specialized function", (int)type);
			return false;
	}

	CHECK(AddInstance(InstanceInfo()).Valid(), false, "Unable to add a basic instance");

    return true;
}

bool Model::Create(const std::string &path)
{
	CHECK(UpdateObject::Valid(), false, "Cannot create a model that was not properly initialized. "\
		  "Try calling Create(unsigned int, unsigned int, std::string) instead of this");
	D3DObject::Init();
	mModels.insert(this);

	Assimp::Importer importer;

	const aiScene *pScene = importer.ReadFile(path, aiProcess_Triangulate |
											  aiProcess_ConvertToLeftHanded);
	CHECK(pScene != nullptr, false, "Unable to load model located at path {}", path);

	CHECK(ProcessNode(pScene->mRootNode, pScene, path), false,
		  "Unable to process model located at path {}", path);
	CHECK(AddInstance(InstanceInfo()).Valid(), false, "Unable to add a basic instance");

	return true;
}

bool Model::Create(unsigned int maxDirtyFrames, unsigned int constantBufferIndex, ModelType type)
{
	UpdateObject::Init(maxDirtyFrames, constantBufferIndex);
	D3DObject::Init();
	mModels.insert(this);
	return Create(type);
}

bool Model::Create(unsigned int maxDirtyFrames, unsigned int constantBufferIndex, const std::string &path)
{
	UpdateObject::Init(maxDirtyFrames, constantBufferIndex);
	D3DObject::Init();
	mModels.insert(this);
	return Create(path);
}

Result<uint32_t> Model::AddInstance(const InstanceInfo& instanceInfo, void* Context)
{
	CHECK(mCanAddInstances, std::nullopt, "Model stopped for adding instances...");
	
	uint32_t start = (uint32_t)mInstancesInfo.size();

	mInstancesInfo.push_back(ModelInstanceInfo(instanceInfo, Context));

	return start;
}

void Model::ClearInstances()
{
	mInstancesInfo.clear();
}

uint32_t Model::PrepareInstances(std::function<bool(InstanceInfo&)> func,
								 std::unordered_map<uuids::uuid, UploadBuffer<InstanceInfo>> &instancesBuffer)
{
    if (auto instanceIt = instancesBuffer.find(mObjectUUID); instanceIt != instancesBuffer.end())
	{
		auto& instanceInfo = (*instanceIt).second;

		unsigned int bufferIndex = 0;
		for (auto &it : mInstancesInfo)
		{
			if (func(it.instanceInfo))
			{
				instanceInfo.CopyData(&it.instanceInfo, bufferIndex++);
			}
		}
		return bufferIndex;
	}
	else
	{
		SHOWWARNING("Attempting to prepare instances on a model that is not in the instances buffer");
		return 0;
	}
}

uint32_t Model::PrepareInstances(std::function<bool(InstanceInfo&, void* Context)> func,
								 std::unordered_map<uuids::uuid, UploadBuffer<InstanceInfo>> &instancesBuffer)
{
    if (auto instanceIt = instancesBuffer.find(mObjectUUID); instanceIt != instancesBuffer.end())
	{
		auto &instanceInfo = (*instanceIt).second;

		unsigned int bufferIndex = 0;
		for (auto &it : mInstancesInfo)
		{
			if (func(it.instanceInfo, it.Context))
			{
				instanceInfo.CopyData(&it.instanceInfo, bufferIndex++);
			}
		}
		return bufferIndex;
	}
	else
	{
		SHOWWARNING("Attempting to prepare instances on a model that is not in the instances buffer");
		return 0;
	}
}

uint32_t Model::PrepareInstances(std::unordered_map<uuids::uuid, UploadBuffer<InstanceInfo>>& instancesBuffer)
{
    if (auto instanceIt = instancesBuffer.find(mObjectUUID); instanceIt != instancesBuffer.end()) {
        auto& instanceInfo = (*instanceIt).second;

		unsigned int bufferIndex = 0;
        for (auto& it : mCurrentInstances) {
            instanceInfo.CopyData(&it->instanceInfo, bufferIndex++);
        }

		return bufferIndex;
    } else {
        SHOWWARNING("Attempting to prepare instances on a model that is not in the instances buffer");
        return 0;
    }
}

void Model::BindInstancesBuffer(ID3D12GraphicsCommandList *cmdList, uint32_t instanceCount,
								const std::unordered_map<void *, UploadBuffer<InstanceInfo>> &instancesBuffer)
{
	if (auto it = instancesBuffer.find((void *)this); it != instancesBuffer.end())
	{
		auto& buffer = (*it).second;
		D3D12_VERTEX_BUFFER_VIEW vbView = {};
		vbView.BufferLocation = buffer.GetGPUVirtualAddress();
		vbView.SizeInBytes = sizeof(InstanceInfo) * instanceCount;
		vbView.StrideInBytes = sizeof(InstanceInfo);
		cmdList->IASetVertexBuffers(1, 1, &vbView);
	}
	else
	{
		SHOWWARNING("Attempting to bind instances buffer on a model that is not in the instances buffer");
		return;
	}
}

uint32_t Model::GetInstanceCount() const
{
	return (uint32_t)mInstancesInfo.size();
}

void Model::CloseAddingInstances()
{
	mCanAddInstances = false;
}

const DirectX::BoundingBox& Model::GetBoundingBox() const
{
	return mBoundingBox;
}

const DirectX::BoundingSphere& Model::GetBoundingSphere() const
{
	return mBoundingSphere;
}

bool Model::InitBuffers(ID3D12GraphicsCommandList *cmdList, ComPtr<ID3D12Resource> intermediaryResources[2])
{
	CHECK(mVertices.size() > 0 && mIndices.size() > 0, false, "Unable to initialize model's buffers, because there are no vertices / indices");
	auto d3d = Direct3D::Get();
	auto device = d3d->GetD3D12Device();
	std::tie(mVertexBuffer, intermediaryResources[0]) =
		Utils::CreateDefaultBuffer(device.Get(), cmdList, D3D12_RESOURCE_STATE_GENERIC_READ,
								   mVertices.data(), (uint32_t)sizeof(Vertex) * (uint32_t)mVertices.size());
	CHECK((mVertexBuffer != nullptr) && (intermediaryResources[0] != nullptr), false,
		  "Unable to create Vertex Buffer with {} vertices", mVertices.size());

	std::tie(mIndexBuffer, intermediaryResources[1]) =
		Utils::CreateDefaultBuffer(device.Get(), cmdList, D3D12_RESOURCE_STATE_GENERIC_READ,
								   mIndices.data(), (uint32_t)sizeof(uint32_t) * (uint32_t)mIndices.size());
	CHECK((mIndexBuffer != nullptr) && (intermediaryResources[1] != nullptr), false,
		  "Unable to create Index Buffer with {} vertices", mIndices.size());

	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = (uint32_t)sizeof(Vertex) * (uint32_t)mVertices.size();
	mVertexBufferView.StrideInBytes = (uint32_t)sizeof(Vertex);

	mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mIndexBufferView.SizeInBytes = (uint32_t)sizeof(uint32_t) * (uint32_t)mIndices.size();

	return true;
}

bool Model::BuildTopLevelAccelerationStructure(ID3D12GraphicsCommandList4* cmdList)
{
	uint32_t countInstances = 0;
	struct InstanceInfo
	{
		ComPtr<ID3D12Resource> bottomLevelStructure;
		DirectX::XMMATRIX world;
	};
	// std::unordered_map<uint32_t, InstanceInfo> instancesInfo;
	std::vector<InstanceInfo> instancesInfo;
	for (const auto& model : mModels)
	{
		// countInstances += model->mInstancesInfo.size();
		for (uint32_t i = 0; i < model->mInstancesInfo.size(); ++i)
		{
			auto& currentInstance = instancesInfo.emplace_back();
			currentInstance.bottomLevelStructure = model->mBLASBuffers.resultBuffer;
			currentInstance.world = model->mInstancesInfo[i].instanceInfo.WorldMatrix;
		}
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.NumDescs = (uint32_t)instancesInfo.size();

	auto device = Direct3D::Get()->GetD3D12Device();
	ComPtr<ID3D12Device5> device5;
	CHECK_HR(device.As(&device5), false);
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	ComPtr<ID3D12Resource> intermediaryResource;
	std::tie(mTopLevelBuffers.scratchBuffer, intermediaryResource) = Utils::CreateDefaultBuffer(device5.Get(), cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr, (uint32_t)info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	CHECK(!intermediaryResource, false, "Got a valid intermediary pointer when expected an invalid one");

	std::tie(mTopLevelBuffers.resultBuffer, intermediaryResource) = Utils::CreateDefaultBuffer(device5.Get(), cmdList, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr, (uint32_t)info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	CHECK(!intermediaryResource, false, "Got a valid intermediary pointer when expected an invalid one");
	mSizeTLAS = (uint32_t)info.ResultDataMaxSizeInBytes;

	CHECK(mRaytracingInstancingBuffer.Init((uint32_t)instancesInfo.size()), false, "Cannot create instance buffer for TLAS");

	for (uint32_t i = 0; i < (uint32_t)instancesInfo.size(); ++i)
	{
		D3D12_RAYTRACING_INSTANCE_DESC* instanceDesc = mRaytracingInstancingBuffer.GetMappedMemory(i);

		instanceDesc->AccelerationStructure = instancesInfo[i].bottomLevelStructure->GetGPUVirtualAddress();
		instanceDesc->InstanceID = i;
		instanceDesc->InstanceMask = 0xff;
		instanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instanceDesc->InstanceContributionToHitGroupIndex = i;
		DirectX::XMFLOAT3X4 matrix = {};
		DirectX::XMMATRIX worldMatrix = instancesInfo[i].world;
		DirectX::XMStoreFloat3x4(&matrix, worldMatrix);
		memcpy(instanceDesc->Transform, &matrix, sizeof(instanceDesc->Transform));
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = mRaytracingInstancingBuffer.GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = mTopLevelBuffers.resultBuffer->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = mTopLevelBuffers.scratchBuffer->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = mTopLevelBuffers.resultBuffer.Get();
	cmdList->ResourceBarrier(1, &barrier);

	return true;
}

void Model::Bind(ID3D12GraphicsCommandList *cmdList)
{
	cmdList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	cmdList->IASetIndexBuffer(&mIndexBufferView);
}

void Model::Destroy()
{
	mVertexBuffer.Reset();
	mIndexBuffer.Reset();

	mTopLevelBuffers.resultBuffer.Reset();
	mTopLevelBuffers.scratchBuffer.Reset();

	mBottomLevelAccelerationStructures.clear();
}

uint32_t Model::GetTotalInstanceCount()
{
	uint32_t countInstances = 0;
	for (const auto& model : mModels)
	{
		 countInstances += model->mInstancesInfo.size();
	}
	return countInstances;
}

ComPtr<ID3D12Resource> Model::GetTLASBuffer()
{
	return mTopLevelBuffers.resultBuffer;
}

void Model::ResetCurrentInstances()
{
	this->mCurrentInstances.clear();
}

void Model::AddCurrentInstance(uint32_t index)
{
	this->mCurrentInstances.push_back(&mInstancesInfo[index]);
}

bool Model::CreateTriangle()
{
	if (auto triangleIt = mModelsRenderParameters.find("Triangle"); triangleIt != mModelsRenderParameters.end())
	{
		mInfo = (*triangleIt).second;
		SHOWWARNING("Triangle was already created once. You should be using instancing instead of creating multiple models");
		return true;
	}

	Vertex vertices[] = {
		Vertex(XMFLOAT3( 0.0f,  1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.5f, 0.0f)),
		Vertex(XMFLOAT3(+1.0f, -1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f)),
		Vertex(XMFLOAT3(-1.0f, -1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f)),
	};
	unsigned int indices[] = {
		0, 1, 2
	};

	mModelsRenderParameters["Triangle"].BaseVertexLocation = (uint32_t)mVertices.size();
	mModelsRenderParameters["Triangle"].StartIndexLocation = (uint32_t)mIndices.size();
	mModelsRenderParameters["Triangle"].VertexCount = ARRAYSIZE(vertices);
	mModelsRenderParameters["Triangle"].IndexCount = ARRAYSIZE(indices);

	mVertices.reserve(mVertices.size() + ARRAYSIZE(vertices));
	std::move(std::begin(vertices), std::end(vertices), std::back_inserter(mVertices));

	mIndices.reserve(mIndices.size() + ARRAYSIZE(indices));
	std::move(std::begin(indices), std::end(indices), std::back_inserter(mIndices));

	mInfo = mModelsRenderParameters["Triangle"];

	return true;
}

bool Model::CreateSquare()
{
	if (auto squareIt = mModelsRenderParameters.find("Square"); squareIt != mModelsRenderParameters.end())
	{
		mInfo = (*squareIt).second;
		SHOWWARNING("Square was already created once. You should be using instancing instead of creating multiple models");
		return true;
	}

	Vertex vertices[] = {
		Vertex(XMFLOAT3(-1.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)),
		Vertex(XMFLOAT3(+1.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)),
		Vertex(XMFLOAT3(+1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f)),
		Vertex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f)),
	};
	unsigned int indices[] = {
		0, 1, 2,
		0, 2, 3
	};

	mModelsRenderParameters["Square"].BaseVertexLocation = (uint32_t)mVertices.size();
	mModelsRenderParameters["Square"].StartIndexLocation = (uint32_t)mIndices.size();
	mModelsRenderParameters["Square"].VertexCount = ARRAYSIZE(vertices);
	mModelsRenderParameters["Square"].IndexCount = ARRAYSIZE(indices);

	mVertices.reserve(mVertices.size() + ARRAYSIZE(vertices));
	std::move(std::begin(vertices), std::end(vertices), std::back_inserter(mVertices));

	mIndices.reserve(mIndices.size() + ARRAYSIZE(indices));
	std::move(std::begin(indices), std::end(indices), std::back_inserter(mIndices));

	mInfo = mModelsRenderParameters["Square"];

	return true;
}

bool Model::CreateGrid(const GridInitializationInfo &initInfo)
{
	std::string name = fmt::format("Grid_{}_{}_{}_{}", initInfo.N, initInfo.M, initInfo.width, initInfo.depth);
	if (auto gridIt = mModelsRenderParameters.find(name); gridIt != mModelsRenderParameters.end())
	{
		mInfo = (*gridIt).second;
		SHOWWARNING("Grid was already created once. You should be using instancing instead of creating multiple models");
		return true;
	}

	float halfWidth = 0.5f * initInfo.width;
	float halfDepth = 0.5f * initInfo.depth;

	float dx = initInfo.width / (initInfo.N - 1);
	float dz = initInfo.depth / (initInfo.M - 1);

	float du = 1.0f / (initInfo.N - 1);
	float dv = 1.0f / (initInfo.M - 1);

	uint32_t vertexCount = initInfo.M * initInfo.N;
	uint32_t faceCount = (initInfo.M - 1) * (initInfo.N - 1);

	std::vector<Vertex> vertices(vertexCount);
	std::vector<uint32_t> indices(faceCount * 4);

	for (uint32_t i = 0; i < initInfo.M; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32_t j = 0; j < initInfo.N; ++j)
		{
			float x = -halfWidth + j * dx;

			vertices[i * initInfo.N + j].Position = DirectX::XMFLOAT3(x, 0.0f, z);
			vertices[i * initInfo.N + j].Normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

			vertices[i * initInfo.N + j].TexCoord.x = i * du;
			vertices[i * initInfo.N + j].TexCoord.y = j * dv;
		}
	}

	uint32_t k = 0;
	for (uint32_t i = 0; i < initInfo.M - 1; ++i)
	{
		for (uint32_t j = 0; j < initInfo.N - 1; ++j)
		{
			indices[k] = i * initInfo.N + j;
			indices[k + 1] = i * initInfo.N + j + 1;
			indices[k + 2] = (i + 1) * initInfo.N + j;
			indices[k + 3] = (i + 1) * initInfo.N + j + 1;
			k += 4;

			/*indices[k + 3] = i * initInfo.N + j;
			indices[k + 4] = (i + 1) * initInfo.N + j + 1;
			indices[k + 5] = (i + 1) * initInfo.N + j;
			
			k += 6;*/
			
		}
	}

	mModelsRenderParameters[name].BaseVertexLocation = (uint32_t)mVertices.size();
	mModelsRenderParameters[name].StartIndexLocation = (uint32_t)mIndices.size();
	mModelsRenderParameters[name].VertexCount = (uint32_t)vertices.size();
	mModelsRenderParameters[name].IndexCount = (uint32_t)indices.size();

	mVertices.reserve(mVertices.size() + vertices.size());
	std::move(std::begin(vertices), std::end(vertices), std::back_inserter(mVertices));

	mIndices.reserve(mIndices.size() + indices.size());
	std::move(std::begin(indices), std::end(indices), std::back_inserter(mIndices));

	mInfo = mModelsRenderParameters[name];

	return true;
}

template <typename InitializationInfo>
bool Model::CreatePrimitive(const InitializationInfo & initInfo)
{

	if constexpr (std::is_same_v<InitializationInfo, GridInitializationInfo>)
	{
		AddInstance(InstanceInfo());
		return CreateGrid(initInfo);
	}
	else
	{
		static_assert(false, "Invalid type for creating a model");
	}
	return true;
}

template bool Model::CreatePrimitive(const Model::GridInitializationInfo &);
