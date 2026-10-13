#pragma once

#include "Renderer.h"
#include "RenderUtils.h"

class FLightShader : public FShader
{
public:
	FLightShader(const FString& ShaderPath)
		: ShaderPath(ShaderPath)
	{
	}

	inline void SetLightingEnabled(bool bEnabled)
	{
		const char* MacroValue = bEnabled ? "1" : "0";
		if (Macros[0].Definition == MacroValue)
		{
			return;
		}

		Macros[0].Definition = MacroValue;
		bDirty = true;
	}

	inline void SetGouraudModeEnabled(bool bEnabled)
	{
		const char* MacroValue = bEnabled ? "1" : "0";
		if (Macros[1].Definition == MacroValue)
		{
			return;
		}

		Macros[1].Definition = MacroValue;
		bDirty = true;
	}

	inline void UseLambertModel()
	{
		const char* MacroValue = "0";
		if (Macros[2].Definition == MacroValue)
		{
			return;
		}

		Macros[2].Definition = MacroValue;
		bDirty = true;
	}

	inline void UseBlinnPhongModel()
	{
		const char* MacroValue = "1";
		if (Macros[2].Definition == MacroValue)
		{
			return;
		}

		Macros[2].Definition = MacroValue;
		bDirty = true;
	}

	inline void Compile()
	{
		if (!bDirty)
		{
			return;
		}

		if (VertexShader)
		{
			VertexShader->Release();
		}

		if (PixelShader)
		{
			PixelShader->Release();
		}

		if (InputLayout)
		{
			InputLayout->Release();
		}

		RenderUtils::CompileShader(Device, ShaderPath, Macros, VertexShader, PixelShader, InputLayout, Stride);
		bDirty = false;
	}

private:
	bool bDirty = true;

	FString ShaderPath;
	D3D_SHADER_MACRO Macros[4] = { 
		{ "ENABLE_LIGHTING", "0" },
		{ "LIGHTING_MODEL_GOURAUD", "0" },
		{ "LIGHTING_MODEL_BLINN_PHONG", "0" },
		{ nullptr, nullptr }
	};
};