#include "Model.h"

bool Model::Create(ModelType type)
{
	switch (type)
	{
		case Model::ModelType::Triangle:

			break;
		default:
			SHOWFATAL("Model type {} is not a valid model", (int)type);
			return false;
	}

    return true;
}

bool Model::CreateTriangle()
{
	return true;
}
