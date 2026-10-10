#pragma once

#include "Core.h"
#include "Matrix.h"
#include "Vector.h"
#include "RenderInfo.h"
#include "FRenderPipeline.h"
#include "NvapiHelpers.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace RenderUtils
{
	static Microsoft::WRL::ComPtr<IDXGIAdapter1> FindHighPerformanceAdapter()
	{
		Microsoft::WRL::ComPtr<IDXGIFactory1> Factory1;
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&Factory1))))
		{
			return {};
		}

		Microsoft::WRL::ComPtr<IDXGIFactory6> Factory6;
		if (FAILED(Factory1.As(&Factory6)))
		{
			return {};
		}

		for (UINT Index = 0; Index < 16; ++Index)
		{
			Microsoft::WRL::ComPtr<IDXGIAdapter1> Adapter;
			const HRESULT Hr = Factory6->EnumAdapterByGpuPreference(
				Index,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(&Adapter));
			if (Hr == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}
			if (FAILED(Hr))
			{
				continue;
			}

			DXGI_ADAPTER_DESC1 Description{};
			if (SUCCEEDED(Adapter->GetDesc1(&Description)) &&
				(Description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
			{
				return Adapter;
			}
		}

		return {};
	}

	static UINT GetByteSizeFromFormat(DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return 16;
		case DXGI_FORMAT_R32G32B32_FLOAT:
			return 12;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return 8;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return 4;
		default:
			return 0; // Unknown format
		}
	}

	static void CompileShaderFromMemory(ID3D11Device* Device, const FString& Memory, Microsoft::WRL::ComPtr<ID3D11VertexShader>& VertexShader, Microsoft::WRL::ComPtr<ID3D11PixelShader>& PixelShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& InputLayout, uint32& Stride)
	{
		ID3DBlob* VertexShaderCSO;
		ID3DBlob* PixelShaderCSO;
		HRESULT Result;

		ID3DBlob* VSErrorBlob;
		Result = D3DCompile(Memory.c_str(), Memory.Len(), nullptr, nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &VertexShaderCSO, &VSErrorBlob);
		if (SUCCEEDED(Result))
		{
			Device->CreateVertexShader(VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), nullptr, VertexShader.GetAddressOf());
		}
		else if (VSErrorBlob)
		{
			OutputDebugStringA((char*)VSErrorBlob->GetBufferPointer());
			VSErrorBlob->Release();
		}

		ID3DBlob* PSErrorBlob;
		Result = D3DCompile(Memory.c_str(), Memory.Len(), nullptr, nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &PixelShaderCSO, &PSErrorBlob);
		if (SUCCEEDED(Result))
		{
			Device->CreatePixelShader(PixelShaderCSO->GetBufferPointer(), PixelShaderCSO->GetBufferSize(), nullptr, PixelShader.GetAddressOf());
		}
		else if (PSErrorBlob)
		{
			OutputDebugStringA((char*)PSErrorBlob->GetBufferPointer());
			PSErrorBlob->Release();
		}

		if (VertexShaderCSO)
		{
			// NOTE: 나중에 HLSL Reflection을 이용해서 InputLayout을 자동으로 생성하도록 개선, 추가로 캐싱해서 재사용 가능하도록 Pool을 만들어도 좋음
			D3D11_INPUT_ELEMENT_DESC Layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			Device->CreateInputLayout(Layout, ARRAYSIZE(Layout), VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), InputLayout.GetAddressOf());
			Stride = sizeof(FVertex);

			VertexShaderCSO->Release();
		}

		if (PixelShaderCSO)
		{
			PixelShaderCSO->Release();
		}
	}

	static void CompileShader(ID3D11Device* Device, const FString& ShaderPath, Microsoft::WRL::ComPtr<ID3D11VertexShader>& VertexShader, Microsoft::WRL::ComPtr<ID3D11PixelShader>& PixelShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& InputLayout, uint32& Stride)
	{
		std::ifstream FileStream(ShaderPath.ToString(), std::ios::in | std::ios::binary);
		if (!FileStream)
		{
			return;
		}

		std::stringstream Buffer;
		Buffer << FileStream.rdbuf();

		FString Memory(Buffer.str());
		CompileShaderFromMemory(Device, Memory, VertexShader, PixelShader, InputLayout, Stride);
	}

	static ID3D11Buffer* CreateConstantBuffer(ID3D11Device* Device, UINT Size)
	{
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.ByteWidth = Size + 0xf & 0xfffffff0;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		ID3D11Buffer* ConstantBuffer = nullptr;
		HRESULT Hr = Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffer);
		if (FAILED(Hr))
		{
			return nullptr;
		}

		return ConstantBuffer;
	}
}
