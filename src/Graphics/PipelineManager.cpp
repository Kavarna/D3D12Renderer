#include "PipelineManager.h"
#include "Direct3D.h"
#include "Conversions.h"
#include "Utils/BatchRenderer.h"
#include "Utils/RayTracingStructures.h"
#include "Utils/Utils.h"
#include <dxcapi.h>

std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> GetSamplers()
{
    CD3DX12_STATIC_SAMPLER_DESC wrapLinearSampler(0, // shader register
                                                  D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
                                                  D3D12_TEXTURE_ADDRESS_MODE_WRAP, // address mode U
                                                  D3D12_TEXTURE_ADDRESS_MODE_WRAP, // address mode V
                                                  D3D12_TEXTURE_ADDRESS_MODE_WRAP // address mode W
    );

    CD3DX12_STATIC_SAMPLER_DESC wrapPointSampler(1, // shader register
                                                 D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP, // address mode U
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP, // address mode V
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP // address mode W
    );

    CD3DX12_STATIC_SAMPLER_DESC clampLinearSampler(2, // shader register
                                                   D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
                                                   D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // address mode U
                                                   D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // address mode V
                                                   D3D12_TEXTURE_ADDRESS_MODE_CLAMP // address mode W
    );

    CD3DX12_STATIC_SAMPLER_DESC clampPointSampler(3, // shader register
                                                  D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // address mode U
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // address mode V
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP // address mode W
    );

    return { wrapLinearSampler, wrapPointSampler, clampLinearSampler, clampPointSampler };
}

ComPtr<ID3DBlob> CompileLibrary(const wchar_t* filename, const wchar_t* target)
{
    ComPtr<IDxcCompiler> compiler;
    ComPtr<IDxcLibrary> library;

    // Create library & compiler
    CHECK_HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)), nullptr);
    CHECK_HR(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)), nullptr);

    // Read the shader
    std::ifstream shaderFile(filename);
    CHECK(shaderFile.good(), nullptr, "Unable to open file");

    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    std::string shaderContent = strStream.str();

    // Create blob from string
    ComPtr<IDxcBlobEncoding> textBlob;
    CHECK_HR(library->CreateBlobWithEncodingFromPinned((void*)shaderContent.data(), (uint32_t)shaderContent.size(), 0, &textBlob), nullptr);

    ComPtr<IDxcOperationResult> result;
    CHECK_HR(compiler->Compile(textBlob.Get(), filename, L"", target, nullptr, 0, nullptr, 0, nullptr, &result), nullptr);

    HRESULT errorCode;
    CHECK_HR(result->GetStatus(&errorCode), nullptr);
    if (FAILED(errorCode))
    {
        ComPtr<IDxcBlobEncoding> errorBlob;
        CHECK_HR(result->GetErrorBuffer(&errorBlob), nullptr);
        
        std::string error = Utils::ConvertBlobToString(errorBlob);
        SHOWFATAL("Error when compiling shader {}", error);
        return nullptr;
    }

    ComPtr<ID3DBlob> finalResult;
    CHECK_HR(result->GetResult((IDxcBlob**)finalResult.GetAddressOf()), nullptr);
    SHOWINFO("Successfully compiled library");
    return finalResult;
}

bool PipelineManager::Init()
{
    CHECK(D3DObject::Init(), false, "Unable to initialize d3d object");
    mSamplers = GetSamplers();

    CHECK(InitRootSignatures(), false, "Unable to initialize all root signatures");
    CHECK(InitPipelines(), false, "Unable to initialize all pipelines");

    return true;
}

auto PipelineManager::GetPipeline(PipelineType pipelineType)->Result<ID3D12PipelineState *>
{
    ID3D12PipelineState *pipeline = nullptr;
    ID3D12RootSignature *rootSignature = nullptr;

    if (auto pipelineIt = mPipelines.find(pipelineType); pipelineIt != mPipelines.end())
    {
        pipeline = (*pipelineIt).second.Get();
        return pipeline;
    }
    else
    {
        SHOWFATAL("Cannot find pipeline of type {}", PipelineTypeString[(int)pipelineType]);
        return std::nullopt;
    }

}

