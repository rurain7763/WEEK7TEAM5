#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "Core.h"
#include "TArray.h"
#include "enum.h"
#include "RenderUtils.h"
#include <initializer_list>

class URenderer;
class FSamplerStatePool;
struct FComputeShader;

class FComputePipeline
{
public:
	FComputePipeline(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, FSamplerStatePool* InSamplerStatePool);
	~FComputePipeline();

	void Release();

	void SetShader(const FString& ShaderPath, const D3D_SHADER_MACRO* Macros = nullptr);
	void SetShader(const TSharedPtr<FComputeShader>& InShader);

	void SetShaderResource(uint32 Slot, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV);
	void ClearShaderResource();

	void SetUnorderedAccess(uint32 Slot, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> UAV);
	void ClearUnorderedAcess();

	void SetSamplerState(uint32 Slot, D3D11_FILTER Filter, D3D11_TEXTURE_ADDRESS_MODE AddressU, D3D11_TEXTURE_ADDRESS_MODE AddressV);
	void ClearSamplerState();

	inline uint32 GetVersion() const { return Version; }

	void SetConstantBuffer(uint32 Index, ID3D11Buffer* Buffer)
	{
		if (Index >= ConstantBuffers.Num())
		{
			ConstantBuffers.SetNum(Index + 1);
			ExternalConstantBuffers.SetNum(Index + 1);
		}

		ConstantBuffers[Index] = Buffer;
		ExternalConstantBuffers[Index] = true;
		++Version;
	}

	template <typename T>
	void AddConstantBuffer()
	{
		if (!Device)
		{
			return;
		}

		ID3D11Buffer* NewBuffer = RenderUtils::CreateConstantBuffer(Device, sizeof(T));
		if (NewBuffer)
		{
			ConstantBuffers.Add(NewBuffer);
			ExternalConstantBuffers.Add(false);
			++Version;
		}
	}

	template <typename T>
	void UpdateConstantBuffer(uint32 Index, const T& Data)
	{
		// 같은 버퍼의 내용만 바꾸므로 바인딩 버전은 유지합니다.
		if (DeviceContext && Index < static_cast<uint32>(ConstantBuffers.Num()))
		{
			ID3D11Buffer* ConstantBuffer = ConstantBuffers[Index];
			D3D11_MAPPED_SUBRESOURCE MappedResource;
			HRESULT Hr = DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			if (SUCCEEDED(Hr))
			{
				memcpy(MappedResource.pData, &Data, sizeof(T));
				DeviceContext->Unmap(ConstantBuffer, 0);
			}
		}
	}

private:
	friend class URenderer;

	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	FSamplerStatePool* SamplerStatePool = nullptr;
	TSharedPtr<FComputeShader> Shader;
	TArray<ID3D11Buffer*> ConstantBuffers;
	TArray<int8> ExternalConstantBuffers; // 외부에서 생성한 상수 버퍼인지 여부를 나타냅니다.
	TArray<ID3D11ShaderResourceView*> ShaderResourceViews;
	TArray<ID3D11UnorderedAccessView*> UnorderedAccessViews;
	TArray<ID3D11SamplerState*> SamplerStates;

	uint32 Version = 0;
};
