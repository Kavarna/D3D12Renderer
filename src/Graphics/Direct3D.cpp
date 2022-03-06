#include "Direct3D.h"


void EnableDebugLayer()
{
    ComPtr<ID3D12Debug> debugInterface;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
    debugInterface->EnableDebugLayer();
}


bool Direct3D::Init(HWND hwnd)
{
    CHECK(hwnd, false, "Cannot create a D3D12 intance without a valid window");

    EnableDebugLayer();

    ASSIGN_RESULT(mFactory, CreateFactory(), false, "Cannot create a valid factory");
    ASSIGN_RESULT(mAdapter, CreateAdapter(), false, "Cannot create an adapter");
    ASSIGN_RESULT(mDevice, CreateD3D12Device(), false, "Cannot create a D3D12 device");
    ASSIGN_RESULT(mDirectCommandQueue, CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT), false, "Cannot initialize direct command queue");
    ASSIGN_RESULT(mSwapchain, CreateSwapchain(hwnd), false, "Cannot create swapchain");
    ASSIGN_RESULT(mRTVHeap, CreateDescriptorHeap(kBufferCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
                  false, "Unable to create a rtv descriptor heap with {} descriptors", kBufferCount);
    ASSIGN_RESULT(mDSVHeap, CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV),
                  false, "Unable to create a dsv descriptor heap with 1 descriptor");

    ASSIGN_RESULT(mDepthStencilResource, CreateDepthStencilBuffer(),
                  false, "Unable to create a depth stencil buffer");

    CHECK(UpdateDescriptors(), false, "Unable to update descriptors");

    SHOWINFO("Successfully initialized Direct3D");
    return true;
}

bool Direct3D::OnResize(uint32_t width, uint32_t height)
{
    for (unsigned int i = 0; i < kBufferCount; ++i)
    {
        mSwapchainResources[i].Reset();
    }
    mDepthStencilResource.Reset();

    DXGI_SWAP_CHAIN_DESC swapchainDesc;
    CHECK_HR(mSwapchain->GetDesc(&swapchainDesc), false);

    CHECK_HR(mSwapchain->ResizeBuffers(
        kBufferCount, width, height, kBackbufferFormat,
        swapchainDesc.Flags), false);

    ASSIGN_RESULT(mDepthStencilResource, CreateDepthStencilBuffer(width, height),
                  false, "Unable to create a depth stencil buffer");


    return UpdateDescriptors();
}

void Direct3D::OnRenderBegin(ID3D12GraphicsCommandList *cmdList)
{
    uint32_t activeBackBuffer = mSwapchain->GetCurrentBackBufferIndex();
    Transition(cmdList, mSwapchainResources[activeBackBuffer].Get(),
               D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    static float r = 0.0f, g = 0.0f, b = 0.0f;
#ifdef COLORFUL_BACKGROUND
    static int32_t dr = 1, dg = 1, db = 1;
    r += dr * 0.001f;
    g += dg * 0.003f;
    b += db * 0.001f;

    if (r <= 0.0f || r >= 1.0f)
    {
        dr *= -1;
    }
    if (g <= 0.0f || g >= 1.0f)
    {
        dg *= -1;
    }
    if (b <= 0.0f || b >= 1.0f)
    {
        db *= -1;
    }
#endif // COLORFUL_BACKGROUND
}

void Direct3D::OnRenderEnd(ID3D12GraphicsCommandList *cmdList)
{
    uint32_t activeBackBuffer = mSwapchain->GetCurrentBackBufferIndex();
    Transition(cmdList, mSwapchainResources[activeBackBuffer].Get(),
               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void Direct3D::Present()
{
    UINT presentInterval = mVsync ? 1 : 0;
    UINT presentFlags = (!mVsync && AllowTearing()) ? DXGI_PRESENT_ALLOW_TEARING : 0;
    mSwapchain->Present(presentInterval, presentFlags);
}

D3D12_CPU_DESCRIPTOR_HANDLE Direct3D::GetDSVHandle()
{
    return mDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE Direct3D::GetBackbufferHandle()
{
    uint32_t activeBackBuffer = mSwapchain->GetCurrentBackBufferIndex();
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(activeBackBuffer, GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>());
    return rtvHandle;
}

Result<ComPtr<ID3D12CommandAllocator>> Direct3D::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> result;
    CHECK_HR(mDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&result)), std::nullopt);
    SHOWINFO("Successfully created a command allcator");
    return result;
}

Result<ComPtr<ID3D12GraphicsCommandList>> Direct3D::CreateCommandList(ID3D12CommandAllocator *allocator,
                                                                      D3D12_COMMAND_LIST_TYPE type, ID3D12PipelineState *pipeline)
{
    ComPtr<ID3D12GraphicsCommandList> result;
    CHECK_HR(mDevice->CreateCommandList(0, type, allocator, pipeline, IID_PPV_ARGS(&result)), std::nullopt);
    SHOWINFO("Successfully created a command list");
    return result;
}

Result<ComPtr<ID3D12Fence>> Direct3D::CreateFence(uint64_t initialValue)
{
    ComPtr<ID3D12Fence> result;
    CHECK_HR(mDevice->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&result)), std::nullopt);
    SHOWINFO("Successfully created a fence");
    return result;
}