auto PipelineManager::GetRootSignature(PipelineType pipelineType) -> Result<ID3D12RootSignature *>
{
    if (auto rootSignatureTypeIt = mPipelineToRootSignature.find(pipelineType);
        rootSignatureTypeIt != mPipelineToRootSignature.end())
    {
        if (auto rootSignatureIt = mRootSignatures.find((*rootSignatureTypeIt).second);
            rootSignatureIt != mRootSignatures.end())
        {
            return (*rootSignatureIt).second.Get();
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
}

auto PipelineManager::GetPipelineAndRootSignature(PipelineType pipelineType) -> Result<std::tuple<ID3D12PipelineState *, ID3D12RootSignature *>>
{
    auto pipelineResult = GetPipeline(pipelineType);
    CHECK(pipelineResult.Valid(), std::nullopt, "Unable to get pipeline {}", PipelineTypeString[(int)pipelineType]);
    auto rootSignatureResult = GetRootSignature(pipelineType);
    CHECK(rootSignatureResult.Valid(), std::nullopt, "Unable to get root signature for pipeline type {}", PipelineTypeString[(int)pipelineType]);
    
    std::tuple<ID3D12PipelineState *, ID3D12RootSignature *> result = { pipelineResult.Get(), rootSignatureResult.Get() };
    return result;
}

bool PipelineManager::InitRootSignatures()
{
    CHECK(InitEmptyRootSignature(), false, "Unable to initialize an empty root signature");
    CHECK(InitSimpleColorRootSignature(), false, "Unable to initialize simple color's root signature");
    CHECK(InitObjectFrameMaterialRootSignature(), false, "Unable to initialize object frame material root signature");
    CHECK(InitTextureOnlyRootSignature(), false, "Unable to initialize texture only root signature");
    CHECK(InitTextureSrvUavBufferRootSignature(), false, "Unable to initialize texture srv uav and buffer signature");
    CHECK(InitPassMaterialLightsTextureInstance(), false, "Unable to initialize pass material light texture instance signature");
    CHECK(InitOneCBV(), false, "Unable to initialize one CBV root signature");

    SHOWINFO("Successfully initialized all root signatures");
    return true;
}

bool PipelineManager::InitPipelines()
{
    CHECK(InitSimpleColorPipeline(), false, "Unable to initialize simple color pipeline");
    CHECK(InitMaterialLightPipeline(), false, "Unable to initialize material light pipeline");
    CHECK(InitRawTexturePipeline(), false, "Unable to initialize raw texture pipeline");
    CHECK(InitBlurPipelines(), false, "Unable to initialize blur pipelines");
    CHECK(InitInstancedMaterialLightPipeline(), false, "Unable to initialize material light pipeline");
    CHECK(InitTerrainPipeline(), false, "Unable to initialize terrain pipeline");
    CHECK(InitInstancedMaterialColorLightPipeline(), false, "Unable to initialize instanced material color light pipeline");
    CHECK(InitDebugPipeline(), false, "Unable to initialize debug pipeline");
    CHECK(InitBasicRaytracingPipeline(), false, "Unable to initialize basic raytracing pipeline");

    SHOWINFO("Successfully initialized all pipelines");
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
    mRootSignatureDescs[type] = signatureDesc;

    SHOWINFO("Successfully initialized empty root signature");
    return true;
}

bool PipelineManager::InitSimpleColorRootSignature()
{
    auto d3d = Direct3D::Get();
    auto type = RootSignatureType::SimpleColor;

    CD3DX12_ROOT_PARAMETER parameters[2];
    parameters[0].InitAsConstantBufferView(0);
    parameters[1].InitAsConstantBufferView(1);

    D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
    signatureDesc.NumParameters = ARRAYSIZE(parameters);
    signatureDesc.pParameters = parameters;
    signatureDesc.NumStaticSamplers = 0;
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    auto signature = d3d->CreateRootSignature(signatureDesc);
    CHECK(signature.Valid(), false, "Unable to create a valid simple color root signature");
    mRootSignatures[type] = signature.Get();
    mRootSignatureDescs[type] = signatureDesc;

    SHOWINFO("Successfully initialized simple color root signature");
    return true;
}

bool PipelineManager::InitObjectFrameMaterialRootSignature()
{
    auto d3d = Direct3D::Get();
    auto type = RootSignatureType::ObjectFrameMaterialLights;

    CD3DX12_ROOT_PARAMETER parameters[5];
    parameters[0].InitAsConstantBufferView(0); // Object
    parameters[1].InitAsConstantBufferView(1); // Pass
    parameters[2].InitAsConstantBufferView(2); // Material
    parameters[3].InitAsConstantBufferView(3); // Lights
    
    CD3DX12_DESCRIPTOR_RANGE srvs[1];
    srvs[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    parameters[4].InitAsDescriptorTable(ARRAYSIZE(srvs), srvs);

    D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
    signatureDesc.NumParameters = ARRAYSIZE(parameters);
    signatureDesc.pParameters = parameters;
    signatureDesc.NumStaticSamplers = (uint32_t)mSamplers.size();
    signatureDesc.pStaticSamplers = mSamplers.data();
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    auto signature = d3d->CreateRootSignature(signatureDesc);
    CHECK(signature.Valid(), false, "Unable to create a valid object frame material root signature");
    mRootSignatures[type] = signature.Get();
    mRootSignatureDescs[type] = signatureDesc;

    SHOWINFO("Successfully initialized object frame material root signature");
    return true;
}

bool PipelineManager::InitTextureOnlyRootSignature()
{
    auto d3d = Direct3D::Get();
    auto type = RootSignatureType::TextureOnly;

    CD3DX12_DESCRIPTOR_RANGE ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    CD3DX12_ROOT_PARAMETER parameters[1];
    parameters[0].InitAsDescriptorTable(1, ranges);

    D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
    signatureDesc.NumParameters = ARRAYSIZE(parameters);
    signatureDesc.pParameters = parameters;
    signatureDesc.NumStaticSamplers = (uint32_t)mSamplers.size();
    signatureDesc.pStaticSamplers = mSamplers.data();
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    auto signature = d3d->CreateRootSignature(signatureDesc);
    CHECK(signature.Valid(), false, "Unable to create a valid simple color root signature");
    mRootSignatures[type] = signature.Get();
    mRootSignatureDescs[type] = signatureDesc;

    SHOWINFO("Successfully initialized texture only root signature");
    return true;
}

bool PipelineManager::InitTextureSrvUavBufferRootSignature()
{
    auto d3d = Direct3D::Get();
    auto type = RootSignatureType::TextureSrvUavBuffer;

    CD3DX12_DESCRIPTOR_RANGE srvRanges[1], uavRanges[1];
    srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    uavRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    CD3DX12_ROOT_PARAMETER parameters[3];
    parameters[0].InitAsDescriptorTable(ARRAYSIZE(srvRanges), srvRanges);
    parameters[1].InitAsDescriptorTable(ARRAYSIZE(uavRanges), uavRanges);
    parameters[2].InitAsConstantBufferView(0);

    D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
    signatureDesc.NumParameters = ARRAYSIZE(parameters);
    signatureDesc.pParameters = parameters;
    signatureDesc.NumStaticSamplers = 0;
    signatureDesc.pStaticSamplers = nullptr;
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    auto signature = d3d->CreateRootSignature(signatureDesc);
    CHECK(signature.Valid(), false, "Unable to create a valid simple color root signature");
    mRootSignatures[type] = signature.Get();
    mRootSignatureDescs[type] = signatureDesc;

    SHOWINFO("Successfully initialized texture srv uav and buffer signature");
    return true;
}

bool PipelineManager::InitPassMaterialLightsTextureInstance()
{
    auto d3d = Direct3D::Get();
    auto type = RootSignatureType::PassMaterialLightsTextureInstance;


    CD3DX12_DESCRIPTOR_RANGE srvRanges[1];
    srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER parameters[5];
    parameters[0].InitAsConstantBufferView(0); // PerFrame
    parameters[1].InitAsConstantBufferView(1); // Material
    parameters[2].InitAsConstantBufferView(2); // Lights
    parameters[3].InitAsShaderResourceView(0, 1, D3D12_SHADER_VISIBILITY_VERTEX); // Instance
    parameters[4].InitAsDescriptorTable(ARRAYSIZE(srvRanges), srvRanges, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
    signatureDesc.NumParameters = ARRAYSIZE(parameters);
    signatureDesc.pParameters = parameters;
    signatureDesc.NumStaticSamplers = (uint32_t)mSamplers.size();
    signatureDesc.pStaticSamplers = mSamplers.data();
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    auto signature = d3d->CreateRootSignature(signatureDesc);
    CHECK(signature.Valid(), false, "Unable to create a valid PassMaterialLightsTextureInstance signature");
    mRootSignatures[type] = signature.Get();
    mRootSignatureDescs[type] = signatureDesc;

    SHOWINFO("Successfully initialized PassMaterialLightsTextureInstance");
    return true;
}

bool PipelineManager::InitOneCBV()
{
    auto d3d = Direct3D::Get();
    auto type = RootSignatureType::OneCBV;

    CD3DX12_ROOT_PARAMETER parameters[1];
    parameters[0].InitAsConstantBufferView(0); // PerFrame

    D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
    signatureDesc.NumParameters = ARRAYSIZE(parameters);
    signatureDesc.pParameters = parameters;
    signatureDesc.NumStaticSamplers = (uint32_t)mSamplers.size();
    signatureDesc.pStaticSamplers = mSamplers.data();
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    auto signature = d3d->CreateRootSignature(signatureDesc);
    CHECK(signature.Valid(), false, "Unable to create a valid OneCBV signature");
    mRootSignatures[type] = signature.Get();
    mRootSignatureDescs[type] = signatureDesc;

    SHOWINFO("Successfully initialized OneCBV");
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
    simpleColorPipeline.DSVFormat = Direct3D::kDepthStencilFormat;
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
    
    auto elementDesc = PositionColorVertex::GetInputElementDesc();
    simpleColorPipeline.InputLayout.NumElements = (uint32_t)elementDesc.size();
    simpleColorPipeline.InputLayout.pInputElementDescs = elementDesc.data();

    ComPtr<ID3DBlob> vertexShader, pixelShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\SimpleColorPipeline_VertexShader.cso", &vertexShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\SimpleColorPipeline_PixelShader.cso", &pixelShader), false);

    simpleColorPipeline.VS.BytecodeLength = vertexShader->GetBufferSize();
    simpleColorPipeline.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    simpleColorPipeline.PS.BytecodeLength = pixelShader->GetBufferSize();
    simpleColorPipeline.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineState(simpleColorPipeline);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);
    
    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    SHOWINFO("Successfully initialized simple color pipeline");
    return true;
}

bool PipelineManager::InitMaterialLightPipeline()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::MaterialLight;
    RootSignatureType rootSignatureType = RootSignatureType::ObjectFrameMaterialLights;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC materialLightPipeline = {};

    materialLightPipeline.NodeMask = 0;
    materialLightPipeline.BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    materialLightPipeline.CachedPSO.CachedBlobSizeInBytes = 0;
    materialLightPipeline.CachedPSO.pCachedBlob = nullptr;
    materialLightPipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    materialLightPipeline.DepthStencilState.StencilEnable = FALSE;
    materialLightPipeline.DepthStencilState.DepthEnable = TRUE;
    materialLightPipeline.DSVFormat = Direct3D::kDepthStencilFormat;
    materialLightPipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    materialLightPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    materialLightPipeline.NumRenderTargets = 1;
    materialLightPipeline.RTVFormats[0] = Direct3D::kBackbufferFormat;
    materialLightPipeline.SampleDesc.Count = 1;
    materialLightPipeline.SampleDesc.Quality = 0;
    materialLightPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    materialLightPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    materialLightPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());

    auto rootSignature = mRootSignatures.find(rootSignatureType);
    CHECK(!(rootSignature == mRootSignatures.end()), false,
          "Unable to find empty root signature for pipeline type {}", PipelineTypeString[int(type)]);
    materialLightPipeline.pRootSignature = rootSignature->second.Get();

    auto elementDesc = PositionNormalTexCoordVertex::GetInputElementDesc();
    materialLightPipeline.InputLayout.NumElements = (uint32_t)elementDesc.size();
    materialLightPipeline.InputLayout.pInputElementDescs = elementDesc.data();

    ComPtr<ID3DBlob> vertexShader, pixelShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\MaterialLightPipeline_VertexShader.cso", &vertexShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\MaterialLightPipeline_PixelShader.cso", &pixelShader), false);

    materialLightPipeline.VS.BytecodeLength = vertexShader->GetBufferSize();
    materialLightPipeline.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    materialLightPipeline.PS.BytecodeLength = pixelShader->GetBufferSize();
    materialLightPipeline.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineState(materialLightPipeline);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);

    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    SHOWINFO("Successfully initialized material light pipeline");
    return true;
}

