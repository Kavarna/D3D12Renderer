#include "Texture.h"
#include "Utils/DDSTextureLoader.h"
#include "Conversions.h"

bool Texture::Init(ID3D12GraphicsCommandList *cmdList, const wchar_t* path, ComPtr<ID3D12Resource>& intermediary)
{
    CHECK(D3DObject::Init(), false, "Unable to initialize d3d object for path {}", Conversions::ws2s(path));
    
    CHECK_HR(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), cmdList, path,
                                                 mResource, intermediary), false);

    mCurrentResourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    mDesc = mResource->GetDesc();
    
    return true;
}

bool Texture::Init(const D3D12_RESOURCE_DESC &resourceDesc, D3D12_CLEAR_VALUE *clearValue, const D3D12_HEAP_PROPERTIES &heapProperties,
                   const D3D12_HEAP_FLAGS &heapFlags, const D3D12_RESOURCE_STATES &state)
{
    CHECK(D3DObject::Init(), false, "Unable to initialize d3d object for texture");

    CHECK_HR(mDevice->CreateCommittedResource(
        &heapProperties, heapFlags, &resourceDesc,
        state, clearValue, IID_PPV_ARGS(&mResource)), false);

    mCurrentResourceState = state;
    mDesc = mResource->GetDesc();

    return true;
}

void Texture::Transition(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES state)
{
    if (state == mCurrentResourceState)
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), mCurrentResourceState, state);
    mCurrentResourceState = state;
    cmdList->ResourceBarrier(1, &barrier);
}

void Texture::CreateShaderResourceView(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    auto d3d = Direct3D::Get();
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = mDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mDesc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    d3d->CreateShaderResourceView(mResource.Get(), srvDesc, cpuHandle);
}

void Texture::CreateUnorederedAccessView(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    auto d3d = Direct3D::Get();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = mDesc.Format;
    uavDesc.Texture2D.MipSlice = 0;
    uavDesc.Texture2D.PlaneSlice = 0;

    d3d->CreateUnorderedAccessView(mResource.Get(), uavDesc, cpuHandle);
}

void Texture::CreateRenderTargetView(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    auto d3d = Direct3D::Get();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = mDesc.Format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    
    d3d->CreateRenderTargetView(mResource.Get(), rtvDesc, cpuHandle);
}

void Texture::CreateDepthStencilView(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    auto d3d = Direct3D::Get();

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = mDesc.Format;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.Texture2D.MipSlice = 0;

    d3d->CreateDepthStencilView(mResource.Get(), dsvDesc, cpuHandle);
}