Result<ComPtr<ID3D12DescriptorHeap>> Direct3D::CreateDescriptorHeap(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                                    D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
    ComPtr<ID3D12DescriptorHeap> result;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NodeMask = 0;
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = type;
    heapDesc.Flags = flags;
    CHECK_HR(mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&result)), std::nullopt);
    SHOWINFO("Successfully created a descriptor heap with {} descriptors", numDescriptors);
    return result;
}

Result<ComPtr<ID3D12PipelineState>> Direct3D::CreatePipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
    ComPtr<ID3D12PipelineState> result;
    CHECK_HR(mDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&result)), std::nullopt);
    SHOWINFO("Successfully created a pipeline state");
    return result;
}

Result<ComPtr<ID3D12PipelineState>> Direct3D::CreatePipelineState(const D3D12_COMPUTE_PIPELINE_STATE_DESC &desc)
{
    ComPtr<ID3D12PipelineState> result;
    CHECK_HR(mDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&result)), std::nullopt);
    SHOWINFO("Successfully created a compute pipeline state");
    return result;
}

Result<ComPtr<ID3D12RootSignature>> Direct3D::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    ComPtr<ID3D12RootSignature> result;
    ComPtr<ID3DBlob> signatureBlob, errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signatureBlob, &errorBlob);
    if (errorBlob)
    {
        SHOWWARNING("Message shown while serializing a root signature: {}", (char *)errorBlob->GetBufferPointer());
    }
    CHECK(SUCCEEDED(hr), std::nullopt, "Unable to create a root signature from a unaserialized root signature");

    CHECK_HR(mDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
                                          IID_PPV_ARGS(&result)), std::nullopt);
    SHOWINFO("Successfully created a root signature");
    return result;
}

ComPtr<ID3D12Device> Direct3D::GetD3D12Device()
{
    return mDevice;
}

ComPtr<ID3D12Resource> Direct3D::GetCurrentBackbufferResource()
{
    uint32_t activeBackBuffer = mSwapchain->GetCurrentBackBufferIndex();
    return mSwapchainResources[activeBackBuffer];
}

void Direct3D::CreateShaderResourceView(ID3D12Resource *resource, const D3D12_SHADER_RESOURCE_VIEW_DESC &srvDesc,
                                        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    mDevice->CreateShaderResourceView(resource, &srvDesc, cpuHandle);
    SHOWINFO("Successfully created a shader resource view");
}

void Direct3D::CreateUnorderedAccessView(ID3D12Resource *resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC &uavDesc, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    mDevice->CreateUnorderedAccessView(resource, nullptr, &uavDesc, cpuHandle);
    SHOWINFO("Successfully created a unordered access view");
}

void Direct3D::CreateRenderTargetView(ID3D12Resource *resource, const D3D12_RENDER_TARGET_VIEW_DESC &rtvDesc, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    mDevice->CreateRenderTargetView(resource, &rtvDesc, cpuHandle);
    SHOWINFO("Successfully created a render target view");
}