bool PipelineManager::InitRawTexturePipeline()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::RawTexture;
    RootSignatureType rootSignatureType = RootSignatureType::TextureOnly;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC materialLightPipeline = {};

    materialLightPipeline.NodeMask = 0;
    materialLightPipeline.BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    materialLightPipeline.CachedPSO.CachedBlobSizeInBytes = 0;
    materialLightPipeline.CachedPSO.pCachedBlob = nullptr;
    materialLightPipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    materialLightPipeline.DepthStencilState.StencilEnable = FALSE;
    materialLightPipeline.DepthStencilState.DepthEnable = FALSE;
    materialLightPipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    materialLightPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    materialLightPipeline.NumRenderTargets = 1;
    materialLightPipeline.RTVFormats[0] = Direct3D::kBackbufferFormat;
    materialLightPipeline.SampleDesc.Count = 1;
    materialLightPipeline.SampleDesc.Quality = 0;
    materialLightPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    materialLightPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    materialLightPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());

    auto rootSignature = mRootSignatures.find(rootSignatureType);
    CHECK(!(rootSignature == mRootSignatures.end()), false,
          "Unable to find empty root signature for pipeline type {}", PipelineTypeString[int(type)]);
    materialLightPipeline.pRootSignature = rootSignature->second.Get();

    auto elementDesc = PositionNormalTexCoordVertex::GetInputElementDesc();
    materialLightPipeline.InputLayout.NumElements = (uint32_t)elementDesc.size();
    materialLightPipeline.InputLayout.pInputElementDescs = elementDesc.data();

    ComPtr<ID3DBlob> vertexShader, pixelShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\RawTexturePipeline_VertexShader.cso", &vertexShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\RawTexturePipeline_PixelShader.cso", &pixelShader), false);

    materialLightPipeline.VS.BytecodeLength = vertexShader->GetBufferSize();
    materialLightPipeline.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    materialLightPipeline.PS.BytecodeLength = pixelShader->GetBufferSize();
    materialLightPipeline.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineState(materialLightPipeline);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);

    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    SHOWINFO("Successfully initialized raw texture pipeline");
    return true;
}

