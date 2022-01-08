#include "Engine.h"
#include "Direct3D.h"
#include "PipelineManager.h"
#include "TextureManager.h"

// Imgui stuff
#include "Graphics/imgui/imgui.h"
#include "Graphics/imgui/imgui_impl_win32.h"
#include "Graphics/imgui/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

constexpr auto APPLICATION_NAME = TEXT("Game");
constexpr auto ENGINE_NAME = TEXT("Oblivion");
constexpr const char *CONFIG_FILE = "Oblivion.ini";


Engine::Engine()
{
}

bool Engine::Init(HINSTANCE hInstance)
{
    SHOWINFO("Started initializing application. Version = {}", VERSION);
    mInstance = hInstance;
    CHECK(InitWindow(), false, "Unable to initialize window");
    return true;
}

void Engine::Run()
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
            // SHOWINFO("~~~~~~~~~~~~~~~~~~~~ FRAME {} ~~~~~~~~~~~~~~~~~~~~", mCurrentFrame);
            CHECKBK(OnUpdate(), "Failed to update frame {}", mCurrentFrame);
            CHECKBK(OnRender(), "Failed to render frame {}", mCurrentFrame);
            // SHOWINFO("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        }
    }

    OnDestroy();
}

bool Engine::InitWindow()
{
    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.hCursor = LoadCursor(mInstance, IDC_ARROW);
    wndClass.hInstance = mInstance;
    wndClass.lpfnWndProc = Engine::WndProc;
    wndClass.lpszClassName = ENGINE_NAME;
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    CHECK(RegisterClassEx(&wndClass), false, "Unbale to register class {}", ENGINE_NAME);

    int screenSizeX = GetSystemMetrics(SM_CXSCREEN);
    int screenSizeY = GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, (LONG)mClientWidth, (LONG)mClientHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int windowX = (int)std::max({ 0.0f, (screenSizeX - windowWidth) / 2.0f });
    int windowY = (int)std::max({ 0.0f, (screenSizeY - windowHeight) / 2.0f });

    mWindow = CreateWindow(ENGINE_NAME, APPLICATION_NAME,
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, windowX, windowY,
                           windowWidth, windowHeight, nullptr, nullptr, mInstance,
                           this);
    CHECK(mWindow, false, "Unable to create window");

    SHOWINFO("Created window with size {}x{}", mClientWidth, mClientHeight);
    return true;
}

bool Engine::OnInit()
{
    auto d3d = Direct3D::Get();
    SHOWINFO("Started initializing application");

    CHECK(InitD3D(), false, "Unable to initialize D3D");
    CHECK(InitInput(), false, "Unable to initialize input");
    
    CHECK(OnInit(mCommandList.Get(), mInitializationCommandAllocator.Get()), false, "Unable to initialize application");
    
    CHECK(InitFrameResources(), false, "Unable to initialize Frame Resources");
    CHECK(InitImgui(), false, "Unable to initialize imgui");

    SHOWINFO("Finished initializing application");
    return true;
}

void Engine::OnDestroy()
{
    auto d3d = Direct3D::Get();
    SHOWINFO("Started destroying application");

    d3d->Signal(mFence.Get(), mCurrentFrame);
    d3d->WaitForFenceValue(mFence.Get(), mCurrentFrame++);

    OnClose();

    TextureManager::Destroy();
    Model::Destroy();
    PipelineManager::Destroy();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    Direct3D::Destroy();
    SHOWINFO("Finished destroying application");
}

bool Engine::OnResize(uint32_t width, uint32_t height)
{
    auto d3d = Direct3D::Get();
    d3d->Signal(mFence.Get(), mCurrentFrame);
    d3d->WaitForFenceValue(mFence.Get(), mCurrentFrame++);

    SHOWINFO("Window resized from {}x{} to {}x{}", mClientWidth, mClientHeight, width, height);
    mClientWidth = width, mClientHeight = height;


    for (auto& frameResource : mFrameResources)
    {
        CHECKCONT(frameResource.OnResize(width, height), "Unable to resize frame resources");
    }

    CHECK(OnResize(), false, "Unable to resize application");

    d3d->OnResize(width, height);

    return true;
}

bool Engine::OnUpdate()
{
    float dt = 1.0f / ImGui::GetIO().Framerate;
    if (ImGui::GetIO().Framerate == 0.0f)
    {
        dt = 0.0f;
    }

    auto d3d = Direct3D::Get();
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % Direct3D::kBufferCount;
    mCurrentFrameResource = &mFrameResources[mCurrentFrameResourceIndex];
    if (mCurrentFrameResource->FenceValue != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->FenceValue)
    {
        d3d->WaitForFenceValue(mFence.Get(), mCurrentFrameResource->FenceValue);
    }

    MaterialManager::Get()->UpdateMaterialsBuffer(mCurrentFrameResource->MaterialsBuffers);

    CHECK(OnUpdate(mCurrentFrameResource, dt), false, "Unable to update frame {}", mCurrentFrame);


    return true;
}

