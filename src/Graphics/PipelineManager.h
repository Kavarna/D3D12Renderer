#pragma once


#include <Oblivion.h>
#include "Direct3D.h"
#include "Vertex.h"

enum class PipelineType
{
    SimpleColor = 0, MaterialLight, RawTexture, HorizontalBlur, VerticalBlur,
    InstancedMaterialLight, Terrain,
};
static constexpr const char *PipelineTypeString[] =
{
    "SimpleColor", "MaterialLight", "RawTexture", "HorizontalBlur", "VerticalBlur",
    "InstancedMaterialLight", "Terrain",
};

enum class RootSignatureType
{
    Empty = 0, SimpleColor, ObjectFrameMaterialLights, TextureOnly, TextureSrvUavBuffer
};
static constexpr const char *RootSignatureTypeString[] =
{
    "Empty", "SimpleColor", "ObjectFrameMaterialLights", "TextureOnly", "TextureSrvUavBuffer"
};

class PipelineManager : public ISingletone<PipelineManager>
{
    MAKE_SINGLETONE_CAPABLE(PipelineManager);
private:
    PipelineManager() = default;
    ~PipelineManager() = default;

public:
    bool Init();

public:
    auto GetPipeline(PipelineType pipeline)->Result<ID3D12PipelineState *>;
    auto GetRootSignature(PipelineType pipeline)->Result<ID3D12RootSignature *>;
    auto GetPipelineAndRootSignature(PipelineType pipeline)->Result<std::tuple<ID3D12PipelineState *, ID3D12RootSignature *>>;

private:
    bool InitRootSignatures();
    bool InitPipelines();

private:
    bool InitEmptyRootSignature();
    bool InitSimpleColorRootSignature();
    bool InitObjectFrameMaterialRootSignature();
    bool InitTextureOnlyRootSignature();
    bool InitTextureSrvUavBufferRootSignature();

private:
    bool InitSimpleColorPipeline();
    bool InitMaterialLightPipeline();
    bool InitRawTexturePipeline();
    bool InitBlurPipelines();
    bool InitInstancedMaterialLightPipeline();
    bool InitTerrainPipeline();

private:
    bool mCreated = false;
    
    std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> mSamplers;

    std::unordered_map<PipelineType, ComPtr<ID3D12PipelineState>> mPipelines;
    std::unordered_map<PipelineType, RootSignatureType> mPipelineToRootSignature;
    std::unordered_map<PipelineType, std::vector<ComPtr<ID3DBlob>>> mShaders;

    std::unordered_map<RootSignatureType, ComPtr<ID3D12RootSignature>> mRootSignatures;
};

