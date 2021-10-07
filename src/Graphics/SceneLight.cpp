#include "SceneLight.h"

SceneLight::SceneLight(unsigned int maxDirtyFrames):
    UpdateObject(maxDirtyFrames, 0)
{
}

void SceneLight::SetAmbientColor(float r, float g, float b, float a)
{
    SetAmbientColor({ r, g, b, a });
}

void SceneLight::SetAmbientColor(DirectX::XMFLOAT4 &&color)
{
    mAmbientColor = color;
    MarkUpdate();
}

bool SceneLight::AddDirectionalLight(std::string &&name, DirectX::XMFLOAT3 &&direction, DirectX::XMFLOAT3 &&strength)
{
    unsigned int allLights = GetLightCount();
    CHECK(allLights < MAX_LIGHTS, false, "Maximum lights count {} reached: {}", MAX_LIGHTS, allLights);
    CHECK(!(direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f), false, "Cannot set a null direction");

    direction = Math::Normalize(direction);


    LightCB lightCB;
    lightCB.Direction = direction;
    lightCB.Strength = strength;
    mDirectionalLights.emplace_back(std::move(lightCB));
    mDirectionalLightNames.emplace_back(name);

    MarkUpdate();
    return true;
}

bool SceneLight::AddPointLight(std::string &&name, DirectX::XMFLOAT3 &&Position, DirectX::XMFLOAT3 &&strength, float falloffStart, float falloffEnd)
{
    unsigned int allLights = GetLightCount();
    CHECK(allLights < MAX_LIGHTS, false, "Maximum lights count {} reached: ", MAX_LIGHTS, allLights);

    LightCB lightCB;
    lightCB.Position = Position;
    lightCB.Strength = strength;
    lightCB.FalloffStart = falloffStart;
    lightCB.FalloffEnd = falloffEnd;

    mPointLights.push_back(std::move(lightCB));
    mPointLightsNames.emplace_back(name);

    MarkUpdate();
    return true;
}

bool SceneLight::AddSpotlight(std::string &&name, DirectX::XMFLOAT3 &&Position, DirectX::XMFLOAT3 &&direction, DirectX::XMFLOAT3 &&strength, float falloffStart, float falloffEnd, float spotPower)
{
    unsigned int allLights = GetLightCount();
    CHECK(allLights < MAX_LIGHTS, false, "Maximum lights count {} reached: {}", MAX_LIGHTS, allLights);
    CHECK(!(direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f), false, "Cannot set a null direction");

    LightCB lightCB;
    lightCB.Position = Position;
    lightCB.Direction = direction;
    lightCB.Strength = strength;
    lightCB.FalloffStart = falloffStart;
    lightCB.FalloffEnd = falloffEnd;
    lightCB.SpotPower = spotPower;

    mSpotlights.push_back(std::move(lightCB));
    mSpotlightsNames.emplace_back(name);

    MarkUpdate();
    return true;
}

unsigned int SceneLight::GetLightCount() const
{
    return (unsigned int)(mDirectionalLights.size());
}

const std::vector<std::string> &SceneLight::GetDirectionalLightsNames() const
{
    return mDirectionalLightNames;
}

Result<const LightCB *> SceneLight::GetDirectionalLight(int index) const
{
    CHECK(index >= 0 && index < mDirectionalLights.size(), std::nullopt,
          "Unable to get directional light at index {}. Maximum index = ", index, mDirectionalLights.size() - 1);

    return &mDirectionalLights[(unsigned int)index];
}

void SceneLight::UpdateDirectionalLight(int index, DirectX::XMFLOAT3 &&direction, DirectX::XMFLOAT3 &&strength)
{
    CHECKRET(index >= 0 && index < mDirectionalLights.size(),
             "Unable to update directional light at index {}. Maximum index = ", index, mDirectionalLights.size() - 1);

    direction = Math::Normalize(direction);

    mDirectionalLights[index].Direction = direction;
    mDirectionalLights[index].Strength = strength;

    MarkUpdate();
}

void SceneLight::DeleteDirectionalLight(int index)
{
    CHECKRET(index >= 0 && index < mDirectionalLights.size(),
             "Unable to delete directional light at index {}. Maximum index = ", index, mDirectionalLights.size() - 1);

    mDirectionalLights.erase(mDirectionalLights.begin() + index);
    mDirectionalLightNames.erase(mDirectionalLightNames.begin() + index);

    MarkUpdate();
}

