#include "Application.h"
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


Application::Application() : 
    mSceneLight((unsigned int)mFrameResources.size())
{
}

bool Application::Init(HINSTANCE hInstance)
{
    SHOWINFO("Started initializing application. Version = {}", VERSION);
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
    CHECK(InitInput(), false, "Unable to initialize input");
    CHECK(InitModels(), false, "Unable to load all models");
    CHECK(InitFrameResources(), false, "Unable to initialize Frame Resources");
    CHECK(InitImgui(), false, "Unable to initialize imgui");

    mSceneLight.SetAmbientColor(0.2f, 0.2f, 0.2f, 1.0f);
    // mSceneLight.AddPointLight("MyPointLight", { 0.0f, 5.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f, 30.0f);
    mSceneLight.AddDirectionalLight("Sun", { 0.0f,-1.0f, 1.0f }, { 0.6f, 0.6f, 0.6f });

    SHOWINFO("Finished initializing application");
    return true;
}

void Application::OnDestroy()
{
    auto d3d = Direct3D::Get();
    SHOWINFO("Started destroying application");

    d3d->Signal(mFence.Get(), mCurrentFrame);
    d3d->WaitForFenceValue(mFence.Get(), mCurrentFrame++);

    TextureManager::Destroy();
    Model::Destroy();
    PipelineManager::Destroy();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

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
    mBlurViewport = mViewport;
    mBlurViewport.Width /= (float)FrameResources::kBlurScale;
    mBlurViewport.Height /= (float)FrameResources::kBlurScale;;

    mScissors.left = 0;
    mScissors.top = 0;
    mScissors.right = mClientWidth;
    mScissors.bottom = mClientWidth;
    mBlurScissors = mScissors;
    mBlurScissors.right /= FrameResources::kBlurScale;
    mBlurScissors.bottom /= FrameResources::kBlurScale;

    mCamera.Create(mCamera.GetPosition(), (float)mClientWidth / mClientHeight);

    d3d->OnResize(width, height);

    return true;
}

bool Application::OnUpdate()
{
    float dt = 1.0f / ImGui::GetIO().Framerate;

    auto d3d = Direct3D::Get();
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % Direct3D::kBufferCount;
    mCurrentFrameResource = &mFrameResources[mCurrentFrameResourceIndex];
    if (mCurrentFrameResource->FenceValue != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->FenceValue)
    {
        d3d->WaitForFenceValue(mFence.Get(), mCurrentFrameResource->FenceValue);
    }

    ReactToKeyPresses(dt);
    UpdateModels();
    UpdatePassBuffers();
    MaterialManager::Get()->UpdateMaterialsBuffer(mCurrentFrameResource->MaterialsBuffers);
    mSceneLight.UpdateLightsBuffer(mCurrentFrameResource->LightsBuffer);

    // mModels[0].RotateY(0.01f);
    // mModels[1].RotateY(-0.01f);

    return true;
}

