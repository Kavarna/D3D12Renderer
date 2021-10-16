#pragma once


#include <Oblivion.h>
#include <ISingletone.h>

#include "Texture.h"

class TextureManager : public ISingletone<TextureManager>
{
    MAKE_SINGLETONE_CAPABLE(TextureManager);

    static constexpr const uint32_t SRV_INDEX = 0;
    static constexpr const uint32_t UAV_INDEX = 0;
    static constexpr const uint32_t RTV_INDEX = 0;
    static constexpr const uint32_t DSV_INDEX = 0;

public:
    TextureManager() = default;
    ~TextureManager() = default;

public:
    Result<uint32_t> AddTexture(const std::string &path);
    Result<uint32_t> AddTexture(LPCWSTR path);
    Result<uint32_t> AddTexture(const D3D12_RESOURCE_DESC &resourceDesc, const D3D12_HEAP_PROPERTIES &heapProperties,
                                const D3D12_RESOURCE_STATES &state = D3D12_RESOURCE_STATE_GENERIC_READ,
                                const D3D12_HEAP_FLAGS &heapFlags = D3D12_HEAP_FLAG_NONE,
                                D3D12_CLEAR_VALUE *clearValue = nullptr);

public:
    bool CloseAddingTextures(ID3D12GraphicsCommandList *cmdList, std::vector<ComPtr<ID3D12Resource>> &intermediaryResources);
    uint32_t GetTextureCount() const;

    ComPtr<ID3D12DescriptorHeap> GetSrvUavDescriptorHeap();
    Result<D3D12_GPU_DESCRIPTOR_HANDLE> GetGPUDescriptorSrvHandleForTextureIndex(uint32_t textureIndex);

private:
    bool InitTextures(ID3D12GraphicsCommandList *cmdList, std::vector<ComPtr<ID3D12Resource>> &intermediaryResources);
    bool InitDescriptors();

    bool InitAllViews();

private:
    bool mCanAddTextures = true;

    struct TextureInitializationParams
    {
        TextureInitializationParams(const std::wstring &_Path): _InitializationType(Path), _Path(_Path)
        {
        }

        TextureInitializationParams(const D3D12_RESOURCE_DESC &resourceDesc, const D3D12_HEAP_PROPERTIES &heapProperties,
                                    const D3D12_RESOURCE_STATES &state, const D3D12_HEAP_FLAGS &heapFlags, D3D12_CLEAR_VALUE *clearValue):
            _InitializationType(InitializationParams), _InitializationParams()
        {
            _InitializationParams.resourceDesc = resourceDesc;
            _InitializationParams.clearValue= clearValue;
            _InitializationParams.heapProperties = heapProperties;
            _InitializationParams.heapFlags = heapFlags;
            _InitializationParams.state = state;
        }

        std::wstring _Path;

        struct
        {
            D3D12_RESOURCE_DESC resourceDesc;
            D3D12_CLEAR_VALUE* clearValue;
            D3D12_HEAP_PROPERTIES heapProperties;
            D3D12_HEAP_FLAGS heapFlags;
            D3D12_RESOURCE_STATES state;
        } _InitializationParams;

        enum
        {
            Path, InitializationParams
        } _InitializationType;
    };

    ComPtr<ID3D12DescriptorHeap> mSrvUavDescriptorHeap;

    std::vector<TextureInitializationParams> mTexturesToLoad;

    std::unordered_map<uint32_t, std::tuple<int32_t, int32_t, int32_t, int32_t>> mTextureIndexToHeapIndex;

    std::vector<Texture> mTextures;

    uint32_t mNumTextures = 0;
    uint32_t mNumCbvSrvUav = 0;
    uint32_t mNumRtv = 0;
    uint32_t mNumDsv = 0;
};

