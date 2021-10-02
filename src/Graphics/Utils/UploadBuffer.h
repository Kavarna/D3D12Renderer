#pragma once


#include <Oblivion.h>
#include "D3DObject.h"

template <typename T>
class UploadBuffer : public D3DObject
{
public:
    UploadBuffer() = default;
    ~UploadBuffer()
    {
        CHECKSHOW(Destroy(), "Unable to destroy an upload buffer");
    }

    bool Init(unsigned int elementCount = 1, bool isConstantBuffer = false)
    {
        CHECK(D3DObject::Init(), false, "Unable to initialize D3DObject");

        mElementCount = elementCount;
        mElementSize = sizeof(T);

        if (isConstantBuffer)
        {
            mElementSize = Math::AlignUp(mElementSize, 256);
        }

        mTotalSize = mElementSize * mElementCount;



        auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mTotalSize);
        CHECK_HR(mDevice->CreateCommittedResource(
            &uploadHeap, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&mMappedResource)), false);


        CHECK_HR(mMappedResource->Map(0, nullptr, &mMappedData), false);

        return true;
    }

    void CopyData(T* data, unsigned int index = 0)
    {
        memcpy((char *)mMappedData + mElementSize * index, &data, mElementSize);
    }

    T *GetMappedMemory(unsigned int index = 0)
    {
        return (T *)((char *)mMappedData + mElementSize * index);
    }

    ID3D12Resource *GetResource() const
    {
        return mMappedResource.Get();
    }

    // Use this as a shortcut
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return mMappedResource->GetGPUVirtualAddress();
    }

    unsigned int GetElementSize() const
    {
        return mElementSize;
    }

    unsigned int GetElementCount() const
    {
        return mElementCount;
    }

    unsigned int GetTotalSize() const
    {
        return mTotalSize;
    }

    bool Destroy()
    {
        mMappedResource->Unmap(0, nullptr);
        mMappedResource = nullptr;
        return true;
    }

private:
    unsigned int mElementSize;
    unsigned int mElementCount;
    unsigned int mTotalSize;

    ComPtr<ID3D12Resource> mMappedResource;

    void *mMappedData;
};

