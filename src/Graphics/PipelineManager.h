#pragma once


#include <Oblivion.h>
#include "Direct3D.h"
#include "Vertex.h"

enum class PipelineType
{
    SimpleColor = 0
};
static constexpr const char *PipelineTypeString[] =
{
    "SimpleColor"
};

enum class RootSignatureType
{
    Empty = 0
};
static constexpr const char *RootSignatureTypeString[] =
{
    "Empty"
};

class PipelineManager : public ISingletone<PipelineManager>
{
    MAKE_SINGLETONE_CAPABLE(PipelineManager);
private:
    PipelineManager() = default;
    ~PipelineManager() = default;

public:
    bool Init();

private:
    bool InitRootSignatures();
    bool InitPipelines();

private:
    bool InitEmptyRootSignature();

private:
    bool InitSimpleColorPipeline();

    Result<ComPtr<ID3DBlob>> CompileShader(LPCWSTR filename, LPCSTR profile);

private:
    bool mCreated = false;

    std::unordered_map<PipelineType, std::unordered_map<VertexType, ComPtr<ID3D12PipelineState>>> mPipelines;
    std::unordered_map<PipelineType, RootSignatureType> mPipelineToRootSignature;
    std::unordered_map<PipelineType, std::vector<ComPtr<ID3DBlob>>> mShaders;

    std::unordered_map<RootSignatureType, ComPtr<ID3D12RootSignature>> mRootSignatures;
};