void Direct3D::CreateDepthStencilView(ID3D12Resource *resource, const D3D12_DEPTH_STENCIL_VIEW_DESC &dsvDesc, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    mDevice->CreateDepthStencilView(resource, &dsvDesc, cpuHandle);
    SHOWINFO("Successfully created a depth stencil view");
}

bool Direct3D::AllowTearing()
{
    static std::optional<bool> tearingEnabled;
    if (tearingEnabled.has_value())
        return *tearingEnabled;

    ComPtr<IDXGIFactory5> factory5;
    CHECK_HR(mFactory.As(&factory5), false);

    BOOL tearingFeature = FALSE;
    CHECK_HR(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingFeature, sizeof(tearingFeature)), false);
    
    tearingEnabled = (tearingFeature == TRUE);
    return *tearingEnabled;
}

void Direct3D::Transition(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *resource,
                          D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_STATES finalState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, initialState, finalState);
    cmdList->ResourceBarrier(1, &barrier);
}

void Direct3D::Signal(ID3D12Fence *fence, uint64_t value)
{
    CHECKRET_HR(mDirectCommandQueue->Signal(fence, value));
}

void Direct3D::WaitForFenceValue(ID3D12Fence* fence, uint64_t value)
{
    if (fence->GetCompletedValue() >= value)
    {
        return;
    }

    HANDLE evt = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    CHECKRET_HR(fence->SetEventOnCompletion(value, evt));
    WaitForSingleObject(evt, INFINITE);
    CloseHandle(evt);
}

void Direct3D::ExecuteCommandList(ID3D12GraphicsCommandList *cmdList)
{
    ID3D12CommandList *cmdLists[] = {
        cmdList
    };
    mDirectCommandQueue->ExecuteCommandLists(ARRAYSIZE(cmdLists), cmdLists);
}

void Direct3D::Flush(ID3D12GraphicsCommandList *cmdList, ID3D12Fence *fence, uint64_t value)
{
    ExecuteCommandList(cmdList);
    Signal(fence, value);
    WaitForFenceValue(fence, value);
}

Result<ComPtr<ID3D12Resource>> Direct3D::CreateDepthStencilBuffer()
{
    CHECK(mSwapchain, std::nullopt, "Cannot create a default depth stencil buffer without a valid swapchain");

    DXGI_SWAP_CHAIN_DESC swapchainDesc;
    CHECK_HR(mSwapchain->GetDesc(&swapchainDesc), std::nullopt);
    
    return CreateDepthStencilBuffer(swapchainDesc.BufferDesc.Width, swapchainDesc.BufferDesc.Height);
}

Result<ComPtr<ID3D12Resource>> Direct3D::CreateDepthStencilBuffer(uint32_t width, uint32_t height)
{
    auto defaultHeapBuffer = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(kDepthStencilFormat, width, height);

    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE optimizedClearValue;
    optimizedClearValue.Format = kDepthStencilFormat;
    optimizedClearValue.DepthStencil.Depth = 1.0f;
    optimizedClearValue.DepthStencil.Stencil = 0;

    ComPtr<ID3D12Resource> depthBuffer;

    CHECK_HR(mDevice->CreateCommittedResource(
        &defaultHeapBuffer, D3D12_HEAP_FLAG_NONE, &textureDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearValue,
        IID_PPV_ARGS(&depthBuffer)), std::nullopt);

    SHOWINFO("Successfully created a depth stencil buffer");
    return depthBuffer;
}

Result<ComPtr<ID3D12CommandQueue>> Direct3D::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE queueType)
{
    ComPtr<ID3D12CommandQueue> queue;
    CHECK(mDevice, std::nullopt, "Cannot create a command queue if Direct3D wasn't successfully initialized");
    
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Type = queueType;

    CHECK_HR(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)), std::nullopt);
    SHOWINFO("Successfully created a command queue");
    return queue;
}