const std::vector<std::string> &SceneLight::GetPointLightsNames() const
{
    return mPointLightsNames;
}

Result<const LightCB *> SceneLight::GetPointLight(int index) const
{
    CHECK(index >= 0 && index < mPointLights.size(), std::nullopt,
          "Unable to get point light at index {}. Maximum index = ", index, mPointLights.size() - 1);

    return &mPointLights[index];
}

void SceneLight::UpdatePointLight(int index, DirectX::XMFLOAT3 &&Position, DirectX::XMFLOAT3 &&strength, float falloffStart, float falloffEnd)
{
    CHECKRET(index >= 0 && index < mPointLights.size(),
             "Unable to update point light at index {}. Maximum index = ", index, mPointLights.size() - 1);

    mPointLights[index].Position = Position;
    mPointLights[index].Strength = strength;
    mPointLights[index].FalloffStart = falloffStart;
    mPointLights[index].FalloffEnd = falloffEnd;

    MarkUpdate();
}

void SceneLight::DeletePointLight(int index)
{
    CHECKRET(index >= 0 && index < mPointLights.size(),
             "Unable to delete point light at index {}. Maximum index = ", index, mPointLights.size() - 1);

    mPointLights.erase(mPointLights.begin() + index);
    mPointLightsNames.erase(mPointLightsNames.begin() + index);

    MarkUpdate();
}

const std::vector<std::string> &SceneLight::GetSpotlightNames() const
{
    return mSpotlightsNames;
}

Result<const LightCB *> SceneLight::GetSpotLight(int index) const
{
    CHECK(index >= 0 && index < mSpotlights.size(), std::nullopt,
          "Unable to get spotlight at index {}. Maximum index = ", index, mSpotlights.size() - 1);

    return &mSpotlights[index];
}

void SceneLight::UpdateSpotlight(int index, DirectX::XMFLOAT3 &&Position, DirectX::XMFLOAT3 &&Direction, DirectX::XMFLOAT3 &&strength, float falloffStart, float falloffEnd, float spotPower)
{
    CHECKRET(index >= 0 && index < mSpotlights.size(),
             "Unable to update spotlight at index {}. Maximum index = ", index, mSpotlights.size() - 1);

    Direction = Math::Normalize(Direction);

    mSpotlights[index].Position = Position;
    mSpotlights[index].Direction = Direction;
    mSpotlights[index].Strength = strength;
    mSpotlights[index].FalloffStart = falloffStart;
    mSpotlights[index].FalloffEnd = falloffEnd;
    mSpotlights[index].SpotPower = spotPower;

    MarkUpdate();
}

void SceneLight::DeleteSpotlight(int index)
{
    CHECKRET(index >= 0 && index < mSpotlights.size(),
             "Unable to delete spotlight at index {}. Maximum index = ", index, mSpotlights.size() - 1);

    mSpotlights.erase(mSpotlights.begin() + index);
    mSpotlightsNames.erase(mSpotlightsNames.begin() + index);

    MarkUpdate();
}

DirectX::XMFLOAT4 &SceneLight::GetAmbientColor()
{
    return mAmbientColor;
}

void SceneLight::UpdateLightsBuffer(UploadBuffer<LightsBuffer> &buffer)
{
    if (DirtyFrames)
    {
        UpdateLightsBuffer(buffer.GetMappedMemory());
        DirtyFrames--;
    }
}

void SceneLight::UpdateLightsBuffer(LightsBuffer *lb) const
{
    lb->AmbientColor = mAmbientColor;
    unsigned int destinationIndex = 0;

    lb->NumDirectionalLights = (unsigned int)mDirectionalLights.size();
    if (lb->NumDirectionalLights)
    {
        memcpy(&lb->Lights[destinationIndex], mDirectionalLights.data(), sizeof(LightCB) * lb->NumDirectionalLights);
        destinationIndex += lb->NumDirectionalLights;
    }

    lb->NumPointLights = (unsigned int)mPointLights.size();
    if (lb->NumPointLights)
    {
        memcpy(&lb->Lights[destinationIndex], mPointLights.data(), sizeof(LightCB) * lb->NumPointLights);
        destinationIndex += lb->NumPointLights;
    }

    lb->NumSpotLights = (unsigned int)mSpotlights.size();
    if (lb->NumSpotLights)
    {
        memcpy(&lb->Lights[destinationIndex], mSpotlights.data(), sizeof(LightCB) * lb->NumSpotLights);
        destinationIndex += lb->NumSpotLights;
    }
}

