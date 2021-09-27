#pragma once

#include <Oblivion.h>

namespace Utils
{
    Result<ComPtr<ID3DBlob>> CompileShader(LPCWSTR filename, LPCSTR profile);

    std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>> CreateDefaultBuffer(
        ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES state, void *data, uint32_t dataSize);

}