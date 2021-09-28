#pragma once

#include "Oblivion.h"
#include "ISingletone.h"

class Direct3D : public ISingletone<Direct3D>
{
    MAKE_SINGLETONE_CAPABLE(Direct3D);
public:
    static constexpr const auto kBufferCount = 3;
    static constexpr const auto kBackbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
private:
    Direct3D() = default;
    ~Direct3D() = default;
    
public:
    bool Init(HWND hwnd);

public:
    void OnRenderBegin(ID3D12GraphicsCommandList *cmdList);
    void OnRenderEnd(ID3D12GraphicsCommandList *cmdList);
    void Present();

public:
    Result<ComPtr<ID3D12CommandAllocator>> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type);
    Result<ComPtr<ID3D12GraphicsCommandList>> CreateCommandList(ID3D12CommandAllocator *allocator,
                                                                D3D12_COMMAND_LIST_TYPE type, ID3D12PipelineState *pipeline = nullptr);
    Result<ComPtr<ID3D12Fence>> CreateFence(uint64_t initialValue);
    Result<ComPtr<ID3D12DescriptorHeap>> CreateDescriptorHeap(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                              D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    Result<ComPtr<ID3D12PipelineState>> CreatePipelineSteate(const D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc);
    Result<ComPtr<ID3D12RootSignature>> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC &desc);

    ComPtr<ID3D12Device> GetD3D12Device();

public:
    bool AllowTearing();

    void Transition(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *resource, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_STATES finalState);

    void Signal(ID3D12Fence *fence, uint64_t value);
    void WaitForFenceValue(ID3D12Fence *fence, uint64_t value);
    void ExecuteCommandList(ID3D12GraphicsCommandList *cmdList);
    void Flush(ID3D12GraphicsCommandList *cmdList, ID3D12Fence *fence, uint64_t value);

    template <D3D12_DESCRIPTOR_HEAP_TYPE heapType>
    constexpr unsigned int GetDescriptorIncrementSize();

private:
    Result<ComPtr<ID3D12CommandQueue>> CreateCommandQueue(D3D12_COMMAND_LIST_TYPE queueType);
    Result<ComPtr<IDXGISwapChain4>> CreateSwapchain(HWND hwnd);

private:
    Result<ComPtr<IDXGIFactory>> CreateFactory();
    Result<ComPtr<IDXGIAdapter>> CreateAdapter();
    Result<ComPtr<ID3D12Device>> CreateD3D12Device();

private:
    bool UpdateDescriptors();

private:
    ComPtr<IDXGIFactory> mFactory;
    ComPtr<IDXGIAdapter> mAdapter;
    ComPtr<ID3D12Device> mDevice;

    ComPtr<ID3D12CommandQueue> mDirectCommandQueue;

    ComPtr<IDXGISwapChain4> mSwapchain;

    ComPtr<ID3D12DescriptorHeap> mRTVHeap;

    std::array<ComPtr<ID3D12Resource>, kBufferCount> mSwapchainResources;

    bool mVsync = true;
};


template constexpr unsigned int Direct3D::GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>();
template constexpr unsigned int Direct3D::GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>();
template constexpr unsigned int Direct3D::GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>();
template constexpr unsigned int Direct3D::GetDescriptorIncrementSize<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>();
