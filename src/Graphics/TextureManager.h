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
    Result<uint32_t> AddTexture(const std::string &path);
    Result<uint32_t> AddTexture(LPCWSTR path);

public:
    bool CloseAddingTextures(ID3D12GraphicsCommandList *cmdList, std::vector<ComPtr<ID3D12Resource>> &intermediaryResources);
    uint32_t GetTextureCount() const;

private:
    bool InitTextures(ID3D12GraphicsCommandList *cmdList, std::vector<ComPtr<ID3D12Resource>> &intermediaryResources);

private:
    bool mCanAddTextures = true;

    std::vector<std::wstring> mTexturesToLoad;

    std::vector<Texture> mTextures;
};

