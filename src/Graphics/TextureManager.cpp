#include "TextureManager.h"
#include "Conversions.h"
#include "Conversions.h"

Result<uint32_t> TextureManager::AddTexture(const std::string &path)
{
    CHECK(mCanAddTextures, std::nullopt,
          "Cannot add new textures to texture manager, as it is closed");

    mTexturesToLoad.push_back(Conversions::s2ws(path));

    return (uint32_t)mTexturesToLoad.size() - 1;
}

Result<uint32_t> TextureManager::AddTexture(LPCWSTR path)
{
    CHECK(mCanAddTextures, std::nullopt,
          "Cannot add new textures to texture manager, as it is closed");

    mTexturesToLoad.push_back(path);

    return (uint32_t)mTexturesToLoad.size() - 1;
}

bool TextureManager::CloseAddingTextures(ID3D12GraphicsCommandList* cmdList, std::vector<ComPtr<ID3D12Resource>>& intermediaryResources)
{
    CHECK(InitTextures(cmdList, intermediaryResources), false, "Unable to initialize textures");
    CHECK(InitDescriptors(), false, "Unable to initialize descriptors for textures");
    mCanAddTextures = false;
    return true;
}

uint32_t TextureManager::GetTextureCount() const
{
    return (uint32_t)mTexturesToLoad.size();
}

ComPtr<ID3D12DescriptorHeap> TextureManager::GetSRVDescriptorHeap()
{
    return mSRVDescriptorHeap;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGPUDescriptorSRVHandleForTextureIndex(uint32_t textureIndex)
{
    auto d3d = Direct3D::Get();
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    gpuHandle.Offset(textureIndex, d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>());
    return gpuHandle;
}

bool TextureManager::InitTextures(ID3D12GraphicsCommandList *cmdList, std::vector<ComPtr<ID3D12Resource>> &intermediaryResources)
{
    SHOWINFO("Loading {} textures", mTexturesToLoad.size());
    mTextures.resize(mTexturesToLoad.size());
    intermediaryResources.resize(mTexturesToLoad.size());

    for (uint32_t i = 0; i < (uint32_t)mTexturesToLoad.size(); ++i)
    {
        CHECK(mTextures[i].Init(cmdList, mTexturesToLoad[i].c_str(), intermediaryResources[i]),
              false, "Unable to initialize texture {}", Conversions::ws2s(mTexturesToLoad[i]));
    }

    SHOWINFO("Successfully initialized {} textures", mTexturesToLoad.size());
    return true;
}

bool TextureManager::InitDescriptors()
{
    auto d3d = Direct3D::Get();
    ASSIGN_RESULT(mSRVDescriptorHeap,
                  d3d->CreateDescriptorHeap((uint32_t)mTextures.size(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE), false,
                  "Unable to create descriptor heap for textures");
    CHECK(InitSRVs(), false, "Unable to initialize all SRVs");

    return true;
}

bool TextureManager::InitSRVs()
{
    auto d3d = Direct3D::Get();
    SHOWINFO("Creating {} shader resource views", mTexturesToLoad.size());
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (unsigned int i = 0; i < mTextures.size(); ++i)
    {
        mTextures[i].CreateShaderResourceView(mSRVDescriptorHeap.Get(), cpuHandle);
        cpuHandle.Offset(1, d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>());
    }

    SHOWINFO("Successfully created {} shader resource views", mTexturesToLoad.size());
    return true;
}
