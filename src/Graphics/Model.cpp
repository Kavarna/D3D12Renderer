#include "Model.h"
#include "Utils/Utils.h"
#include "Direct3D.h"
#include "TextureManager.h"

std::vector<Model::Vertex> Model::mVertices;
std::vector<uint32_t> Model::mIndices;

ComPtr<ID3D12Resource> Model::mVertexBuffer;
ComPtr<ID3D12Resource> Model::mIndexBuffer;

D3D12_VERTEX_BUFFER_VIEW Model::mVertexBufferView;
D3D12_INDEX_BUFFER_VIEW Model::mIndexBufferView;

std::unordered_map<std::string, Model::RenderParameters> Model::mModelsRenderParameters;

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

void Model::SetMaterial(MaterialManager::Material * newMaterial)
{
	mMaterial = newMaterial;
}

MaterialManager::Material const *Model::GetMaterial() const
{
	return mMaterial;
}

const DirectX::XMMATRIX &__vectorcall Model::GetWorld(unsigned int instanceID) const
{
	return mInstancesInfo[instanceID].instanceInfo.WorldMatrix;
}

const DirectX::XMMATRIX &__vectorcall Model::GetTexWorld(unsigned int instanceID) const
{
	return mInstancesInfo[instanceID].instanceInfo.TexWorld;
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
	mMaterial = MaterialManager::Get()->AddMaterial(
		GetMaxDirtyFrames(), materialName, materialInfo);
	CHECK(mMaterial != nullptr, false, "[Loading Model {}] Cannot add material {} to material manager ", path, materialName);

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
		SHOWWARNING("Unable get specular color for material {}. Using default { 0.25, 0.25, 0.25 }", materialName);
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

Model::Model(unsigned int maxDirtyFrames, unsigned int constantBufferIndex) : 
	UpdateObject(maxDirtyFrames, constantBufferIndex)
{
}

bool Model::Create(ModelType type)
{
	CHECK(UpdateObject::Valid(), false, "Cannot create a model that was not properly initialized. "\
		  "Try calling Create(unsigned int, unsigned int, ModelType) instead of this");
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
	AddInstance();

    return true;
}

bool Model::Create(const std::string &path)
{
	CHECK(UpdateObject::Valid(), false, "Cannot create a model that was not properly initialized. "\
		  "Try calling Create(unsigned int, unsigned int, std::string) instead of this");
	Assimp::Importer importer;

	const aiScene *pScene = importer.ReadFile(path, aiProcess_Triangulate |
											  aiProcess_ConvertToLeftHanded);


	CHECK(ProcessNode(pScene->mRootNode, pScene, path), false,
		  "Unable to load model located at path {}", path);
	AddInstance();

	return true;
}

bool Model::Create(unsigned int maxDirtyFrames, unsigned int constantBufferIndex, ModelType type)
{
	UpdateObject::Init(maxDirtyFrames, constantBufferIndex);
	return Create(type);
}

bool Model::Create(unsigned int maxDirtyFrames, unsigned int constantBufferIndex, const std::string &path)
{
	UpdateObject::Init(maxDirtyFrames, constantBufferIndex);
	return Create(path);
}

Result<uint32_t> Model::AddInstance(const DirectX::XMMATRIX &worldMatrix, const DirectX::XMMATRIX &texMatrix, void* Context)
{
	CHECK(mCanAddInstances, std::nullopt, "Model stopped for adding instances...");
	
	uint32_t start = (uint32_t)mInstancesInfo.size();

	mInstancesInfo.push_back(ModelInstanceInfo({ worldMatrix, texMatrix }, Context));

	return start;
}

void Model::ClearInstances()
{
	mInstancesInfo.clear();
}

uint32_t Model::PrepareInstances(std::function<bool(DirectX::XMMATRIX &, DirectX::XMMATRIX &)> func,
								 std::unordered_map<void *, UploadBuffer<InstanceInfo>> &instancesBuffer)
{
	if (auto instanceIt = instancesBuffer.find((void *)this); instanceIt != instancesBuffer.end())
	{
		auto& instanceInfo = (*instanceIt).second;

		unsigned int bufferIndex = 0;
		for (auto &it : mInstancesInfo)
		{
			if (func(it.instanceInfo.WorldMatrix, it.instanceInfo.TexWorld))
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

uint32_t Model::PrepareInstances(std::function<bool(DirectX::XMMATRIX &, DirectX::XMMATRIX &, void *Context)> func,
								 std::unordered_map<void *, UploadBuffer<InstanceInfo>> &instancesBuffer)
{
	if (auto instanceIt = instancesBuffer.find((void *)this); instanceIt != instancesBuffer.end())
	{
		auto &instanceInfo = (*instanceIt).second;

		unsigned int bufferIndex = 0;
		for (auto &it : mInstancesInfo)
		{
			if (func(it.instanceInfo.WorldMatrix, it.instanceInfo.TexWorld, it.Context))
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

bool Model::ShouldRender() const
{
	return mShouldRender;
}

void Model::SetShouldRender(bool shouldRender)
{
	mShouldRender = shouldRender;
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

void Model::Bind(ID3D12GraphicsCommandList *cmdList)
{
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	cmdList->IASetIndexBuffer(&mIndexBufferView);
}

void Model::Destroy()
{
	mVertexBuffer.Reset();
	mIndexBuffer.Reset();
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
	uint32_t faceCount = (initInfo.M - 1) * (initInfo.N - 1) * 2;

	std::vector<Vertex> vertices(vertexCount);
	std::vector<uint32_t> indices(faceCount * 3);

	for (uint32_t i = 0; i < initInfo.M; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32_t j = 0; j < initInfo.N; ++j)
		{
			float x = -halfWidth + j * dx;

			vertices[i * initInfo.N + j].Position = DirectX::XMFLOAT3(x, 0.0f, z);
			vertices[i * initInfo.N + j].Normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

			vertices[i * initInfo.N + j].TexCoord.x = du;
			vertices[i * initInfo.N + j].TexCoord.y = dv;
		}
	}

	uint32_t k = 0;
	for (uint32_t i = 0; i < initInfo.M - 1; ++i)
	{
		for (uint32_t j = 0; j < initInfo.N - 1; ++j)
		{
			indices[k] = i * initInfo.N + j;
			indices[k + 1] = i * initInfo.N + j + 1;
			indices[k + 2] = (i + 1) * initInfo.N + j + 1;
			indices[k + 4] = (i + 1) * initInfo.N + j;

			/*indices[k + 3] = i * initInfo.N + j;
			indices[k + 4] = (i + 1) * initInfo.N + j + 1;
			indices[k + 5] = (i + 1) * initInfo.N + j;*/

			k += 6;
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
		AddInstance();
		return CreateGrid(initInfo);
	}
	else
	{
		static_assert(false, "Invalid type for creating a model");
	}
	return true;
}

template bool Model::CreatePrimitive(const Model::GridInitializationInfo &);
