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

MaterialManager::Material const *Model::GetMaterial() const
{
	return mMaterial;
}

const DirectX::XMMATRIX &__vectorcall Model::GetWorld() const
{
	return mWorld;
}

const DirectX::XMMATRIX &__vectorcall Model::GetTexWorld() const
{
	return mTexWorld;
}

void Model::Identity()
{
	mWorld = DirectX::XMMatrixIdentity();
	MarkUpdate();
}

void Model::Translate(float x, float y, float z)
{
	mWorld *= DirectX::XMMatrixTranslation(x, y, z);
	MarkUpdate();
}

void Model::RotateX(float theta)
{
	mWorld = DirectX::XMMatrixRotationX(theta) * mWorld;
	MarkUpdate();
}

void Model::RotateY(float theta)
{
	mWorld = DirectX::XMMatrixRotationY(theta) * mWorld;
	MarkUpdate();
}

void Model::RotateZ(float theta)
{
	mWorld = DirectX::XMMatrixRotationZ(theta) * mWorld;
	MarkUpdate();
}

void Model::Scale(float scaleFactor)
{
	Scale(scaleFactor, scaleFactor, scaleFactor);
}

void Model::Scale(float scaleFactorX, float scaleFactorY, float scaleFactorZ)
{
	mWorld = DirectX::XMMatrixScaling(scaleFactorX, scaleFactorY, scaleFactorZ) * mWorld;
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
			SHOWFATAL("Model type {} is not a valid model", (int)type);
			return false;
	}

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
