#pragma once


#include <Oblivion.h>
#include "../Direct3D.h"


class D3DObject
{
public:
    D3DObject() = default;

    bool Init()
    {
        mDevice = Direct3D::Get()->GetD3D12Device();
        return true;
    }

protected:
    ComPtr<ID3D12Device> mDevice;
};

