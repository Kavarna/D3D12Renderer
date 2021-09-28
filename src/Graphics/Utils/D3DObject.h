#pragma once


#include <Oblivion.h>
#include "../Direct3D.h"


class D3DObject
{
public:
    D3DObject()
    {
        Direct3D::Get()->GetD3D12Device();
    }

protected:
    ComPtr<ID3D12Device> mDevice;
};

