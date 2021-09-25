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
                  false, "Unable to create a descriptor heap with {} descriptors", kBufferCount);

    CHECK(UpdateDescriptors(), false, "Unable to update descriptors");

    SHOWINFO("Successfully initialized Direct3D");
    return true;
}

void Direct3D::OnRenderBegin(ID3D12GraphicsCommandList *cmdList)
{
    uint32_t activeBackBuffer = mSwapchain->GetCurrentBackBufferIndex();
    Transition(cmdList, mSwapchainResources[activeBackBuffer].Get(),
               D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(activeBackBuffer, GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>());
    FLOAT backgroundColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(rtvHandle, backgroundColor, 0, nullptr);
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

Result<ComPtr<ID3D12CommandAllocator>> Direct3D::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> result;
    CHECK_HR(mDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&result)), std::nullopt);
    return result;
}

Result<ComPtr<ID3D12GraphicsCommandList>> Direct3D::CreateCommandList(ID3D12CommandAllocator *allocator,
                                                                      D3D12_COMMAND_LIST_TYPE type, ID3D12PipelineState *pipeline)
{
    ComPtr<ID3D12GraphicsCommandList> result;
    CHECK_HR(mDevice->CreateCommandList(0, type, allocator, pipeline, IID_PPV_ARGS(&result)), std::nullopt);
    return result;
}

Result<ComPtr<ID3D12Fence>> Direct3D::CreateFence(uint64_t initialValue)
{
    ComPtr<ID3D12Fence> result;
    CHECK_HR(mDevice->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&result)), std::nullopt);
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

    return result;
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

    return bestAdapter;
}

Result<ComPtr<ID3D12Device>> Direct3D::CreateD3D12Device()
{
    ComPtr<ID3D12Device> device;
    CHECK_HR(D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)), std::nullopt);
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

    SHOWINFO("Successfully updated {} descriptors", kBufferCount);
    return true;
}