bool PipelineManager::InitBlurPipelines()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::HorizontalBlur;
    RootSignatureType rootSignatureType = RootSignatureType::TextureSrvUavBuffer;

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};

    pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    pipelineStateDesc.NodeMask = 0;
    pipelineStateDesc.pRootSignature = mRootSignatures[rootSignatureType].Get();
    pipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
    pipelineStateDesc.CachedPSO.pCachedBlob = nullptr;
    
    ComPtr<ID3DBlob> horizontalBlurComputeShader, verticalBlurComputeShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\HorizontalBlurPipeline_ComputeShader.cso", &horizontalBlurComputeShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\VerticalBlurPipeline_ComputeShader.cso", &verticalBlurComputeShader), false);

    pipelineStateDesc.CS.BytecodeLength = horizontalBlurComputeShader->GetBufferSize();
    pipelineStateDesc.CS.pShaderBytecode = horizontalBlurComputeShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineState(pipelineStateDesc);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);

    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(horizontalBlurComputeShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    type = PipelineType::VerticalBlur;

    pipelineStateDesc.CS.BytecodeLength = verticalBlurComputeShader->GetBufferSize();
    pipelineStateDesc.CS.pShaderBytecode = verticalBlurComputeShader->GetBufferPointer();
    pipeline = d3d->CreatePipelineState(pipelineStateDesc);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);
    
    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(verticalBlurComputeShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    return true;
}

