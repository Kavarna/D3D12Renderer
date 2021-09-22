#include "Oblivion.h"
#include "Application.h"
#include "Direct3D.h"


constexpr auto APPLICATION_NAME = TEXT("Game");
constexpr auto ENGINE_NAME = TEXT("Oblivion");
constexpr const char *CONFIG_FILE = "Oblivion.ini";


void Application::Init(HINSTANCE hInstance)
{
    mInstance = hInstance;
    InitWindow();
}

void Application::Run()
{
    OnInit();

    ShowWindow(mWindow, SW_SHOWNORMAL);
    CHECK(UpdateWindow(mWindow)) << "Unable to update window";

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

        }
    }

    OnDestroy();
}

void Application::InitWindow()
{
    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.hCursor = LoadCursor(mInstance, IDC_ARROW);
    wndClass.hInstance = mInstance;
    wndClass.lpfnWndProc = Application::WndProc;
    wndClass.lpszClassName = ENGINE_NAME;
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    CHECK(RegisterClassEx(&wndClass)) << "Unable to register class" << ENGINE_NAME;

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
                           windowWidth, windowHeight, nullptr, nullptr, mInstance, nullptr);
    CHECK(mWindow != nullptr) << "Unable to create window";

    LOG(INFO) << "Created window with size (" << mClientWidth << "x" << mClientHeight << ")";
}

void Application::OnInit()
{
    LOG(INFO) << "Started initializing application";
    
    Direct3D::Init();

    LOG(INFO) << "Finished initializing application";
}

void Application::OnDestroy()
{
    LOG(INFO) << "Started destroying application";

    Direct3D::Destroy();

    LOG(INFO) << "Finished destroying application";
}

LRESULT Application::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
