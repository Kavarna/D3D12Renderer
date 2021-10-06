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
        Material(unsigned int maxDirtyFrames, unsigned int cbIndex,
                 std::string materialName, const MaterialConstants &info):
            UpdateObject(maxDirtyFrames, cbIndex), Name(materialName), Info(info)
        {
        };

        std::string Name;
    private:
        MaterialConstants Info;
    };

public:
    MaterialManager() = default;
    ~MaterialManager() = default;

public:
    Material *AddMaterial(unsigned int maxDirtyFrames, const std::string &materialName, const MaterialConstants &);

public:
    std::unordered_map<std::string, Material> mMaterials;

};


