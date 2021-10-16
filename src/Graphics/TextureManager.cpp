#include "TextureManager.h"
#include "Conversions.h"
#include "Conversions.h"

using DESCRIPTOR_FLAG_TYPE = uint8_t;
constexpr DESCRIPTOR_FLAG_TYPE FLAG_MASK = ~0;
constexpr auto FLAG_SIZE = 8;


Result<uint32_t> TextureManager::AddTexture(const std::string &path)
{    
    return AddTexture(Conversions::s2ws(path).c_str());
}

Result<uint32_t> TextureManager::AddTexture(LPCWSTR path)
{
    CHECK(mCanAddTextures, std::nullopt,
          "Cannot add new textures to texture manager, as it is closed");

    mTexturesToLoad.emplace_back(path);

    uint32_t textureIndex = mNumTextures++;
    SHOWINFO("Created SRV with descriptor {}", textureIndex);

    mTextureIndexToHeapIndex[textureIndex] = { mNumCbvSrvUav++, -1, -1, -1 };

    return textureIndex;
}

Result<uint32_t> TextureManager::AddTexture(const D3D12_RESOURCE_DESC &resourceDesc, const D3D12_HEAP_PROPERTIES &heapProperties,
                                            const D3D12_RESOURCE_STATES &state, const D3D12_HEAP_FLAGS &heapFlags, D3D12_CLEAR_VALUE *clearValue)
{
    CHECK(mCanAddTextures, std::nullopt,
          "Cannot add new textures to texture manager, as it is closed");

    mTexturesToLoad.emplace_back(resourceDesc, heapProperties, state, heapFlags, clearValue);

    uint32_t textureIndex = mNumTextures++;

    int32_t srvIndex = resourceDesc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE ? -1 : mNumCbvSrvUav++;
    int32_t uavIndex = resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ? mNumCbvSrvUav++ : -1;
    int32_t rtvIndex = resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? mNumRtv++ : -1;
    int32_t dsvIndex = resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ? mNumDsv++ : -1;

    mTextureIndexToHeapIndex[textureIndex] = { srvIndex, uavIndex, rtvIndex, dsvIndex };


    SHOWINFO("Created texture with descriptor {}", textureIndex);

    return textureIndex;
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
    return mNumTextures;
}

ComPtr<ID3D12DescriptorHeap> TextureManager::GetSrvUavDescriptorHeap()
{
    return mSrvUavDescriptorHeap;
}

Result<D3D12_GPU_DESCRIPTOR_HANDLE> TextureManager::GetGPUDescriptorSrvHandleForTextureIndex(uint32_t textureIndex)
{
    CHECK(textureIndex < mTextures.size(), std::nullopt,
          "Texture index {} is invalid. This value should be less than {}", textureIndex, mTextures.size());
    CHECK(mTextureIndexToHeapIndex.find(textureIndex) != mTextureIndexToHeapIndex.end(), std::nullopt,
          "Texture index {} is not a index recognized by TextureManager");
    auto heapIndex = std::get<0>(mTextureIndexToHeapIndex[textureIndex]);
    CHECK(heapIndex != -1, std::nullopt, "Texture index {} is not a SRV");

    auto d3d = Direct3D::Get();
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    gpuHandle.Offset(heapIndex, d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>());
    return gpuHandle;
}

bool TextureManager::InitTextures(ID3D12GraphicsCommandList *cmdList, std::vector<ComPtr<ID3D12Resource>> &intermediaryResources)
{
    SHOWINFO("Loading {} textures", mTexturesToLoad.size());
    mTextures.resize(mTexturesToLoad.size());
    intermediaryResources.resize(mTexturesToLoad.size());

    for (uint32_t i = 0; i < (uint32_t)mTexturesToLoad.size(); ++i)
    {
        if (mTexturesToLoad[i]._InitializationType == TextureInitializationParams::Path)
        {
            CHECK(mTextures[i].Init(cmdList, mTexturesToLoad[i]._Path.c_str(), intermediaryResources[i]),
                  false, "Unable to initialize texture {}", Conversions::ws2s(mTexturesToLoad[i]._Path));
        }
        else if (mTexturesToLoad[i]._InitializationType == TextureInitializationParams::InitializationParams)
        {
            auto &initParams = mTexturesToLoad[i]._InitializationParams;
            CHECK(mTextures[i].Init(initParams.resourceDesc, initParams.clearValue, initParams.heapProperties,
                                    initParams.heapFlags, initParams.state), false,
                  "Unable to initialize texture at index {}", i);
        }
    }

    SHOWINFO("Successfully initialized {} textures", mTexturesToLoad.size());
    return true;
}

bool TextureManager::InitDescriptors()
{
    auto d3d = Direct3D::Get();
    ASSIGN_RESULT(mSrvUavDescriptorHeap,
                  d3d->CreateDescriptorHeap(mNumCbvSrvUav, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE), false,
                  "Unable to create descriptor heap for textures");
    CHECK(InitAllViews(), false, "Unable to initialize all SRVs");

    return true;
}

bool TextureManager::InitAllViews()
{
    auto d3d = Direct3D::Get();
    SHOWINFO("Creating {} views", mNumTextures);
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE initialCpuHandle(mSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = initialCpuHandle;

    for (unsigned int i = 0; i < mTextures.size(); ++i)
    {
        if (mTexturesToLoad[i]._InitializationType == TextureInitializationParams::Path)
        {
            // Path-only textures are only shader resource views
            mTextures[i].CreateShaderResourceView(
                mSrvUavDescriptorHeap.Get(),
                cpuHandle.Offset(std::get<SRV_INDEX>(mTextureIndexToHeapIndex[i]), d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>()));
        }
        else if (mTexturesToLoad[i]._InitializationType == TextureInitializationParams::InitializationParams)
        {
            uint32_t createdViews = 0;
            if (!(mTexturesToLoad[i]._InitializationParams.resourceDesc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
            {
                // Create SRV if needed
                mTextures[i].CreateShaderResourceView(
                    mSrvUavDescriptorHeap.Get(),
                    cpuHandle.Offset(std::get<SRV_INDEX>(mTextureIndexToHeapIndex[i]), d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>()));
                createdViews++;
            }
            if (mTexturesToLoad[i]._InitializationParams.resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
            {
                // Create UAV if needed
                mTextures[i].CreateUnorederedAccessView(
                    mSrvUavDescriptorHeap.Get(),
                    cpuHandle.Offset(std::get<UAV_INDEX>(mTextureIndexToHeapIndex[i]), d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>()));
                createdViews++;
            }
            if (mTexturesToLoad[i]._InitializationParams.resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
            {
                // Create RTV if needed
                mTextures[i].CreateRenderTargetView(
                    mSrvUavDescriptorHeap.Get(),
                    cpuHandle.Offset(std::get<UAV_INDEX>(mTextureIndexToHeapIndex[i]), d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>()));
                createdViews++;
            }
            if (mTexturesToLoad[i]._InitializationParams.resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            {
                // Create DSV if needed
                mTextures[i].CreateRenderTargetView(
                    mSrvUavDescriptorHeap.Get(),
                    cpuHandle.Offset(std::get<UAV_INDEX>(mTextureIndexToHeapIndex[i]), d3d->GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>()));
                createdViews++;
            }
            SHOWINFO("Created {} views for texture located at index {}", createdViews, i);
        }
        cpuHandle = initialCpuHandle;
    }

    SHOWINFO("Successfully created {} shader resource views", mTexturesToLoad.size());
    return true;
}
