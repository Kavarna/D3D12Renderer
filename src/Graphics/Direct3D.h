#pragma once

#include "Oblivion.h"


class Direct3D
{
private:
    Direct3D() = default;
    ~Direct3D() = default;

public:
    static void Init();
    static Direct3D* GetInstance();
    static void Destroy();
    
private:
    void InternalInit();
    void InternalDestroy();

private:
    void InitFactory();
    void InitAdapter();
    void InitD3D12Device();

private:
    ComPtr<IDXGIFactory> mFactory;
    ComPtr<IDXGIAdapter> mAdapter;
    ComPtr<ID3D12Device> mDevice;
};