bool PipelineManager::InitInstancedMaterialLightPipeline()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::InstancedMaterialLight;
    RootSignatureType rootSignatureType = RootSignatureType::ObjectFrameMaterialLights;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC materialLightPipeline = {};

    materialLightPipeline.NodeMask = 0;
    materialLightPipeline.BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    materialLightPipeline.CachedPSO.CachedBlobSizeInBytes = 0;
    materialLightPipeline.CachedPSO.pCachedBlob = nullptr;
    materialLightPipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    materialLightPipeline.DepthStencilState.StencilEnable = FALSE;
    materialLightPipeline.DepthStencilState.DepthEnable = TRUE;
    materialLightPipeline.DSVFormat = Direct3D::kDepthStencilFormat;
    materialLightPipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    materialLightPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    materialLightPipeline.NumRenderTargets = 1;
    materialLightPipeline.RTVFormats[0] = Direct3D::kBackbufferFormat;
    materialLightPipeline.SampleDesc.Count = 1;
    materialLightPipeline.SampleDesc.Quality = 0;
    materialLightPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    materialLightPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    materialLightPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());

    auto rootSignature = mRootSignatures.find(rootSignatureType);
    CHECK(!(rootSignature == mRootSignatures.end()), false,
          "Unable to find empty root signature for pipeline type {}", PipelineTypeString[int(type)]);
    materialLightPipeline.pRootSignature = rootSignature->second.Get();

    // Init input element desc
    auto elementDesc = PositionNormalTexCoordVertex::GetInputElementDesc();
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements(elementDesc.begin(), elementDesc.end());
    D3D12_INPUT_ELEMENT_DESC worldMatrix[4], texWorldMatrix[4];
    worldMatrix[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    worldMatrix[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
    worldMatrix[0].InputSlot = 1;
    worldMatrix[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    worldMatrix[0].InstanceDataStepRate = 1;
    worldMatrix[0].SemanticIndex = 0;
    worldMatrix[0].SemanticName = "WORLDMATRIX";
    texWorldMatrix[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    texWorldMatrix[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
    texWorldMatrix[0].InputSlot = 1;
    texWorldMatrix[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    texWorldMatrix[0].InstanceDataStepRate = 1;
    texWorldMatrix[0].SemanticIndex = 0;
    texWorldMatrix[0].SemanticName = "TEXWORLDMATRIX";
    for (int i = 1; i < 4; ++i)
    {
        worldMatrix[i] = worldMatrix[0];
        worldMatrix[i].SemanticIndex = i;
        texWorldMatrix[i] = texWorldMatrix[0];
        texWorldMatrix[i].SemanticIndex = i;
    }
    std::copy(std::begin(worldMatrix), std::end(worldMatrix), std::back_inserter(inputElements));
    std::copy(std::begin(texWorldMatrix), std::end(texWorldMatrix), std::back_inserter(inputElements));

    materialLightPipeline.InputLayout.NumElements = (uint32_t)inputElements.size();
    materialLightPipeline.InputLayout.pInputElementDescs = inputElements.data();

    ComPtr<ID3DBlob> vertexShader, pixelShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\InstancedMaterialLightPipeline_VertexShader.cso", &vertexShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\InstancedMaterialLightPipeline_PixelShader.cso", &pixelShader), false);

    materialLightPipeline.VS.BytecodeLength = vertexShader->GetBufferSize();
    materialLightPipeline.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    materialLightPipeline.PS.BytecodeLength = pixelShader->GetBufferSize();
    materialLightPipeline.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineState(materialLightPipeline);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);

    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    SHOWINFO("Successfully initialized material light pipeline");
    return true;
}


bool PipelineManager::InitTerrainPipeline()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::Terrain;
    RootSignatureType rootSignatureType = RootSignatureType::ObjectFrameMaterialLights;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC terrainPipeline = {};

    terrainPipeline.NodeMask = 0;
    terrainPipeline.BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    terrainPipeline.CachedPSO.CachedBlobSizeInBytes = 0;
    terrainPipeline.CachedPSO.pCachedBlob = nullptr;
    terrainPipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    terrainPipeline.DepthStencilState.StencilEnable = FALSE;
    terrainPipeline.DepthStencilState.DepthEnable = TRUE;
    terrainPipeline.DSVFormat = Direct3D::kDepthStencilFormat;
    terrainPipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    terrainPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    terrainPipeline.NumRenderTargets = 1;
    terrainPipeline.RTVFormats[0] = Direct3D::kBackbufferFormat;
    terrainPipeline.SampleDesc.Count = 1;
    terrainPipeline.SampleDesc.Quality = 0;
    terrainPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    terrainPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    terrainPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
    terrainPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

    auto rootSignature = mRootSignatures.find(rootSignatureType);
    CHECK(!(rootSignature == mRootSignatures.end()), false,
          "Unable to find empty root signature for pipeline type {}", PipelineTypeString[int(type)]);
    terrainPipeline.pRootSignature = rootSignature->second.Get();

    auto elementDesc = PositionNormalTexCoordVertex::GetInputElementDesc();
    terrainPipeline.InputLayout.NumElements = (uint32_t)elementDesc.size();
    terrainPipeline.InputLayout.pInputElementDescs = elementDesc.data();

    ComPtr<ID3DBlob> vertexShader, hullShader, domainShader, pixelShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\TerrainPipeline_VertexShader.cso", &vertexShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\TerrainPipeline_HullShader.cso", &hullShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\TerrainPipeline_DomainShader.cso", &domainShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\TerrainPipeline_PixelShader.cso", &pixelShader), false);

    terrainPipeline.VS.BytecodeLength = vertexShader->GetBufferSize();
    terrainPipeline.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    terrainPipeline.HS.BytecodeLength = hullShader->GetBufferSize();
    terrainPipeline.HS.pShaderBytecode = hullShader->GetBufferPointer();
    terrainPipeline.DS.BytecodeLength = domainShader->GetBufferSize();
    terrainPipeline.DS.pShaderBytecode = domainShader->GetBufferPointer();
    terrainPipeline.PS.BytecodeLength = pixelShader->GetBufferSize();
    terrainPipeline.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineState(terrainPipeline);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);

    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    SHOWINFO("Successfully initialized material light pipeline");
    return true;
}

