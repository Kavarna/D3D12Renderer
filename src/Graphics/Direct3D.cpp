#include "Direct3D.h"

Direct3D *gInstance = nullptr;

void Direct3D::Init()
{
    if (gInstance) return;
    gInstance = new Direct3D();
    gInstance->InternalInit();
}

Direct3D* Direct3D::GetInstance()
{
    CHECKSHOW(gInstance, "Using Direct3D::GetInstance() on an uninitialized singletone");
    return gInstance;
}

void Direct3D::Destroy()
{
    if (gInstance == nullptr) return;
    gInstance->InternalDestroy();
    gInstance = nullptr;
}

void Direct3D::InternalInit()
{
    InitFactory();
    InitAdapter();
}

void Direct3D::InternalDestroy()
{
    // mFactory->Release();
    // mAdapter->Release();
}

void Direct3D::InitFactory()
{
    unsigned int factoryFlags = 53;
#if DEBUG || _DEBUG
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mFactory)))
}

void Direct3D::InitAdapter()
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

    CHECKSHOW(currentAdapter, "Unable to find a suitable adapter")
    
    mAdapter = currentAdapter;
}

void Direct3D::InitD3D12Device()
{
    ThrowIfFailed(D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice)));
}
