#include "Oblivion.h"
#include "Utils.h"

#include "Conversions.h"
#include <dxcapi.h>


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

ComPtr<ID3DBlob> Utils::CompileLibrary(const wchar_t* filename, const wchar_t* target)
{
    ComPtr<IDxcCompiler> compiler;
    ComPtr<IDxcLibrary> library;

    // Create library & compiler
    CHECK_HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)), nullptr);
    CHECK_HR(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)), nullptr);

    // Get compilation arguments
    // TODO: Make this dynamic
    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-Zi"); // DEBUG
    arguments.push_back(L"-Od"); // Skip optimizations


    // Read the shader
    std::ifstream shaderFile(filename);
    CHECK(shaderFile.good(), nullptr, "Unable to open file");

    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    std::string shaderContent = strStream.str();

    // Create blob from string
    ComPtr<IDxcBlobEncoding> textBlob;
    CHECK_HR(library->CreateBlobWithEncodingFromPinned((void*)shaderContent.data(), (uint32_t)shaderContent.size(), 0, &textBlob), nullptr);

    ComPtr<IDxcOperationResult> result;
    CHECK_HR(compiler->Compile(textBlob.Get(), filename, L"", target, arguments.data(), (uint32_t)arguments.size(), nullptr, 0, nullptr, &result), nullptr);

    HRESULT errorCode;
    CHECK_HR(result->GetStatus(&errorCode), nullptr);
    if (FAILED(errorCode))
    {
        ComPtr<IDxcBlobEncoding> errorBlob;
        CHECK_HR(result->GetErrorBuffer(&errorBlob), nullptr);

        std::string error = Utils::ConvertBlobToString(errorBlob);
        SHOWFATAL("Error when compiling shader {}", error);
        return nullptr;
    }

    ComPtr<ID3DBlob> finalResult;
    CHECK_HR(result->GetResult((IDxcBlob**)finalResult.GetAddressOf()), nullptr);
    SHOWINFO("Successfully compiled library");
    return finalResult;
}
