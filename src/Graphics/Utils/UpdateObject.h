#pragma once



class UpdateObject
{
public:
    UpdateObject() = default;
    UpdateObject(unsigned int maxDirtyFrames, unsigned int constantBufferIndex = 0):
        mMaxDirtyFrames(maxDirtyFrames), ConstantBufferIndex(constantBufferIndex)
    {
        DirtyFrames = mMaxDirtyFrames;
    }

    void Init(unsigned int maxDirtyFrames, unsigned int constantBufferIndex = 0)
    {
        mMaxDirtyFrames = maxDirtyFrames;
        ConstantBufferIndex = ConstantBufferIndex;
        DirtyFrames = mMaxDirtyFrames;
    }

    void MarkUpdate()
    {
        DirtyFrames = mMaxDirtyFrames;
    }

public:
    unsigned int DirtyFrames;
    unsigned int ConstantBufferIndex;

private:
    unsigned int mMaxDirtyFrames;
};


