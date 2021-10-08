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


