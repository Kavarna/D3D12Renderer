#pragma once


#include <Oblivion.h>
#include "./Utils/D3DObject.h"


class Texture : public D3DObject
{
public:
    Texture() = default;

public:
    bool Init(ID3D12GraphicsCommandList *cmdList, const wchar_t *path, ComPtr<ID3D12Resource>& intermediary);

private:
    D3D12_RESOURCE_STATES mCurrentResourceState;
    D3D12_RESOURCE_DESC mDesc;
    ComPtr<ID3D12Resource> mResource;
};


