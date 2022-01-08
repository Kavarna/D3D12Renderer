#include "Utils.h"

#include "Conversions.h"


std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>> Utils::CreateDefaultBuffer(ID3D12Device *device,
                                                                                      ID3D12GraphicsCommandList *cmdList,
                                                                                      D3D12_RESOURCE_STATES state,
                                                                                      void *data, uint32_t dataSize,
                                                                                      D3D12_RESOURCE_FLAGS flags)
{
    ComPtr<ID3D12Resource> finalResource, temporaryResource;
    std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>> invalidResult = { nullptr, nullptr };

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    bufferDesc.Flags = flags;


    if (data == nullptr || dataSize == 0)
    {
        SHOWINFO("Creating a default buffer with no data");

        CHECK_HR(device->CreateCommittedResource(
            &defaultHeapProperties, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, state, nullptr,
            IID_PPV_ARGS(&finalResource)), invalidResult);
    }
    else
    {
        CHECK_HR(device->CreateCommittedResource(
            &defaultHeapProperties, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&finalResource)), invalidResult);
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
    }
    return { finalResource, temporaryResource };
}
