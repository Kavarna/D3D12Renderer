#include "Direct3D.h"


bool Direct3D::Init()
{
    auto factoryResult = CreateFactory();
    CHECK(factoryResult.Valid(), false, "Cannot create a factory");
    mFactory = factoryResult.Get();
    
    auto adapterResult = CreateAdapter();
    CHECK(adapterResult.Valid(), false, "Cannot create an adapter");
    mAdapter = adapterResult.Get();

    auto deviceResult = CreateD3D12Device();
    CHECK(deviceResult.Valid(), false, "Cannot create a D3D12 device");
    mDevice = deviceResult.Get();


    SHOWINFO("Successfully initialized Direct3D");
    return true;
}

Result<ComPtr<IDXGIFactory>> Direct3D::CreateFactory()
{
    unsigned int factoryFlags = 0;
#if DEBUG || _DEBUG
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    ComPtr<IDXGIFactory> factory;
    CHECK_HR(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)), std::nullopt);

    return factory;
}

Result<ComPtr<IDXGIAdapter>> Direct3D::CreateAdapter()
{
    uint32_t currentAdapterIndex = 0;
    size_t maxVideoMemory = 0;
    ComPtr<IDXGIAdapter> currentAdapter, bestAdapter = nullptr;

    while (SUCCEEDED(mFactory->EnumAdapters(currentAdapterIndex, &currentAdapter)))
    {
        currentAdapterIndex++;
        DXGI_ADAPTER_DESC adapterDesc;
        if (FAILED(currentAdapter->GetDesc(&adapterDesc))) continue;

        if (adapterDesc.DedicatedVideoMemory > maxVideoMemory)
        {
            maxVideoMemory = adapterDesc.DedicatedVideoMemory;
            bestAdapter = currentAdapter;
        }
    }

    CHECK(bestAdapter, std::nullopt, "Unable to find a suitable adapter");

    return bestAdapter;
}

Result<ComPtr<ID3D12Device>> Direct3D::CreateD3D12Device()
{
    ComPtr<ID3D12Device> device;
    CHECK_HR(D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)), std::nullopt);
    return device;
}
