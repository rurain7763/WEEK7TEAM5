#include "FNormalPreviewProcess.h"

FRGTextureRef FNormalPreviewProcess::AddPasses(FRenderGraph& RenderGraph, const FPostProcessInputs& Inputs, const FPostProcessContext& Context)
{
	if (Inputs.OverrideOutputTexture == InvalidRGTextureRef)
	{
		throw std::runtime_error("FNormalPreviewProcess: OverrideOutputTexture must be set");
	}

	RenderGraph.AddPass(FName("NormalPreviewProcess"), [this, &RenderGraph, Inputs, Context](URenderer& Renderer)
		{
			if (!Pipeline)
			{
				Pipeline = Renderer.CreateRenderPipeline();
				Pipeline->SetShader("Assets/Shaders/NormalPreviewProcess.hlsl");
				Pipeline->SetRasterRizerState(D3D11_CULL_BACK);
				Pipeline->SetDepthStencilState(false, false);
				Pipeline->SetBlendState(ERenderBlendMode::Opaque);
				Pipeline->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
			}

			FRGTexture& OutputTexture = RenderGraph.GetTexture(Inputs.OverrideOutputTexture);
			FRGTexture& InputNormalTexture = RenderGraph.GetTexture(Inputs.InputNormalTexture);
			FRGTexture& InputDepthTexture = RenderGraph.GetTexture(Inputs.InputDepthTexture);

			RenderTarget.Texture = OutputTexture.Texture->Texture;
			RenderTarget.RTV = OutputTexture.RTV.Get();
			RenderTarget.Width = OutputTexture.Texture->Width;
			RenderTarget.Height = OutputTexture.Texture->Height;

			Renderer.BindRenderTarget(&RenderTarget, nullptr, true);

			Pipeline->SetShaderResource(0, InputNormalTexture.SRV);

			Renderer.Render(Pipeline.get(), 6);

			Renderer.ClearAllShaderResources();
		});

	return Inputs.OverrideOutputTexture;
}