bool PipelineManager::InitInstancedMaterialColorLightPipeline()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::InstancedColorMaterialLight;
    RootSignatureType rootSignatureType = RootSignatureType::PassMaterialLightsTextureInstance;

    auto layoutDesc = PositionNormalTexCoordVertex::GetInputElementDesc();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
    pipelineDesc.NodeMask = 0;
    pipelineDesc.BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    pipelineDesc.DSVFormat = d3d->kDepthStencilFormat;
    pipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    pipelineDesc.InputLayout.pInputElementDescs = layoutDesc.data();
    pipelineDesc.InputLayout.NumElements = (uint32_t)layoutDesc.size();
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = d3d->kBackbufferFormat;
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
    pipelineDesc.RasterizerState.FrontCounterClockwise = FALSE;
    // pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    pipelineDesc.SampleDesc.Count = 1;
    pipelineDesc.SampleDesc.Quality = 0;
    pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    
    auto rootSignature = mRootSignatures.find(rootSignatureType);
    CHECK(!(rootSignature == mRootSignatures.end()), false,
        "Unable to find empty root signature for pipeline type {}", PipelineTypeString[int(type)]);
    pipelineDesc.pRootSignature = rootSignature->second.Get();

    ComPtr<ID3DBlob> vertexShader, pixelShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\InstancedColorMaterialLightPipeline_VertexShader.cso", &vertexShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\InstancedColorMaterialLightPipeline_PixelShader.cso", &pixelShader), false);

    pipelineDesc.VS.BytecodeLength = vertexShader->GetBufferSize();
    pipelineDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    pipelineDesc.PS.BytecodeLength = pixelShader->GetBufferSize();
    pipelineDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineState(pipelineDesc);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);

    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    SHOWINFO("Successfully initialized instanced material light color pipeline");
    return true;
}

