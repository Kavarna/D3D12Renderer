#pragma once


#include <Oblivion.h>

OBLIVION_ALIGN(16)
class ICamera
{
public:
    virtual const DirectX::XMMATRIX &__vectorcall GetView() const = 0;
    virtual const DirectX::XMMATRIX &__vectorcall GetProjection() const = 0;
};