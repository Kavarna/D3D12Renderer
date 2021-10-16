#pragma once


#include <Oblivion.h>
#include "Model.h"
#include "FrameResources.h"
#include "Camera.h"
#include "SceneLight.h"

#include "Keyboard.h"
#include "Mouse.h"

class Application
{
    static constexpr const auto kMINIMUM_WINDOW_SIZE = 200;
public:
    Application();
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
    bool InitInput();
    bool InitModels();
    bool InitFrameResources();
    bool InitImgui();

private:
    void ReactToKeyPresses(float dt);
    void UpdateModels();
    void UpdatePassBuffers();
    void RenderModels();
    void RenderGUI();

private:
    HINSTANCE mInstance = nullptr;
    HWND mWindow = nullptr;

private:
    // D3D Objects
    ComPtr<ID3D12CommandAllocator> mInitializationCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    ComPtr<ID3D12DescriptorHeap> mImguiDescriptorHeap;

    Camera mCamera;

    ComPtr<ID3D12Fence> mFence;
    std::vector<Model> mModels;

    std::array<FrameResources, Direct3D::kBufferCount> mFrameResources;
    FrameResources *mCurrentFrameResource;
    uint32_t mCurrentFrameResourceIndex = 0;

    SceneLight mSceneLight;

    uint64_t mCurrentFrame = 0;

    D3D12_VIEWPORT mViewport, mBlurViewport;
    D3D12_RECT mScissors, mBlurScissors;
private:
    unsigned int mClientWidth = 800, mClientHeight = 600;
    bool mMenuActive = true;

private:
    std::unique_ptr<DirectX::Mouse> mMouse;
    std::unique_ptr<DirectX::Keyboard> mKeyboard;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

};