bool PipelineManager::InitDebugPipeline()
{
    auto d3d = Direct3D::Get();
    PipelineType type = PipelineType::DebugPipeline;
    RootSignatureType rootSignatureType = RootSignatureType::OneCBV;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC simpleColorPipeline = {};

    simpleColorPipeline.NodeMask = 0;
    simpleColorPipeline.BlendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
    simpleColorPipeline.CachedPSO.CachedBlobSizeInBytes = 0;
    simpleColorPipeline.CachedPSO.pCachedBlob = nullptr;
    simpleColorPipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    simpleColorPipeline.DepthStencilState.StencilEnable = FALSE;
    simpleColorPipeline.DepthStencilState.DepthEnable = TRUE;
    simpleColorPipeline.DSVFormat = Direct3D::kDepthStencilFormat;
    simpleColorPipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    simpleColorPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    simpleColorPipeline.NumRenderTargets = 1;
    simpleColorPipeline.RTVFormats[0] = Direct3D::kBackbufferFormat;
    simpleColorPipeline.SampleDesc.Count = 1;
    simpleColorPipeline.SampleDesc.Quality = 0;
    simpleColorPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    simpleColorPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    simpleColorPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());

    auto rootSignature = mRootSignatures.find(rootSignatureType);
    CHECK(!(rootSignature == mRootSignatures.end()), false,
        "Unable to find empty root signature for pipeline type {}", PipelineTypeString[int(type)]);
    simpleColorPipeline.pRootSignature = rootSignature->second.Get();

    auto elementDesc = PositionColorVertex::GetInputElementDesc();
    simpleColorPipeline.InputLayout.NumElements = (uint32_t)elementDesc.size();
    simpleColorPipeline.InputLayout.pInputElementDescs = elementDesc.data();

    ComPtr<ID3DBlob> vertexShader, pixelShader;
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\Debug_VertexShader.cso", &vertexShader), false);
    CHECK_HR(D3DReadFileToBlob(L"Shaders\\Debug_PixelShader.cso", &pixelShader), false);

    simpleColorPipeline.VS.BytecodeLength = vertexShader->GetBufferSize();
    simpleColorPipeline.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    simpleColorPipeline.PS.BytecodeLength = pixelShader->GetBufferSize();
    simpleColorPipeline.PS.pShaderBytecode = pixelShader->GetBufferPointer();

    auto pipeline = d3d->CreatePipelineState(simpleColorPipeline);
    CHECK(pipeline.Valid(), false, "Unable to create pipeline type {}", PipelineTypeString[int(type)]);

    mPipelines[type] = pipeline.Get();
    mShaders[type].push_back(vertexShader);
    mShaders[type].push_back(pixelShader);
    mPipelineToRootSignature[type] = rootSignatureType;

    SHOWINFO("Successfully initialized debug pipeline");
    return true;
}

