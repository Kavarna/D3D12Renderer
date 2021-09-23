#pragma once

#include "Oblivion.h"
#include "ISingletone.h"

class Direct3D : public ISingletone<Direct3D>
{
    MAKE_SINGLETONE_CAPABLE(Direct3D);
private:
    Direct3D() = default;
    ~Direct3D() = default;
    
public:
    bool Init();

private:
    Result<ComPtr<IDXGIFactory>> CreateFactory();
    Result<ComPtr<IDXGIAdapter>> CreateAdapter();
    Result<ComPtr<ID3D12Device>> CreateD3D12Device();

private:
    ComPtr<IDXGIFactory> mFactory;
    ComPtr<IDXGIAdapter> mAdapter;
    ComPtr<ID3D12Device> mDevice;
};