Result<ComPtr<IDXGISwapChain4>> Direct3D::CreateSwapchain(HWND hwnd)
{
    CHECK(mDirectCommandQueue, std::nullopt, "Cannot create a swapchain without a valid direct command queue");
    ComPtr<IDXGIFactory4> factory4;
    CHECK_HR(mFactory.As(&factory4), std::nullopt);

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc1 = {};
    swapchainDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchainDesc1.BufferCount = kBufferCount;
    swapchainDesc1.BufferUsage = DXGI_USAGE_BACK_BUFFER;
    swapchainDesc1.Format = kBackbufferFormat;
    swapchainDesc1.Scaling = DXGI_SCALING_NONE;
    swapchainDesc1.Stereo = FALSE;
    swapchainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    swapchainDesc1.SampleDesc.Count = 1;
    swapchainDesc1.SampleDesc.Quality = 0;

    swapchainDesc1.Flags = AllowTearing() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    swapchainDesc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapchain1;
    CHECK_HR(factory4->CreateSwapChainForHwnd(mDirectCommandQueue.Get(), hwnd, &swapchainDesc1, nullptr, nullptr, &swapchain1), std::nullopt);
    ComPtr<IDXGISwapChain4> swapchain;
    CHECK_HR(swapchain1.As(&swapchain), std::nullopt);

    SHOWINFO("Successfully created a swapchain");
    return swapchain;
}

Result<ComPtr<IDXGIFactory>> Direct3D::CreateFactory()
{
    unsigned int factoryFlags = 0;
#if DEBUG || _DEBUG
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    ComPtr<IDXGIFactory> factory;
    CHECK_HR(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)), std::nullopt);
    SHOWINFO("Successfully created a dxgi factory");
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
        CHECKCONT(SUCCEEDED(currentAdapter->GetDesc(&adapterDesc)),
                  "Unable to get description of adapter located at index {}", currentAdapterIndex);

        if (adapterDesc.DedicatedVideoMemory > maxVideoMemory)
        {
            maxVideoMemory = adapterDesc.DedicatedVideoMemory;
            bestAdapter = currentAdapter;
        }
    }

    CHECK(bestAdapter, std::nullopt, "Unable to find a suitable adapter");
    SHOWINFO("Successfully created a IDXGIAdapter");
    return bestAdapter;
}

Result<ComPtr<ID3D12Device>> Direct3D::CreateD3D12Device()
{
    ComPtr<ID3D12Device> device;
    CHECK_HR(D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)), std::nullopt);
    // CHECK(CheckFeatures(device.Get()), std::nullopt, "The found adapter is not good enough");
    SHOWINFO("Successfully created a ID3D12Device");
    return device;
}

template <D3D12_DESCRIPTOR_HEAP_TYPE heapType>
constexpr unsigned int Direct3D::GetDescriptorIncrementSize()
{
    static std::optional<unsigned int> incrementSize = std::nullopt;
    if (!incrementSize.has_value())
    {
        incrementSize = mDevice->GetDescriptorHandleIncrementSize(heapType);
    }
    return *incrementSize;
}

bool Direct3D::CheckFeatures(ID3D12Device* device)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 raytracingSupport;
    CHECK_HR(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &raytracingSupport, sizeof(raytracingSupport)), false);

    CHECK(raytracingSupport.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED, false, "Raytracing not working on this computer");

    return true;
}

bool Direct3D::UpdateDescriptors()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (uint32_t i = 0; i < kBufferCount; ++i)
    {
        CHECK_HR(mSwapchain->GetBuffer(i, IID_PPV_ARGS(&mSwapchainResources[i])), false);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;
        rtvDesc.Format = kBackbufferFormat;

        mDevice->CreateRenderTargetView(mSwapchainResources[i].Get(), &rtvDesc, cpuHandle);
        cpuHandle.Offset(1, GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>());
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.Format = kDepthStencilFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    mDevice->CreateDepthStencilView(mDepthStencilResource.Get(), &dsvDesc, mDSVHeap->GetCPUDescriptorHandleForHeapStart());

    SHOWINFO("Successfully updated {} descriptors", kBufferCount + 1);
    return true;
}
