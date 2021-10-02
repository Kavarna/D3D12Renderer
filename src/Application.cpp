#include "Application.h"
#include "Direct3D.h"
#include "PipelineManager.h"

constexpr auto APPLICATION_NAME = TEXT("Game");
constexpr auto ENGINE_NAME = TEXT("Oblivion");
constexpr const char *CONFIG_FILE = "Oblivion.ini";


bool Application::Init(HINSTANCE hInstance)
{
    mInstance = hInstance;
    CHECK(InitWindow(), false, "Unable to initialize window");
    return true;
}

void Application::Run()
{
    CHECKRET(OnInit(), "Failed toinitialize application");

    ShowWindow(mWindow, SW_SHOWNORMAL);
    UpdateWindow(mWindow);

    MSG message;
    while (true)
    {
        if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                break;
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        else
        {
            CHECKBK(OnUpdate(), "Failed to update frame {}", mCurrentFrame);
            CHECKBK(OnRender(), "Failed to render frame {}", mCurrentFrame);
        }
    }

    OnDestroy();
}

bool Application::InitWindow()
{
    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.hCursor = LoadCursor(mInstance, IDC_ARROW);
    wndClass.hInstance = mInstance;
    wndClass.lpfnWndProc = Application::WndProc;
    wndClass.lpszClassName = ENGINE_NAME;
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    CHECK(RegisterClassEx(&wndClass), false, "Unbale to register class {}", ENGINE_NAME);

    int screenSizeX = GetSystemMetrics(SM_CXSCREEN);
    int screenSizeY = GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, (LONG)mClientWidth, (LONG)mClientHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int windowX = (int)std::max({ 0.0f, (screenSizeX - windowWidth) / 2.0f });
    int windowY = (int)std::max({ 0.0f, (screenSizeY - windowHeight) / 2.0f });

    mWindow = CreateWindow(ENGINE_NAME, APPLICATION_NAME,
                           WS_OVERLAPPEDWINDOW, windowX, windowY,
                           windowWidth, windowHeight, nullptr, nullptr, mInstance,
                           this);
    CHECK(mWindow, false, "Unable to create window");

    SHOWINFO("Created window with size {}x{}", mClientWidth, mClientHeight);
    return true;
}

bool Application::OnInit()
{
    auto d3d = Direct3D::Get();
    SHOWINFO("Started initializing application");

    CHECK(InitD3D(), false, "Unable to initialize D3D");
    CHECK(InitModels(), false, "Unable to load all models");
    CHECK(InitFrameResources(), false, "Unable to initialize Frame Resources");

    SHOWINFO("Finished initializing application");
    return true;
}

void Application::OnDestroy()
{
    auto d3d = Direct3D::Get();
    SHOWINFO("Started destroying application");

    d3d->Signal(mFence.Get(), mCurrentFrame);
    d3d->WaitForFenceValue(mFence.Get(), mCurrentFrame++);

    Model::Destroy();
    PipelineManager::Destroy();

    Direct3D::Destroy();
    SHOWINFO("Finished destroying application");
}

bool Application::OnResize(uint32_t width, uint32_t height)
{
    auto d3d = Direct3D::Get();
    d3d->Signal(mFence.Get(), mCurrentFrame);
    d3d->WaitForFenceValue(mFence.Get(), mCurrentFrame++);

    SHOWINFO("Window resized from {}x{} to {}x{}", mClientWidth, mClientHeight, width, height);
    mClientWidth = width, mClientHeight = height;

    mViewport.Width = (FLOAT)mClientWidth;
    mViewport.Height = (FLOAT)mClientHeight;
    mViewport.TopLeftX = 0;
    mViewport.TopLeftY = 0;
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;

    mScissors.left = 0;
    mScissors.top = 0;
    mScissors.right = mClientWidth;
    mScissors.bottom = mClientWidth;

    d3d->OnResize(width, height);

    return true;
}

bool Application::OnUpdate()
{
    auto d3d = Direct3D::Get();
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % Direct3D::kBufferCount;
    mCurrentFrameResource = &mFrameResources[mCurrentFrameResourceIndex];
    if (mCurrentFrameResource->FenceValue != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->FenceValue)
    {
        d3d->WaitForFenceValue(mFence.Get(), mCurrentFrameResource->FenceValue);
    }

    return true;
}

