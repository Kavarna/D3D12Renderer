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
        ConstantBufferIndex = constantBufferIndex;
        DirtyFrames = mMaxDirtyFrames;
    }

    void MarkUpdate()
    {
        DirtyFrames = mMaxDirtyFrames;
    }

    unsigned int GetMaxDirtyFrames() const
    {
        return mMaxDirtyFrames;
    }

public:
    unsigned int DirtyFrames = (unsigned int)-1;
    unsigned int ConstantBufferIndex = (unsigned int)-1;

protected:
    bool Valid()
    {
        return (mMaxDirtyFrames != -1 && DirtyFrames != -1 && ConstantBufferIndex != -1);
    }

private:
    unsigned int mMaxDirtyFrames = (unsigned int)-1;
};