bool PipelineManager::InitBasicRaytracingPipeline()
{
    ComPtr<ID3D12Device5> device5;
    CHECK_HR(mDevice.As(&device5), false);

    std::array<D3D12_STATE_SUBOBJECT, 10> subobjects;
    uint32_t index = 0;

    const wchar_t* szRayGen = L"rayGen";
    const wchar_t* szMiss = L"miss";
    const wchar_t* szClosestHit = L"chs";

    auto shadersLibrary = CompileLibrary(L"Shaders\\Basic.rt.hlsl", L"lib_6_3");
    DxilLibrary library(shadersLibrary, { szRayGen, szMiss, szClosestHit });
    subobjects[index++] = library.stateSubobject; // 0

    HitGroup hitGroup(L"", L"chs", L"HitGroup");
    subobjects[index++] = hitGroup.stateSubobject; // 1

    CD3DX12_DESCRIPTOR_RANGE ranges[2];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 1);

    CD3DX12_ROOT_PARAMETER parameters[1];
    parameters[0].InitAsDescriptorTable(ARRAYSIZE(ranges), ranges);
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = parameters;

    LocalRootSignature localRootSignature(device5, rootSignatureDesc);
    subobjects[index] = localRootSignature.stateSubobject; // 2

    const wchar_t* rayGenAssociations[] =
    {
        szRayGen
    };
    ExportAssociations exportAssociations(rayGenAssociations, ARRAYSIZE(rayGenAssociations), &subobjects[index++]);
    subobjects[index++] = exportAssociations.stateSubobject; // 3

    D3D12_ROOT_SIGNATURE_DESC emptyRootSignatureDesc = {};
    emptyRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    emptyRootSignatureDesc.NumParameters = 0;
    LocalRootSignature emptyRootSignature(device5, emptyRootSignatureDesc);
    subobjects[index] = emptyRootSignature.stateSubobject; // 4

    const wchar_t* emptyAssociations[] =
    {
        szMiss, szClosestHit
    };
    ExportAssociations emptyAssociation(emptyAssociations, ARRAYSIZE(emptyAssociations), &subobjects[index++]);
    subobjects[index++] = emptyAssociation.stateSubobject; // 5

    ShaderConfig config(2 * sizeof(float), 1 * sizeof(float));
    subobjects[index] = config.stateSubobject; // 6

    const wchar_t* shaderConfigAssociations[] =
    {
        szRayGen, szMiss, szClosestHit
    };
    ExportAssociations shaderConfigAssociation(shaderConfigAssociations, ARRAYSIZE(shaderConfigAssociations), &subobjects[index++]);
    subobjects[index++] = shaderConfigAssociation.stateSubobject; // 7

    PipelineConfig pipelineConfig(0);
    subobjects[index++] = pipelineConfig.stateSubobject; // 8

    GlobalRootSignature globalRootSignature(device5, {});
    subobjects[index++] = globalRootSignature.stateSubobject; // 9


    D3D12_STATE_OBJECT_DESC objectDesc = {};
    objectDesc.pSubobjects = subobjects.data();
    objectDesc.NumSubobjects = index;
    objectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    CHECK_HR(device5->CreateStateObject(&objectDesc, IID_PPV_ARGS(&mRtStateObject)), false);

    return true;
}
