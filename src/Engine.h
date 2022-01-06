#pragma once


#include <Oblivion.h>
#include "Model.h"
#include "FrameResources.h"
#include "Camera.h"
#include "ThirdPersonCamera.h"
#include "SceneLight.h"
#include "BlurFilter.h"
#include "OrthographicCamera.h"

#include "Keyboard.h"
#include "Mouse.h"

class Engine
{
    static constexpr const auto kMINIMUM_WINDOW_SIZE = 200;
public:
    Engine();
    ~Engine() = default;

public:
    bool Init(HINSTANCE hInstance);
    void Run();

protected:
    virtual bool OnInit(ID3D12GraphicsCommandList *initializationCmdList, ID3D12CommandAllocator *cmdAllocator) = 0;
    virtual bool OnUpdate(FrameResources *frameResources, float dt) = 0;
    virtual bool OnRender(ID3D12GraphicsCommandList *cmdList, FrameResources* frameResources) = 0;
    virtual bool OnRenderGUI() = 0;
    virtual bool OnResize() = 0;
    virtual std::unordered_map<uuids::uuid, uint32_t> GetInstanceCount();

    virtual ID3D12PipelineState *GetBeginFramePipeline() = 0;

    virtual uint32_t GetModelCount() = 0;
    virtual uint32_t GetPassCount() = 0;

protected:
    std::unique_ptr<DirectX::Mouse> mMouse;
    std::unique_ptr<DirectX::Keyboard> mKeyboard;

    ComPtr<ID3D12Fence> mFence;
    uint64_t mCurrentFrame = 0;
    unsigned int mClientWidth = 800, mClientHeight = 600;

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
    bool InitFrameResources();
    bool InitImgui();

private:
    bool RenderGUI();

private:
    HINSTANCE mInstance = nullptr;
    HWND mWindow = nullptr;

private:
    // D3D Objects
    ComPtr<ID3D12CommandAllocator> mInitializationCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    ComPtr<ID3D12DescriptorHeap> mImguiDescriptorHeap;

    std::array<FrameResources, Direct3D::kBufferCount> mFrameResources;
    FrameResources *mCurrentFrameResource = nullptr;
    uint32_t mCurrentFrameResourceIndex = 0;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

};
