#pragma once

// windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#undef min
#undef max

using namespace DirectX;

// std
#include <cinttypes>
#include <cassert>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <type_traits>
#include <random>
#include <chrono>
#include <variant>
#include <algorithm>