bool Application::OnRender()
{
    auto d3d = Direct3D::Get();

    auto pipelineResult = PipelineManager::Get()->GetPipeline(PipelineType::SimpleColor);
    CHECK(pipelineResult.Valid(), false, "Unable to retrieve pipeline and root signature");
    auto [pipeline, rootSignature] = pipelineResult.Get();

    CHECK_HR(mCurrentFrameResource->CommandAllocator->Reset(), false);
    CHECK_HR(mCommandList->Reset(mCurrentFrameResource->CommandAllocator.Get(), pipeline), false);

    d3d->OnRenderBegin(mCommandList.Get());    
    mCommandList->SetGraphicsRootSignature(rootSignature);

    Model::Bind(mCommandList.Get());

    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissors);

    // RenderModels();

    d3d->OnRenderEnd(mCommandList.Get());

    CHECK_HR(mCommandList->Close(), false);

    d3d->ExecuteCommandList(mCommandList.Get());
    d3d->Present();
    d3d->Signal(mFence.Get(), mCurrentFrame);
  
    mCurrentFrameResource->FenceValue = mCurrentFrame++;

    return true;
}

bool Application::InitD3D()
{
    auto d3d = Direct3D::Get();

    CHECK(d3d->Init(mWindow), false, "Unable to initialize D3D");
    CHECK(PipelineManager::Get()->Init(), false, "Unable to initialize pipeline manager");

    auto commandAllocator = d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
    CHECK(commandAllocator.Valid(), false, "Unable to create a direct command allocator");
    mInitializationCommandAllocator = commandAllocator.Get();


    auto commandList = d3d->CreateCommandList(mInitializationCommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    CHECK(commandList.Valid(), false, "Unable to create a command list from an command allocator");
    mCommandList = commandList.Get();
    CHECK_HR(mCommandList->Close(), false);

    auto fence = d3d->CreateFence(0);
    CHECK(fence.Valid(), false, "Unable to initialize fence with value 0");
    mFence = fence.Get();

    return true;
}

bool Application::InitModels()
{
    CHECK_HR(mInitializationCommandAllocator->Reset(), false);
    CHECK_HR(mCommandList->Reset(mInitializationCommandAllocator.Get(), nullptr), false);

    auto d3d = Direct3D::Get();
    mModels.emplace_back(Direct3D::kBufferCount, 0);
    CHECK(mModels.back().Create("Resources\\Suzanne.obj"), false, "Unable to load Suzanne");

    ComPtr<ID3D12Resource> intermediaryResources[2];
    CHECK(Model::InitBuffers(mCommandList.Get(), intermediaryResources), false, "Unable to initialize buffers for models");

    CHECK_HR(mCommandList->Close(), false);
    d3d->Flush(mCommandList.Get(), mFence.Get(), ++mCurrentFrame);

    return true;
}

bool Application::InitFrameResources()
{
    uint32_t numModels = (uint32_t)mModels.size();
    uint32_t numPasses = 1;

    for (unsigned int i = 0; i < mFrameResources.size(); ++i)
    {
        CHECK(mFrameResources[i].Init(numModels, numPasses), false, "Unable to init frame resource at index {}", i);
    }
    

    return true;
}

void Application::RenderModels()
{
    for (unsigned int i = 0; i < mModels.size(); ++i)
    {
        mCommandList->DrawIndexedInstanced(mModels[i].GetIndexCount(),
                                           1, // Number of instances
                                           mModels[i].GetStartIndexLocation(),
                                           mModels[i].GetBaseVertexLocation(),
                                           0); // Start InstanceLocation
    }
}

LRESULT Application::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static Application *app;
    switch (message)
    {
        case WM_CREATE:
        {
            CREATESTRUCT *createInfo = (CREATESTRUCT *)lParam;
            app = (Application *)createInfo->lpCreateParams;
            break;
        }
        case WM_SIZE:
        {
            uint32_t width = ((uint32_t)(short)LOWORD(lParam));
            uint32_t height = ((uint32_t)(short)HIWORD(lParam));
            if (!(width == 0 || height == 0))
            {
                app->OnResize(width, height);
            }
            break;
        }
        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMinTrackSize.x = kMINIMUM_WINDOW_SIZE;
            lpMMI->ptMinTrackSize.y = kMINIMUM_WINDOW_SIZE;
        }
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
