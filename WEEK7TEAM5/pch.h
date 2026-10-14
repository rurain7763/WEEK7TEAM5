#pragma once

// 프리컴파일드 헤더 (premake의 forceincludes로 엔진 .cpp 맨 앞에 자동으로 포함된다)
// 자주 바뀌지 않는 표준/외부 헤더만 둔다.
// 엔진 헤더(Core.h, Vector.h 등)를 넣으면 그 헤더를 고칠 때마다 전체가 다시 빌드되므로 넣지 않는다.

// C/C++ 표준 라이브러리
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Windows / DirectX
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

// 서드파티
#include <nvapi/nvapi_lite_common.h>
#include <nvapi/nvapi.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "imgui.h"
