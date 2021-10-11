#pragma once


#include <Oblivion.h>
#include <ISingletone.h>

#include "Texture.h"

class TextureManager : public ISingletone<TextureManager>
{
    MAKE_SINGLETONE_CAPABLE(TextureManager);
public:
    TextureManager() = default;
    ~TextureManager() = default;

public:
    Result<uint64_t> AddTexture(const std::string &path);
    Result<uint64_t> AddTexture(LPCWSTR path);

public:
    bool CloseAddingTextures(ID3D12GraphicsCommandList *cmdList, std::vector<ComPtr<ID3D12Resource>> &intermediaryResources);
    uint32_t GetTextureCount() const;

    ComPtr<ID3D12DescriptorHeap> GetSRVDescriptorHeap();
    Result<D3D12_GPU_DESCRIPTOR_HANDLE> GetGPUDescriptorSRVHandleForTextureIndex(uint64_t descriptor);

private:
    bool InitTextures(ID3D12GraphicsCommandList *cmdList, std::vector<ComPtr<ID3D12Resource>> &intermediaryResources);
    bool InitDescriptors();

    bool InitSRVs();

private:
    bool mCanAddTextures = true;

    ComPtr<ID3D12DescriptorHeap> mSRVDescriptorHeap;

    std::vector<std::wstring> mTexturesToLoad;

    std::vector<Texture> mTextures;
};

