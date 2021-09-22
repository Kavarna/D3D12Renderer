#pragma once


// Windows stuff
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined max
#undef max
#endif
#if defined min
#undef min
#endif

// Com stuff
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// DirectX stuff
#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include <DirectXMath.h>
#include <directxcollision.h>
#include <d3dcompiler.h>
#include <wincodec.h>


// Standard stuff
#include <exception>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <chrono>
#include <functional>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <thread>
#include <memory>
#include <cstring>
#include <type_traits>
#include <queue>
#include <array>
#include <iostream>
#include <filesystem>
#include <random>


// My stuff
#include "Logger.h"
#include "CommonMath.h"
#include "d3dx12.h"
#include "Events.h"
#include "KeyCodes.h"
#include "Result.h"
#include "Exceptions.h"

#define _KiB(x) (x * 1024)
#define _MiB(x) (x * 1024 * 1024)

#define _64KiB _KiB(64)
#define _1MiB _MiB(1)
#define _2MiB _MiB(2)
#define _4MiB _MiB(4)
#define _8MiB _MiB(8)
#define _16MiB _MiB(16)
#define _32MiB _MiB(32)
#define _64MiB _MiB(64)
#define _128MiB _MiB(128)
#define _256MiB _MiB(256)

#define OBLIVION_ALIGN(x) __declspec(align(x))



