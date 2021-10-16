#pragma once


#include <Oblivion.h>
#include "./Utils/D3DObject.h"


class Texture : public D3DObject
{
public:
    Texture() = default;

public:
    bool Init(ID3D12GraphicsCommandList *cmdList, const wchar_t *path, ComPtr<ID3D12Resource>& intermediary);
    bool Init(const D3D12_RESOURCE_DESC &resourceDesc, D3D12_CLEAR_VALUE *clearValue, const D3D12_HEAP_PROPERTIES &heapProperties,
              const D3D12_HEAP_FLAGS &heapFlags,
              const D3D12_RESOURCE_STATES &state);

    void CreateShaderResourceView(ID3D12DescriptorHeap* heap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
    void CreateUnorederedAccessView(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
    void CreateRenderTargetView(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
    void CreateDepthStencilView(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

private:
    D3D12_RESOURCE_STATES mCurrentResourceState;
    D3D12_RESOURCE_DESC mDesc;
    ComPtr<ID3D12Resource> mResource;
};


