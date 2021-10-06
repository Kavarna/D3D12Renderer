#include "MaterialManager.h"

MaterialManager::Material *MaterialManager::AddMaterial(unsigned int maxDirtyFrames, const std::string &materialName, const MaterialConstants &info)
{
    if (auto materialIt = mMaterials.find(materialName); materialIt != mMaterials.end())
    {
        SHOWINFO("Material {} already found in material manager. Using it . . .", materialName);
        return &((*materialIt).second);
    }

    SHOWINFO("Material {} not found in material manager. Adding it . . .", materialName);
    mMaterials.insert({ materialName, Material(maxDirtyFrames, (uint32_t)mMaterials.size(), materialName, info) });
    return AddMaterial(maxDirtyFrames, materialName, info);
}

