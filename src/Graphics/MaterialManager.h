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
                 std::string materialName, const MaterialConstants &info,
                 uint64_t textureDescriptor):
            UpdateObject(maxDirtyFrames, cbIndex), Name(materialName), Info(info), TextureDescriptor(textureDescriptor)
        {
        };

        uint64_t GetTextureDescriptor() const
        {
            return TextureDescriptor;
        };

        std::string Name;
    private:
        MaterialConstants Info;
        uint64_t TextureDescriptor = 0;
    };

public:
    MaterialManager() = default;
    ~MaterialManager() = default;

public:
    Material *AddMaterial(unsigned int maxDirtyFrames, const std::string &materialName,
                          const MaterialConstants &, uint64_t textureDescriptor);
    void UpdateMaterialsBuffer(UploadBuffer<MaterialConstants> &materialsBuffer);
    
    uint32_t GetNumMaterials() const;
    void CloseAddingMaterials();

public:
    bool mCanAddMaterial = true;
    std::unordered_map<std::string, Material> mMaterials;

};


