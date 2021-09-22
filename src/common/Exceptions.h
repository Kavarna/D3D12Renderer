#pragma once

#include <exception>
#include <comdef.h>
#include <corecrt_wstring.h>
#include <string>
#include "Conversions.h"


inline const TCHAR* GetStringFromHr(HRESULT hr) {
    _com_error err(hr);
    return err.ErrorMessage();
}

#define ThrowIfFailed(hr) {\
    if (FAILED(hr))\
    {\
        SHOWFATAL("COM operation failed with code {}: {}", hr, GetStringFromHr(hr));\
    }\
}
