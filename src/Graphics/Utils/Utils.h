#pragma once

#include <Oblivion.h>

namespace Utils
{
    std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>> CreateDefaultBuffer(
        ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES state, void *data, uint32_t dataSize,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE);

    template <typename BlobType>
    std::string ConvertBlobToString(BlobType blob)
    {
        std::vector<char> info(blob->GetBufferSize() + 1);
        memcpy(info.data(), blob->GetBufferPointer(), blob->GetBufferSize());
        info[blob->GetBufferSize()] = 0;
        return std::string(info.data());
    }

}