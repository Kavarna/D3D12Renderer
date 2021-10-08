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
    CHECK(InitTextures(cmdList, intermediaryResources), false, "Unable to close adding textures")
    mCanAddTextures = false;
    return true;
}

uint32_t TextureManager::GetTextureCount() const
{
    return (uint32_t)mTexturesToLoad.size();
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

    return true;
}
