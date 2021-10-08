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


