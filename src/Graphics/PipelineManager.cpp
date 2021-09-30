#include "PipelineManager.h"
#include "Direct3D.h"
#include "Conversions.h"

bool PipelineManager::Init()
{
    CHECK(InitRootSignatures(), false, "Unable to initialize all root signatures");
    CHECK(InitPipelines(), false, "Unable to initialize all pipelines");

    return true;
}

auto PipelineManager::GetPipeline(PipelineType pipelineType)->Result<std::tuple<ID3D12PipelineState *, ID3D12RootSignature *>>
{
    ID3D12PipelineState *pipeline = nullptr;
    ID3D12RootSignature *rootSignature = nullptr;

    if (auto pipelineIt = mPipelines.find(pipelineType); pipelineIt != mPipelines.end())
    {
        pipeline = (*pipelineIt).second.Get();
    }
    else
    {
        SHOWFATAL("Cannot find pipeline of type {}", PipelineTypeString[(int)pipelineType]);
        return std::nullopt;
    }

    if (auto rootSignatureTypeIt = mPipelineToRootSignature.find(pipelineType);
        rootSignatureTypeIt != mPipelineToRootSignature.end())
    {
        if (auto rootSignatureIt = mRootSignatures.find((*rootSignatureTypeIt).second);
            rootSignatureIt != mRootSignatures.end())
        {
            rootSignature = (*rootSignatureIt).second.Get();
        }
        else
        {
            SHOWFATAL("Cannot find root signature for pipeline type {} and root signature type{}",
                      PipelineTypeString[(int)pipelineType], RootSignatureTypeString[(int)(*rootSignatureTypeIt).second]);
            return std::nullopt;
        }
    }
    else
    {
        SHOWFATAL("Cannot find root signature type for pipeline type {}", PipelineTypeString[(int)pipelineType]);
        return std::nullopt;
    }

    std::tuple<ID3D12PipelineState *, ID3D12RootSignature *> result = { pipeline, rootSignature };
    return result;
}

bool PipelineManager::InitRootSignatures()
{
    CHECK(InitEmptyRootSignature(), false, "Unable to initialize an empty root signature");
    CHECK(InitSimpleColorRootSignature(), false, "Unable to initialize simple color's root signature");

    return true;
}

bool PipelineManager::InitPipelines()
{
    CHECK(InitSimpleColorPipeline(), false, "Unable to initialize simple color pipeline");

    return true;
}

bool PipelineManager::InitEmptyRootSignature()
{
    auto d3d = Direct3D::Get();

    auto type = RootSignatureType::Empty;

    D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
    signatureDesc.NumParameters = 0;
    signatureDesc.NumStaticSamplers = 0;
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    auto signature = d3d->CreateRootSignature(signatureDesc);
    CHECK(signature.Valid(), false, "Unable to create a valid empty root signature");
    mRootSignatures[type] = signature.Get();

    return true;
}

bool PipelineManager::InitSimpleColorRootSignature()
{
    auto d3d = Direct3D::Get();
    auto type = RootSignatureType::SimpleColor;

    CD3DX12_ROOT_PARAMETER parameters[1];
    parameters[0].InitAsConstantBufferView(0);

    D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
    signatureDesc.NumParameters = ARRAYSIZE(parameters);
    signatureDesc.pParameters = parameters;
    signatureDesc.NumStaticSamplers = 0;
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    auto signature = d3d->CreateRootSignature(signatureDesc);
    CHECK(signature.Valid(), false, "Unable to create a valid empty root signature");
    mRootSignatures[type] = signature.Get();

    return true;
}

bool PipelineManager::InitSimpleColorPipeline()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::SimpleColor;
    RootSignatureType rootSignatureType = RootSignatureType::SimpleColor;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC simpleColorPipeline = {};

    simpleColorPipeline.NodeMask = 0;
    simpleColorPipeline.BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    simpleColorPipeline.CachedPSO.CachedBlobSizeInBytes = 0;
    simpleColorPipeline.CachedPSO.pCachedBlob = nullptr;
    simpleColorPipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    simpleColorPipeline.DepthStencilState.StencilEnable = FALSE;
    simpleColorPipeline.DepthStencilState.DepthEnable = FALSE;
    simpleColorPipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    simpleColorPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    simpleColorPipeline.NumRenderTargets = 1;
    simpleColorPipeline.RTVFormats[0] = Direct3D::kBackbufferFormat;
    simpleColorPipeline.SampleDesc.Count = 1;
    simpleColorPipeline.SampleDesc.Quality = 0;
    simpleColorPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    simpleColorPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    simpleColorPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());

    auto rootSignature = mRootSignatures.find(rootSignatureType);
    CHECK(!(rootSignature == mRootSignatures.end()), false,
          "Unable to find empty root signature for pipeline type {}", PipelineTypeString[int(type)]);
    simpleColorPipeline.pRootSignature = rootSignature->second.Get();
    
    auto elementDesc = Vertex::GetInputElementDesc();
    simpleColorPipeline.InputLayout.NumElements = (uint32_t)elementDesc.size();
    simpleColorPipeline.InputLayout.pInputElementDescs = elementDesc.data();

    ComPtr<ID3DBlob> vertexShader, pixelShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\SimpleColorPipeline_VertexShader.cso", &vertexShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\SimpleColorPipeline_PixelShader.cso", &pixelShader), false);

    simpleColorPipeline.VS.BytecodeLength = vertexShader->GetBufferSize();
    simpleColorPipeline.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    simpleColorPipeline.PS.BytecodeLength = pixelShader->GetBufferSize();
    simpleColorPipeline.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineSteate(simpleColorPipeline);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);
    
    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    return true;
}


