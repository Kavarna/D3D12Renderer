#pragma once


#include "Oblivion.h"
#include "Model.h"

class Application
{
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
    bool OnRender();

private:
    bool InitD3D();
    bool InitModels();

private:
    void RenderModels();

private:
    HINSTANCE mInstance = nullptr;
    HWND mWindow = nullptr;

private:
    // D3D Objects
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    ComPtr<ID3D12Fence> mFence;
    std::vector<Model> mModels;

    uint64_t mCurrentFrame = 0;

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissors;
private:
    unsigned int mClientWidth = 800, mClientHeight = 600;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

};