bool Application::OnRender()
{
    auto d3d = Direct3D::Get();
    auto textureManager = TextureManager::Get();

    FLOAT backgroundColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    auto pipelineResult = PipelineManager::Get()->GetPipeline(PipelineType::MaterialLight);
    CHECK(pipelineResult.Valid(), false, "Unable to retrieve pipeline and root signature");
    auto [pipeline, rootSignature] = pipelineResult.Get();

    CHECK_HR(mCurrentFrameResource->CommandAllocator->Reset(), false);
    CHECK_HR(mCommandList->Reset(mCurrentFrameResource->CommandAllocator.Get(), pipeline), false);

    d3d->OnRenderBegin(mCommandList.Get());

    // Render to texture
    textureManager->Transition(mCommandList.Get(), mCurrentFrameResource->BlurRenderTargetIndex, D3D12_RESOURCE_STATE_RENDER_TARGET);

    mCommandList->RSSetViewports(1, &mBlurViewport);
    mCommandList->RSSetScissorRects(1, &mBlurScissors);

    auto rtvHandleResult = TextureManager::Get()->GetCPUDescriptorRtvHandleForTextureIndex(mCurrentFrameResource->BlurRenderTargetIndex);
    CHECK(rtvHandleResult.Valid(), false, "Unable to get rtv handle for texture index {}", mCurrentFrameResource->BlurRenderTargetIndex);
    auto rtvHandle = rtvHandleResult.Get();

    auto dsvHandleResult = TextureManager::Get()->GetCPUDescriptorDsvHandleForTextureIndex(mCurrentFrameResource->BlurDepthStencilIndex);
    CHECK(dsvHandleResult.Valid(), false, "Unable to get dsv handle for texture index {}", mCurrentFrameResource->BlurDepthStencilIndex);
    auto dsvHandle = dsvHandleResult.Get();

    mCommandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);
    mCommandList->ClearRenderTargetView(rtvHandle, backgroundColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    mCommandList->SetGraphicsRootSignature(rootSignature);

    Model::Bind(mCommandList.Get());

    RenderModels();

    textureManager->Transition(mCommandList.Get(), mCurrentFrameResource->BlurRenderTargetIndex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Render to backbuffer
    D3D12_CPU_DESCRIPTOR_HANDLE backbufferHandle = d3d->GetBackbufferHandle();
    mCommandList->OMSetRenderTargets(1, &backbufferHandle, TRUE, nullptr);
    mCommandList->ClearRenderTargetView(backbufferHandle, backgroundColor, 0, nullptr);

    auto rawTexturePipelineResult = PipelineManager::Get()->GetPipeline(PipelineType::RawTexture);
    CHECK(rawTexturePipelineResult.Valid(), false, "Unable to retrieve pipeline and root signature for raw texture");
    std::tie(pipeline, rootSignature) = rawTexturePipelineResult.Get();

    mCommandList->SetPipelineState(pipeline);
    mCommandList->SetGraphicsRootSignature(rootSignature);

    Model::Bind(mCommandList.Get());
    mCommandList->SetDescriptorHeaps(1, textureManager->GetSrvUavDescriptorHeap().GetAddressOf());

    auto srvCpuHandleResult = textureManager->GetGPUDescriptorSrvHandleForTextureIndex(mCurrentFrameResource->BlurRenderTargetIndex);
    CHECK(srvCpuHandleResult.Valid(), false, "Unable to get srv handle for texture index {}", mCurrentFrameResource->BlurRenderTargetIndex);
    auto srvCpuHandle = srvCpuHandleResult.Get();
    mCommandList->SetGraphicsRootDescriptorTable(0, srvCpuHandle);

    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissors);

    mCommandList->DrawIndexedInstanced(
        mSquare.GetIndexCount(), 1, mSquare.GetStartIndexLocation(), mSquare.GetBaseVertexLocation(), 0);

    RenderGUI();

    d3d->OnRenderEnd(mCommandList.Get());

    CHECK_HR(mCommandList->Close(), false);

    d3d->ExecuteCommandList(mCommandList.Get());
    d3d->Present();
    d3d->Signal(mFence.Get(), mCurrentFrame);
  
    mCurrentFrameResource->FenceValue = ++mCurrentFrame;

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

bool Application::InitInput()
{
    mKeyboard = std::make_unique<DirectX::Keyboard>();
    mMouse = std::make_unique<DirectX::Mouse>();
    mMouse->SetWindow(mWindow);
    mMouse->SetVisible(true);
    mMouse->SetMode(DirectX::Mouse::Mode::MODE_ABSOLUTE);

    return true;
}

bool Application::InitModels()
{
    CHECK_HR(mInitializationCommandAllocator->Reset(), false);
    CHECK_HR(mCommandList->Reset(mInitializationCommandAllocator.Get(), nullptr), false);

    auto d3d = Direct3D::Get();
    mModels.emplace_back(Direct3D::kBufferCount, 0);
    CHECK(mModels.back().Create("Resources\\Suzanne.obj"), false, "Unable to load Suzanne");
    mModels.back().Translate(2.0f, 0.0f, 0.0f);

    mModels.emplace_back(Direct3D::kBufferCount, 1);
    CHECK(mModels.back().Create("Resources\\Cube.obj"), false, "Unable to load Cube");
    mModels.back().Translate(-2.0f, 0.0f, 0.0f);

    CHECK(mSquare.Create(Direct3D::kBufferCount, 2, Model::ModelType::Square), false, "Unable to create square");

    ComPtr<ID3D12Resource> intermediaryResources[2];
    CHECK(Model::InitBuffers(mCommandList.Get(), intermediaryResources), false, "Unable to initialize buffers for models");

    mCamera.Create({ 0.0f, 0.0f, -3.0f }, (float)mClientWidth / mClientHeight);

    CHECK_HR(mCommandList->Close(), false);
    d3d->Flush(mCommandList.Get(), mFence.Get(), ++mCurrentFrame);

    return true;
}

bool Application::InitFrameResources()
{
    auto d3d = Direct3D::Get();

    CHECK_HR(mInitializationCommandAllocator->Reset(), false);
    CHECK_HR(mCommandList->Reset(mInitializationCommandAllocator.Get(), nullptr), false);

    MaterialManager::Get()->CloseAddingMaterials();
    uint32_t numModels = (uint32_t)mModels.size();
    uint32_t numPasses = 1;
    uint32_t numMaterials = MaterialManager::Get()->GetNumMaterials();

    for (unsigned int i = 0; i < mFrameResources.size(); ++i)
    {
        CHECK(mFrameResources[i].Init(numModels, numPasses, numMaterials, mClientWidth, mClientHeight),
              false, "Unable to init frame resource at index {}", i);
    }

    std::vector<ComPtr<ID3D12Resource>> temporaryResources;
    CHECK(TextureManager::Get()->CloseAddingTextures(mCommandList.Get(), temporaryResources), false, "Unable to load all textures");

    CHECK_HR(mCommandList->Close(), false);
    d3d->Flush(mCommandList.Get(), mFence.Get(), ++mCurrentFrame);

    return true;
}

bool Application::InitImgui()
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

void Application::ReactToKeyPresses(float dt)
{
    auto kb = mKeyboard->GetState();
    auto mouse = mMouse->GetState();

    if (kb.Escape)
    {
        PostQuitMessage(0);
    }

    if (!mMenuActive)
    {
        if (kb.W)
        {
            mCamera.MoveForward(dt);
        }
        if (kb.S)
        {
            mCamera.MoveBackward(dt);
        }
        if (kb.D)
        {
            mCamera.MoveRight(dt);
        }
        if (kb.A)
        {
            mCamera.MoveLeft(dt);
        }

        mouse.x = Math::clamp(mouse.x, -25, 25);
        mouse.y = Math::clamp(mouse.y, -25, 25);
        mCamera.Update(dt, (float)mouse.x, (float)mouse.y);
    }
    else
    {
        mCamera.Update(dt, 0.0f, 0.0f);
    }

    static bool bRightClick = false;
    if (mouse.rightButton && !bRightClick)
    {
        bRightClick = true;
        if (mMenuActive)
        {
            mMouse->SetMode(DirectX::Mouse::Mode::MODE_RELATIVE);
            while (ShowCursor(FALSE) > 0);
        }
        else
        {
            mMouse->SetMode(DirectX::Mouse::Mode::MODE_ABSOLUTE);
            while (ShowCursor(TRUE) <= 0);
        }
        mMenuActive = !mMenuActive;
    }
    else if (!mouse.rightButton)
        bRightClick = false;
}

void Application::UpdateModels()
{
    for (auto &model : mModels)
    {
        if (model.DirtyFrames > 0)
        {
            auto mappedMemory = mCurrentFrameResource->PerObjectBuffers.GetMappedMemory(model.ConstantBufferIndex);
            mappedMemory->World = DirectX::XMMatrixTranspose(model.GetWorld());
            mappedMemory->TexWorld = DirectX::XMMatrixTranspose(model.GetTexWorld());
            model.DirtyFrames--;
        }
    }
}

void Application::UpdatePassBuffers()
{
    if (mCamera.DirtyFrames > 0)
    {
        auto mappedMemory = mCurrentFrameResource->PerPassBuffers.GetMappedMemory();
        mappedMemory->View = DirectX::XMMatrixTranspose(mCamera.GetView());
        mappedMemory->Projection = DirectX::XMMatrixTranspose(mCamera.GetProjection());

        mCamera.DirtyFrames--;
    }
}

void Application::RenderModels()
{
    auto textureManager = TextureManager::Get();

    mCommandList->SetGraphicsRootConstantBufferView(1, mCurrentFrameResource->PerPassBuffers.GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootConstantBufferView(3, mCurrentFrameResource->LightsBuffer.GetGPUVirtualAddress());

    mCommandList->SetDescriptorHeaps(1, textureManager->GetSrvUavDescriptorHeap().GetAddressOf());

    for (unsigned int i = 0; i < mModels.size(); ++i)
    {
        auto perObjectBufferAddress = mCurrentFrameResource->PerObjectBuffers.GetGPUVirtualAddress();
        perObjectBufferAddress += mModels[i].ConstantBufferIndex * mCurrentFrameResource->PerObjectBuffers.GetElementSize();
        mCommandList->SetGraphicsRootConstantBufferView(0, perObjectBufferAddress);

        auto materialBufferAddress = mCurrentFrameResource->MaterialsBuffers.GetGPUVirtualAddress();
        const auto* objectMaterial = mModels[i].GetMaterial();
        materialBufferAddress += (uint64_t)objectMaterial->ConstantBufferIndex * mCurrentFrameResource->MaterialsBuffers.GetElementSize();
        mCommandList->SetGraphicsRootConstantBufferView(2, materialBufferAddress);

        if (objectMaterial->GetTextureIndex() != -1)
        {
            auto textureSRVResult = textureManager->GetGPUDescriptorSrvHandleForTextureIndex(objectMaterial->GetTextureIndex());
            mCommandList->SetGraphicsRootDescriptorTable(
                4, textureSRVResult.Get());
        }
        mCommandList->DrawIndexedInstanced(mModels[i].GetIndexCount(),
                                           1, // Number of instances
                                           mModels[i].GetStartIndexLocation(),
                                           mModels[i].GetBaseVertexLocation(),
                                           0); // Start InstanceLocation
    }
}

void Application::RenderGUI()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(-1, -1));
    ImGui::Begin("Debug info", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::Text("Frametime: %f (%.2f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    mCommandList->SetDescriptorHeaps(1, mImguiDescriptorHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());
}

LRESULT Application::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static Application *app;

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
