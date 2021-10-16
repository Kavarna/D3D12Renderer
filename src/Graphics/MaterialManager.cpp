#include "MaterialManager.h"

MaterialManager::Material *MaterialManager::AddMaterial(unsigned int maxDirtyFrames, const std::string &materialName,
                                                        const MaterialConstants &info)
{
    CHECK(mCanAddMaterial, nullptr, "Material manager is closed for adding materials");

    if (auto materialIt = mMaterials.find(materialName); materialIt != mMaterials.end())
    {
        SHOWINFO("Material {} already found in material manager. Using it . . .", materialName);
        return &((*materialIt).second);
    }

    SHOWINFO("Material {} not found in material manager. Adding it . . .", materialName);
    mMaterials.insert({ materialName, Material(maxDirtyFrames, (uint32_t)mMaterials.size(), materialName, info) });
    return AddMaterial(maxDirtyFrames, materialName, info);
}

void MaterialManager::UpdateMaterialsBuffer(UploadBuffer<MaterialConstants> &materialsBuffer)
{
    for (auto &it : mMaterials)
    {
        auto &material = it.second;
        if (material.DirtyFrames > 0)
        {
            auto mappedMemory = materialsBuffer.GetMappedMemory(material.ConstantBufferIndex);
            memcpy(mappedMemory, &it.second.Info, sizeof(it.second.Info));
            material.DirtyFrames--;
        }
    }
}

uint32_t MaterialManager::GetNumMaterials() const
{
    return (uint32_t)mMaterials.size();
}

void MaterialManager::CloseAddingMaterials()
{
    mCanAddMaterial = false;
}

