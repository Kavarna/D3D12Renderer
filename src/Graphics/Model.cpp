#include "Model.h"
#include "Utils/Utils.h"
#include "Direct3D.h"

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
				SHOWINFO("[Loading Model {}] Mesh {} loaded once, using that version", path, meshName);
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

	std::vector<Vertex> vertices;
	vertices.reserve(mesh->mNumVertices);
	for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex currentVertex;
		currentVertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		currentVertex.Color = { 1.0f, 1.0f, 0.0f, 1.0f }; // Default good enough color

		vertices.push_back(std::move(currentVertex));
	}

	std::vector<uint32_t> indices;
	indices.reserve(mesh->mNumFaces * 3);
	for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
	{
		CHECK((mesh->mFaces[i].mNumIndices == 3), false,
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

bool Model::Create(ModelType type)
{
	switch (type)
	{
		case Model::ModelType::Triangle:
			CHECK(CreateTriangle(), false, "Unable to create triangle");
			break;
		default:
			SHOWFATAL("Model type {} is not a valid model", (int)type);
			return false;
	}

    return true;
}

bool Model::Create(const std::string &path)
{
	Assimp::Importer importer;

	const aiScene *pScene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);


	CHECK(ProcessNode(pScene->mRootNode, pScene, path), false,
		  "Unable to load model located at path {}", path);

	return true;
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
		Vertex(XMFLOAT3( 0.0f,  1.0f, 0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
		Vertex(XMFLOAT3(+1.0f, -1.0f, 0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)),
		Vertex(XMFLOAT3(-1.0f, -1.0f, 0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)),
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
