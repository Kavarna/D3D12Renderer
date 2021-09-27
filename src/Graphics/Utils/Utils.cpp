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

    return { finalResource, temporaryResource };
}
