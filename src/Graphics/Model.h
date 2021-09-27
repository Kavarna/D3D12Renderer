#pragma once



#include "Oblivion.h"



class Model
{
public:
    enum class ModelType
    {
        Triangle = 0
    };
    static constexpr const char *ModelTypeString[] =
    {
        "Triangle"
    };
public:
    Model() = default;
    ~Model() = default;

public:
    bool Create(ModelType type);

private:
    bool CreateTriangle();
};
