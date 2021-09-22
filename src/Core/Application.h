#pragma once


#include "Oblivion.h"


class Application
{
public:
    Application() = default;
    ~Application() = default;

public:
    bool Init(HINSTANCE hInstance);
    void Run();

private:
    void InitWindow();

private:
    void OnCreate();
    void OnDestroy();

private:
    HINSTANCE mInstance = nullptr;
    HWND mWindow = nullptr;

private:
    unsigned int mClientWidth = 800, mClientHeight = 600;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

};
