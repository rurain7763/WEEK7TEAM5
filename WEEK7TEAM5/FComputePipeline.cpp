#include "pch.h"
#include "FComputePipeline.h"
#include "Renderer.h"

FComputePipeline::FComputePipeline(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, FSamplerStatePool* InSamplerStatePool)
	: Device(InDevice)
	, DeviceContext(InDeviceContext)
	, SamplerStatePool(InSamplerStatePool)
{
}

FComputePipeline::~FComputePipeline()
{
	Release();
}

void FComputePipeline::Release()
{
	// 해제 요청 자체로 이전 바인딩 재사용 판단을 무효화합니다.
	Version = 0;

	for (int32 Index = 0; Index < ConstantBuffers.Num(); Index++)
	{
		if (ConstantBuffers[Index] && !ExternalConstantBuffers[Index])
		{
			ConstantBuffers[Index]->Release();
			ConstantBuffers[Index] = nullptr;
		}
	}
}

void FComputePipeline::SetShader(const FString& ShaderPath, const D3D_SHADER_MACRO* Macros)
{
	++Version;

	Shader = MakeShared<FComputeShader>();
	RenderUtils::CompileComputeShader(Device, ShaderPath, Macros, Shader->ComputeShader);
}

void FComputePipeline::SetShader(const TSharedPtr<FComputeShader>& InShader)
{
	++Version;

	Shader = InShader;
}

void FComputePipeline::SetShaderResource(uint32 Slot, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	if (Slot >= static_cast<uint32>(ShaderResourceViews.Num()))
	{
		ShaderResourceViews.SetNum(Slot + 1);
	}

	if (ShaderResourceViews[Slot] == SRV.Get())
	{
		return;
	}

	ShaderResourceViews[Slot] = SRV.Get();
	++Version;
}

void FComputePipeline::ClearShaderResource()
{
	ShaderResourceViews.Empty();
}

void FComputePipeline::SetUnorderedAccess(uint32 Slot, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> UAV)
{
	if (Slot >= static_cast<uint32>(UnorderedAccessViews.Num()))
	{
		UnorderedAccessViews.SetNum(Slot + 1);
	}

	if (UnorderedAccessViews[Slot] == UAV.Get())
	{
		return;
	}

	UnorderedAccessViews[Slot] = UAV.Get();
	++Version;
}

void FComputePipeline::ClearUnorderedAcess()
{
	++Version;
	UnorderedAccessViews.Empty();
}

void FComputePipeline::SetSamplerState(uint32 Slot, D3D11_FILTER Filter, D3D11_TEXTURE_ADDRESS_MODE AddressU, D3D11_TEXTURE_ADDRESS_MODE AddressV)
{
	FSamplerStateKey Key{ Filter, AddressU, AddressV };
	ID3D11SamplerState* SamplerState = SamplerStatePool->GetOrCreateSamplerState(Device, Key);
	if (Slot >= static_cast<uint32>(SamplerStates.Num()))
	{
		SamplerStates.SetNum(Slot + 1);
	}

	SamplerStates[Slot] = SamplerState;
	++Version;
}

void FComputePipeline::ClearSamplerState()
{
	++Version;
	SamplerStates.Empty();
}
