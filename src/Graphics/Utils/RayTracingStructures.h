#pragma once

#include <Oblivion.h>


struct SubobjectWrapper
{
    D3D12_STATE_SUBOBJECT stateSubobject;

    SubobjectWrapper(const void* desc, D3D12_STATE_SUBOBJECT_TYPE subobjectType)
    {
        stateSubobject.pDesc = desc;
        stateSubobject.Type = subobjectType;
    };

    operator D3D12_STATE_SUBOBJECT() const
    {
        return stateSubobject;
    };
};


struct DxilLibrary : public SubobjectWrapper
{
    DxilLibrary(ComPtr<ID3DBlob> shaderBlob, const wchar_t* entrypoints[], uint32_t numEntryPoints) :
        SubobjectWrapper(&dxilDesc, D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY), shaderBlob(shaderBlob)
    {
        dxilDesc.DXILLibrary.BytecodeLength = shaderBlob->GetBufferSize();
        dxilDesc.DXILLibrary.pShaderBytecode = shaderBlob->GetBufferPointer();

        exportDescs.resize(numEntryPoints);
        exportNames.resize(numEntryPoints);

        dxilDesc.NumExports = numEntryPoints;
        dxilDesc.pExports = exportDescs.data();
        for (uint32_t i = 0; i < numEntryPoints; ++i)
        {
            exportNames[i] = entrypoints[i];
            exportDescs[i].ExportToRename = nullptr;
            exportDescs[i].Flags = D3D12_EXPORT_FLAG_NONE;
            exportDescs[i].Name = exportNames[i].c_str();
        }
    }

    D3D12_DXIL_LIBRARY_DESC dxilDesc = {};
    ComPtr<ID3DBlob> shaderBlob;
    std::vector<D3D12_EXPORT_DESC> exportDescs;
    std::vector<std::wstring> exportNames;
};


struct HitGroup : public SubobjectWrapper
{
    HitGroup(const wchar_t* anyHitShader, const wchar_t* closestHitShader, const std::wstring& name) :
        SubobjectWrapper(&hitGroupDesc, D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP), exportName(name)
    {
        hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE::D3D12_HIT_GROUP_TYPE_TRIANGLES;
        hitGroupDesc.AnyHitShaderImport = anyHitShader;
        hitGroupDesc.ClosestHitShaderImport = closestHitShader;
        hitGroupDesc.HitGroupExport = exportName.c_str();
    }

    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    std::wstring exportName;
};

struct LocalRootSignature : public SubobjectWrapper
{
    LocalRootSignature(const D3D12_ROOT_SIGNATURE_DESC& signatureDesc) :
        SubobjectWrapper(&localRootSignature, D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE)
    {
        auto d3d = Direct3D::Get();
        auto signatureResult = d3d->CreateRootSignature(signatureDesc);
        CHECKSHOW(signatureResult.Valid(), "Unable to create a local root signature");
        
        rootSignature = signatureResult.Get();
        localRootSignature.pLocalRootSignature = rootSignature.Get();
    }
    D3D12_LOCAL_ROOT_SIGNATURE localRootSignature = {};
    ComPtr<ID3D12RootSignature> rootSignature;
};

struct ExportAssociation : public SubobjectWrapper
{
    ExportAssociation(uint32_t numExports, const wchar_t* exports[], const D3D12_STATE_SUBOBJECT& subobject) :
        SubobjectWrapper(&exportAssociation, D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION)
    {
        exportAssociation.NumExports = numExports;
        exportAssociation.pExports = exports;
        exportAssociation.pSubobjectToAssociate = &subobject;
    }

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportAssociation = {};
};

struct ShaderConfig : public SubobjectWrapper
{
    ShaderConfig(uint32_t maxAttributeSize, uint32_t maxPayloadSize) :
        SubobjectWrapper(&shaderConfig, D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG)
    {
        shaderConfig.MaxAttributeSizeInBytes = maxAttributeSize;
        shaderConfig.MaxPayloadSizeInBytes = maxPayloadSize;
    }
    
    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
};

struct PipelineConfig : public SubobjectWrapper
{
    PipelineConfig(uint32_t raytracingDepth) :
        SubobjectWrapper(&pipelineConfig, D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG)
    {
        pipelineConfig.MaxTraceRecursionDepth = raytracingDepth;
    }

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
};

struct GlobalRootSignature : public SubobjectWrapper
{
    GlobalRootSignature(const D3D12_ROOT_SIGNATURE_DESC& signatureDesc) :
        SubobjectWrapper(&globalRootSignature, D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE)
    {
        auto d3d = Direct3D::Get();
        auto signatureResult = d3d->CreateRootSignature(signatureDesc);
        CHECKSHOW(signatureResult.Valid(), "Unable to create a local root signature");

        globalRootSignature.pGlobalRootSignature = rootSignature.Get();
    }
    D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature;
    ComPtr<ID3D12RootSignature> rootSignature;
};
