#pragma once

#include <Oblivion.h>


struct DxilLibrary
{
    DxilLibrary(ComPtr<ID3DBlob> pBlob, const std::vector<std::wstring>& entrypoints)
    {
        stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        stateSubobject.pDesc = &libraryDesc;

        libraryDesc = {};
        exportDesc.resize(entrypoints.size());
        exportName.resize(entrypoints.size());

        if (pBlob)
        {
            libraryDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
            libraryDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
            libraryDesc.NumExports = (uint32_t)entrypoints.size();
            libraryDesc.pExports = exportDesc.data();

            for (uint32_t i = 0; i < entrypoints.size(); ++i)
            {
                exportName[i] = entrypoints[i];
                exportDesc[i].Name = exportName[i].c_str();
                exportDesc[i].ExportToRename = nullptr;
                exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
            }
        }
    }

    DxilLibrary() : DxilLibrary(nullptr, {}) {};

    D3D12_DXIL_LIBRARY_DESC libraryDesc = {};
    D3D12_STATE_SUBOBJECT stateSubobject = {};
    ComPtr<ID3DBlob> shaderBlob;
    std::vector<D3D12_EXPORT_DESC> exportDesc;
    std::vector<std::wstring> exportName;
};



struct HitGroup
{
    HitGroup(const std::wstring& ahsExport_, const std::wstring& chsExport_, const std::wstring& name) :
        exportName(name), ahsExport(ahsExport_), chsExport(chsExport_)
    {
        desc = {};
        if (ahsExport.size()) desc.AnyHitShaderImport = ahsExport.c_str();
        if (chsExport.size()) desc.ClosestHitShaderImport = chsExport.c_str();
        if (exportName.size()) desc.HitGroupExport = exportName.c_str();
        desc.IntersectionShaderImport = nullptr;
        desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

        stateSubobject.pDesc = &desc;
        stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    }

    D3D12_STATE_SUBOBJECT stateSubobject = {};
    D3D12_HIT_GROUP_DESC desc = {};
    std::wstring exportName, ahsExport, chsExport;
};


struct LocalRootSignature
{
    LocalRootSignature(ComPtr<ID3D12Device5> device5, const D3D12_ROOT_SIGNATURE_DESC& desc)
    {
        auto d3d = Direct3D::Get();

        stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        stateSubobject.pDesc = &localRootSignature;

        auto rootSignatureResult = d3d->CreateRootSignature(desc);
        CHECKSHOW(rootSignatureResult.Valid(), "Unable to create a local root signature");
        rootSignature = rootSignatureResult.Get();

        lrsInterface = rootSignature.Get();
        localRootSignature.pLocalRootSignature = lrsInterface;
        
    }

    D3D12_STATE_SUBOBJECT stateSubobject = {};
    D3D12_LOCAL_ROOT_SIGNATURE localRootSignature;

    ComPtr<ID3D12RootSignature> rootSignature;
    ID3D12RootSignature* lrsInterface;
};


struct ExportAssociations
{
    ExportAssociations(const WCHAR* exportNames[], uint32_t exportCount, const D3D12_STATE_SUBOBJECT* subobjectToAssociate)
    {
        stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;

        association.NumExports = exportCount;
        association.pExports = exportNames;
        association.pSubobjectToAssociate = subobjectToAssociate;

        stateSubobject.pDesc = &association;
    }

    D3D12_STATE_SUBOBJECT stateSubobject = {};
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
};

struct ShaderConfig
{
    ShaderConfig(uint32_t maxAttributeSize, uint32_t maxPayloadSize)
    {
        stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;

        config.MaxAttributeSizeInBytes = maxAttributeSize;
        config.MaxPayloadSizeInBytes = maxPayloadSize;

        stateSubobject.pDesc = &config;
    }
    
    D3D12_STATE_SUBOBJECT stateSubobject = {};
    D3D12_RAYTRACING_SHADER_CONFIG config = {};
};

struct PipelineConfig
{
    PipelineConfig(uint32_t maxRecursionDepth)
    {
        stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;

        pipelineConfig.MaxTraceRecursionDepth = maxRecursionDepth;

        stateSubobject.pDesc = &pipelineConfig;
    }

    D3D12_STATE_SUBOBJECT stateSubobject = {};
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
};


struct GlobalRootSignature
{
    GlobalRootSignature(ComPtr<ID3D12Device5> device5, const D3D12_ROOT_SIGNATURE_DESC& desc)
    {
        auto d3d = Direct3D::Get();
        stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;

        auto rootSignatureResult = d3d->CreateRootSignature(desc);
        CHECKSHOW(rootSignatureResult.Valid(), "Unable to create a global root signature");
        rootSignature = rootSignatureResult.Get();

        grsInterface = rootSignature.Get();
        globalRootSignature.pGlobalRootSignature = grsInterface;

        stateSubobject.pDesc = &globalRootSignature;
    }

    D3D12_STATE_SUBOBJECT stateSubobject = {};
    D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature;
    ComPtr<ID3D12RootSignature> rootSignature;
    ID3D12RootSignature* grsInterface;
};
