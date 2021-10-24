#pragma once


#include <Oblivion.h>
#include "Utils/UpdateObject.h"
#include "FrameResources.h"

class SceneLight : public UpdateObject
{
public:
    SceneLight() = default;
    SceneLight(unsigned int maxDirtyFrames);

    void Init(unsigned int maxDirtyFrames, unsigned int cbIndex = 0);

public:
    void SetAmbientColor(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f);
    void SetAmbientColor(DirectX::XMFLOAT4 &&color);
    bool AddDirectionalLight(std::string &&name, DirectX::XMFLOAT3 &&direction, DirectX::XMFLOAT3 &&strength);
    bool AddPointLight(std::string &&name, DirectX::XMFLOAT3 &&Position, DirectX::XMFLOAT3 &&strength, float falloffStart, float falloffEnd);
    bool AddSpotlight(std::string &&name, DirectX::XMFLOAT3 &&Position, DirectX::XMFLOAT3 &&Direction,
                      DirectX::XMFLOAT3 &&strength, float falloffStart, float falloffEnd, float spotPower);

public:
    unsigned int GetLightCount() const;

    const std::vector<std::string> &GetDirectionalLightsNames() const;
    Result<const LightCB *> GetDirectionalLight(int index) const;
    void UpdateDirectionalLight(int index, DirectX::XMFLOAT3 &&direction, DirectX::XMFLOAT3 &&strength);
    void DeleteDirectionalLight(int index);

    const std::vector<std::string> &GetPointLightsNames() const;
    Result<const LightCB *> GetPointLight(int index) const;
    void UpdatePointLight(int index, DirectX::XMFLOAT3 &&Position, DirectX::XMFLOAT3 &&strength, float falloffStart, float falloffEnd);
    void DeletePointLight(int index);

    const std::vector<std::string> &GetSpotlightNames() const;
    Result<const LightCB *> GetSpotLight(int index) const;
    void UpdateSpotlight(int index, DirectX::XMFLOAT3 &&Position, DirectX::XMFLOAT3 &&Direction,
                         DirectX::XMFLOAT3 &&strength, float falloffStart, float falloffEnd, float spotPower);
    void DeleteSpotlight(int index);

    DirectX::XMFLOAT4 &GetAmbientColor();

    void UpdateLightsBuffer(UploadBuffer<LightsBuffer> &buffer);
    void UpdateLightsBuffer(LightsBuffer *lb) const;

private:
    DirectX::XMFLOAT4 mAmbientColor;

    std::vector<LightCB> mDirectionalLights;
    std::vector<std::string> mDirectionalLightNames;

    std::vector<LightCB> mPointLights;
    std::vector<std::string> mPointLightsNames;

    std::vector<LightCB> mSpotlights;
    std::vector<std::string> mSpotlightsNames;
};


