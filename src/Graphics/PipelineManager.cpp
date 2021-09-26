#include "PipelineManager.h"
#include "Direct3D.h"
#include "Conversions.h"

bool PipelineManager::Init()
{
    CHECK(InitRootSignatures(), false, "Unable to initialize all root signatures");
    CHECK(InitPipelines(), false, "Unable to initialize all pipelines");

    return true;
}

bool PipelineManager::InitRootSignatures()
{
    CHECK(InitEmptyRootSignature(), false, "Unable to initialize an empty root signature");
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

bool PipelineManager::InitSimpleColorPipeline()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::SimpleColor;
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

    auto rootSignature = mRootSignatures.find(RootSignatureType::Empty);
    CHECK(!(rootSignature == mRootSignatures.end()), false,
          "Unable to find empty root signature for pipeline type {}", PipelineTypeString[int(type)]);
    simpleColorPipeline.pRootSignature = rootSignature->second.Get();
    
    auto vertexType = VertexType::PositionColorVertex;
    auto elementDesc = PositionColorVertex::GetInputElementDesc();
    simpleColorPipeline.InputLayout.NumElements = (uint32_t)elementDesc.size();
    simpleColorPipeline.InputLayout.pInputElementDescs = elementDesc.data();

    auto vs = CompileShader(L"Shaders\\SimpleColorPipeline\\VertexShader.hlsl", "vs_5_0");
    CHECK(vs.Valid(), false, "Unable to compile vertex shader for pipeline type {} vertex type {}",
          PipelineTypeString[int(type)], VertexTypeString[int(vertexType)]);
    auto vertexShader = vs.Get();
    auto ps = CompileShader(L"Shaders\\SimpleColorPipeline\\PixelShader.hlsl", "ps_5_0");
    CHECK(ps.Valid(), false, "Unable to compile pixel sahder for pipeline type {} vertex type {}",
          PipelineTypeString[int(type)], VertexTypeString[int(vertexType)]);
    auto pixelShader = ps.Get();

    simpleColorPipeline.VS.BytecodeLength = vertexShader->GetBufferSize();
    simpleColorPipeline.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    simpleColorPipeline.PS.BytecodeLength = pixelShader->GetBufferSize();
    simpleColorPipeline.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineSteate(simpleColorPipeline);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {} for vertex type {}",
          PipelineTypeString[int(type)], VertexTypeString[int(vertexType)]);
    
    mPipelines[type][vertexType] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = RootSignatureType::Empty;

    return true;
}

Result<ComPtr<ID3DBlob>> PipelineManager::CompileShader(LPCWSTR filename, LPCSTR profile)
{
    ComPtr<ID3DBlob> result, errorMessages;

    UINT flags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    auto szFilename = Conversions::ws2s(filename);

    HRESULT hr = D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
                                    profile, flags, 0, &result, &errorMessages);
    if (errorMessages)
    {
        SHOWWARNING("Message shown when compiling {} with profile {}: {}",
                    szFilename, profile, (char*)errorMessages->GetBufferPointer());
    }
    CHECK(SUCCEEDED(hr), false, "Unable to compile {} with profile {}", szFilename, profile);

    return result;
}

