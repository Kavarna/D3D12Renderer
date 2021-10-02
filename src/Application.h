#pragma once


#include "Oblivion.h"
#include "Model.h"
#include "FrameResources.h"

class Application
{
    static constexpr const auto kMINIMUM_WINDOW_SIZE = 200;
public:
    Application() = default;
    ~Application() = default;

public:
    bool Init(HINSTANCE hInstance);
    void Run();

private:
    bool InitWindow();

private:
    bool OnInit();
    void OnDestroy();
    bool OnResize(uint32_t width, uint32_t height);
    bool OnUpdate();
    bool OnRender();

private:
    bool InitD3D();
    bool InitModels();
    bool InitFrameResources();

private:
    void RenderModels();

private:
    HINSTANCE mInstance = nullptr;
    HWND mWindow = nullptr;

private:
    // D3D Objects
    ComPtr<ID3D12CommandAllocator> mInitializationCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    ComPtr<ID3D12Fence> mFence;
    std::vector<Model> mModels;

    std::array<FrameResources, Direct3D::kBufferCount> mFrameResources;
    FrameResources *mCurrentFrameResource;
    uint32_t mCurrentFrameResourceIndex = 0;

    uint64_t mCurrentFrame = 0;

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissors;
private:
    unsigned int mClientWidth = 800, mClientHeight = 600;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

};
