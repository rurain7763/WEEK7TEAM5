project "WEEK7TEAM5"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    systemversion "latest"
    characterset "Unicode"

    -- 소스 파일을 여러 코어에서 동시에 컴파일한다 (/MP)
    multiprocessorcompile "On"

    -- 프리컴파일드 헤더. 엔진 .cpp에는 pch.h가 강제로 포함되므로 .cpp마다 include할 필요가 없다
    pchheader "pch.h"
    pchsource "pch.cpp"
    forceincludes { "pch.h" }

    targetdir ("bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.platform}/%{cfg.buildcfg}")
    debugdir "%{wks.location}"

    files {
        "**.cpp",
        "**.h",
        "../Assets/**",
    }

    removefiles {
        "nvapi/Sample_Code/**",
    }

    -- 에셋은 탐색기에 보이기만 하고 빌드에는 걸리지 않게 한다
    vpaths {
        ["Assets/*"] = "../Assets/**",
    }

    filter "files:**.hlsl"
        buildaction "None"

    -- 서드파티 소스는 자체 설정(예: imgui.cpp는 imgui.h보다 먼저 IMGUI_DEFINE_MATH_OPERATORS를 정의)이 있으므로
    -- PCH를 쓰지 않는다. 파일 단위 강제 포함은 프로젝트 값을 덮어쓰는 방식으로만 지정할 수 있어서,
    -- pch.h 대신 아무 영향이 없는 표준 헤더를 강제 포함시킨다.
    filter "files:ImGui/**.cpp or files:Stb/**.cpp or files:ThirdParty/**.cpp"
        enablepch "Off"
        forceincludes { "cstddef" }

    filter {}

    includedirs {
        ".",
        "nvapi",
        "ImGui",
        "Json",
        "%{wks.location}/Vendor/include",
    }

    libdirs {
        "%{wks.location}/Vendor/lib/%{cfg.buildcfg}",
    }

    links {
        "d3d11",
        "d3dcompiler",
        "dwrite",
        "d2d1",
        "dxgi",
        "dwmapi",
        "gdi32",
        "imm32",
        "user32",
        "comdlg32",
        "Ole32",
	"nvapi64"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        links { 
            "freetyped" 
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "Off"
        symbols "On"
        links { 
            "freetype" 
        }

    filter "configurations:ObjViewerRelease"
        runtime "Release"
        optimize "Off"
        symbols "On"
        -- ObjViewerRelease는 Release용 외부 라이브러리를 재사용한다.
        libdirs { "%{wks.location}/Vendor/lib/Release" }
        defines { "IS_OBJ_VIEWER=1" }
        links {
            "freetype" 
        }

    filter "platforms:x64"
        libdirs { "nvapi/amd64" }
        links { "nvapi64" }

    filter "platforms:x86"
        libdirs { "nvapi/x86" }
        links { "nvapi" }

    filter {}



    filter "system:windows"
        defines {
            "UNICODE",
            "_UNICODE",
            "WIN32_LEAN_AND_MEAN"
        }

filter "toolset:msc*"
    buildoptions { "/utf-8" }

filter {}
