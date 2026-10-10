#pragma once

#include "FPostProcess.h"
#include "FRenderGraph.h"
#include "FRenderPipeline.h"
#include "Renderer.h"

class FNormalPreviewProcess : public FPostProcess
{
public:
	FNormalPreviewProcess()
	{
		SetEnabled(false);
	}

	~FNormalPreviewProcess() override = default;

	FRGTextureRef AddPasses(FRenderGraph& RenderGraph, const FPostProcessInputs& Inputs, const FPostProcessContext& Context) override;

private:
	TSharedPtr<FRenderPipeline> Pipeline;

	FRenderTarget2D RenderTarget;
};
