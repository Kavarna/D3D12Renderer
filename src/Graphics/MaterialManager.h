#pragma once


#include <Oblivion.h>
#include "FrameResources.h"
#include "Utils/UpdateObject.h"

class MaterialManager : public ISingletone<MaterialManager>
{
    MAKE_SINGLETONE_CAPABLE(MaterialManager);

public:
    struct Material : public UpdateObject
    {
        friend class MaterialManager;
        Material(unsigned int maxDirtyFrames, unsigned int cbIndex,
                 std::string materialName, const MaterialConstants &info):
            UpdateObject(maxDirtyFrames, cbIndex), Name(materialName), Info(info)
        {
            MarkUpdate();
        };

        MaterialConstants GetMaterialConstants() const
        {
            return Info;
        }

        unsigned int GetTextureIndex() const
        {
            return Info.textureIndex;
        }

        std::string Name;
    private:
        MaterialConstants Info;
    };

public:
    MaterialManager() = default;
    ~MaterialManager() = default;

public:
    Material *AddMaterial(unsigned int maxDirtyFrames, const std::string &materialName, const MaterialConstants &);
    Material *AddDefaultMaterial(unsigned int maxDirtyFrames);
    void UpdateMaterialsBuffer(UploadBuffer<MaterialConstants> &materialsBuffer);
    
    Material *GetMaterial(const std::string& material);

    uint32_t GetNumMaterials() const;
    void CloseAddingMaterials();

public:
    bool mCanAddMaterial = true;
    std::unordered_map<std::string, Material> mMaterials;

};