bool Engine::OnRender()
{
    auto d3d = Direct3D::Get();
    auto textureManager = TextureManager::Get();

    CHECK_HR(mCurrentFrameResource->CommandAllocator->Reset(), false);
    CHECK_HR(mCommandList->Reset(mCurrentFrameResource->CommandAllocator.Get(), GetBeginFramePipeline()), false);

    d3d->OnRenderBegin(mCommandList.Get());

    CHECK(OnRender(mCommandList.Get(), mCurrentFrameResource), false, "Unable to render frame");
    CHECK(RenderGUI(), false, "Failed to render GUI");

    d3d->OnRenderEnd(mCommandList.Get());

    CHECK_HR(mCommandList->Close(), false);

    d3d->ExecuteCommandList(mCommandList.Get());
    d3d->Present();
    d3d->Signal(mFence.Get(), mCurrentFrame);
  
    mCurrentFrameResource->FenceValue = ++mCurrentFrame;

    return true;
}

bool Engine::InitD3D()
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
    mCommandList->SetName(L"Engine command list");
    CHECK_HR(mCommandList->Close(), false);

    auto fence = d3d->CreateFence(0);
    CHECK(fence.Valid(), false, "Unable to initialize fence with value 0");
    mFence = fence.Get();

    return true;
}

bool Engine::InitInput()
{
    mKeyboard = std::make_unique<DirectX::Keyboard>();
    mMouse = std::make_unique<DirectX::Mouse>();
    mMouse->SetWindow(mWindow);
    mMouse->SetVisible(true);
    mMouse->SetMode(DirectX::Mouse::Mode::MODE_ABSOLUTE);

    return true;
}

bool Engine::InitFrameResources()
{
    auto d3d = Direct3D::Get();

    CHECK_HR(mInitializationCommandAllocator->Reset(), false);
    CHECK_HR(mCommandList->Reset(mInitializationCommandAllocator.Get(), nullptr), false);

    MaterialManager::Get()->CloseAddingMaterials();
    uint32_t numModels = GetModelCount();
    uint32_t numPasses = GetPassCount();
    uint32_t numMaterials = MaterialManager::Get()->GetNumMaterials();
    auto instances = GetInstanceCount();

    for (unsigned int i = 0; i < mFrameResources.size(); ++i)
    {
        CHECK(mFrameResources[i].Init(numModels, numPasses, numMaterials, mClientWidth, mClientHeight, instances),
              false, "Unable to init frame resource at index {}", i);
    }


    std::vector<ComPtr<ID3D12Resource>> temporaryResources;
    CHECK(TextureManager::Get()->CloseAddingTextures(mCommandList.Get(), temporaryResources), false, "Unable to load all textures");

    CHECK_HR(mCommandList->Close(), false);
    d3d->Flush(mCommandList.Get(), mFence.Get(), ++mCurrentFrame);

    return true;
}

bool Engine::InitImgui()
{
    auto d3d = Direct3D::Get();

    auto descriptorHeap = d3d->CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    CHECK(descriptorHeap.Valid(), false, "Unable to initialize imgui descriptor heap");
    mImguiDescriptorHeap = descriptorHeap.Get();;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(mWindow);
    ImGui_ImplDX12_Init(d3d->GetD3D12Device().Get(), Direct3D::kBufferCount,
                        Direct3D::kBackbufferFormat, mImguiDescriptorHeap.Get(),
                        mImguiDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                        mImguiDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    return true;
}

bool Engine::RenderGUI()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    CHECK(OnRenderGUI(), false, "Unable to render GUI");

    ImGui::SetNextWindowPos(ImVec2(-1, -1));
    ImGui::Begin("Debug info", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::Text("Frametime: %f (%.2f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    mCommandList->SetDescriptorHeaps(1, mImguiDescriptorHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

    return true;
}

LRESULT Engine::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static Engine *app;

    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
        return true;

    switch (message)
    {
        case WM_ACTIVATEAPP:
            DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
            DirectX::Mouse::ProcessMessage(message, wParam, lParam);
            break;

        case WM_MOUSEMOVE:
        case WM_INPUT:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEHOVER:
            DirectX::Mouse::ProcessMessage(message, wParam, lParam);
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
            break;

        case WM_CREATE:
        {
            CREATESTRUCT *createInfo = (CREATESTRUCT *)lParam;
            app = (Engine *)createInfo->lpCreateParams;
            break;
        }
        case WM_SIZE:
        {
            uint32_t width = ((uint32_t)(short)LOWORD(lParam));
            uint32_t height = ((uint32_t)(short)HIWORD(lParam));
            if (!(width == 0 || height == 0))
            {
                if (!app->OnResize(width, height))
                {
                    SHOWFATAL("Failed to resize window");
                    PostQuitMessage(0);
                }
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

std::unordered_map<uuids::uuid, uint32_t> Engine::GetInstanceCount()
{
    return std::unordered_map<uuids::uuid, uint32_t>();
}
