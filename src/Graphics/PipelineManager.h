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
    Empty = 0, SimpleColor
};
static constexpr const char *RootSignatureTypeString[] =
{
    "Empty", "SimpleColor"
};

class PipelineManager : public ISingletone<PipelineManager>
{
    MAKE_SINGLETONE_CAPABLE(PipelineManager);
    using Vertex = PositionColorVertex;
private:
    PipelineManager() = default;
    ~PipelineManager() = default;

public:
    bool Init();

public:
    auto GetPipeline(PipelineType pipeline)->Result<std::tuple<ID3D12PipelineState *, ID3D12RootSignature *>>;

private:
    bool InitRootSignatures();
    bool InitPipelines();

private:
    bool InitEmptyRootSignature();
    bool InitSimpleColorRootSignature();

private:
    bool InitSimpleColorPipeline();

private:
    bool mCreated = false;

    std::unordered_map<PipelineType, ComPtr<ID3D12PipelineState>> mPipelines;
    std::unordered_map<PipelineType, RootSignatureType> mPipelineToRootSignature;
    std::unordered_map<PipelineType, std::vector<ComPtr<ID3DBlob>>> mShaders;

    std::unordered_map<RootSignatureType, ComPtr<ID3D12RootSignature>> mRootSignatures;
};

