#include "Utils.h"

#include "Conversions.h"

Result<ComPtr<ID3DBlob>> Utils::CompileShader(LPCWSTR filename, LPCSTR profile)
{
    ComPtr<ID3DBlob> result, errorMessages;

    UINT flags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    auto szFilename = Conversions::ws2s(filename);

    HRESULT hr = D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
                                    profile, flags, 0, &result, &errorMessages);
    if (errorMessages)
    {
        SHOWWARNING("Message shown when compiling {} with profile {}: {}",
                    szFilename, profile, (char *)errorMessages->GetBufferPointer());
    }
    CHECK(SUCCEEDED(hr), false, "Unable to compile {} with profile {}", szFilename, profile);

    return result;
}


std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>> Utils::CreateDefaultBuffer(ID3D12Device *device,
                                                                                      ID3D12GraphicsCommandList *cmdList,
                                                                                      D3D12_RESOURCE_STATES state,
                                                                                      void *data, uint32_t dataSize)
{
    ComPtr<ID3D12Resource> finalResource, temporaryResource;
    std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>> invalidResult = { nullptr, nullptr };

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

    CHECK_HR(device->CreateCommittedResource(
        &defaultHeapProperties, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&finalResource)), invalidResult);

    if (data == nullptr || dataSize == 0)
    {
        SHOWINFO("Creating a default buffer wit no data");
        return { finalResource, temporaryResource };
    }

    CHECK_HR(device->CreateCommittedResource(
        &uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&temporaryResource)), invalidResult);

    D3D12_SUBRESOURCE_DATA subresourceData;
    subresourceData.pData = data;
    subresourceData.RowPitch = dataSize;
    subresourceData.SlicePitch = dataSize;

    UpdateSubresources<1>(cmdList, finalResource.Get(), temporaryResource.Get(), 0, 0, 1, &subresourceData);

    if (state != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        CD3DX12_RESOURCE_BARRIER barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(finalResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, state);
        cmdList->ResourceBarrier(1, &barrier);
    }
    return { finalResource, temporaryResource };
}
