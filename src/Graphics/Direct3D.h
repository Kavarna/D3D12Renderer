#pragma once

#include "Oblivion.h"
#include "ISingletone.h"

class Direct3D : public ISingletone<Direct3D>
{
    MAKE_SINGLETONE_CAPABLE(Direct3D);
    static constexpr const auto kBufferCount = 3;
    static constexpr const auto kBackbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
private:
    Direct3D() = default;
    ~Direct3D() = default;
    
public:
    bool Init(HWND hwnd);

public:
    Result<ComPtr<ID3D12CommandAllocator>> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type);
    Result<ComPtr<ID3D12GraphicsCommandList>> CreateCommandList(ID3D12CommandAllocator *allocator,
                                                                D3D12_COMMAND_LIST_TYPE type, ID3D12PipelineState *pipeline = nullptr);
    Result<ComPtr<ID3D12Fence>> CreateFence(uint64_t initialValue);

public:
    bool AllowTearing();

private:
    Result<ComPtr<ID3D12CommandQueue>> CreateCommandQueue(D3D12_COMMAND_LIST_TYPE queueType);
    Result<ComPtr<IDXGISwapChain>> CreateSwapchain(HWND hwnd);

private:
    Result<ComPtr<IDXGIFactory>> CreateFactory();
    Result<ComPtr<IDXGIAdapter>> CreateAdapter();
    Result<ComPtr<ID3D12Device>> CreateD3D12Device();

private:
    ComPtr<IDXGIFactory> mFactory;
    ComPtr<IDXGIAdapter> mAdapter;
    ComPtr<ID3D12Device> mDevice;

    ComPtr<ID3D12CommandQueue> mDirectCommandQueue;

    ComPtr<IDXGISwapChain> mSwapchain;
};


