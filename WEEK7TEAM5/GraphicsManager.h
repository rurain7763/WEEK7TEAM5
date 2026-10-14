#pragma once

#include "Matrix.h"
#include "Enum.h"

#include "TArray.h"
#include "TMap.h"
#include "Renderer.h"
#include "Camera.h"
#include "RenderInfo.h"
#include "Vector.h"
#include "ShowFlags.h"
#include "FFogProcess.h"
#include "FDepthPreviewProcess.h"
#include "FNormalPreviewProcess.h"
#include "FFXAAProcess.h"
#include "FRenderGraph.h"

class FAssetManager;
struct FViewport;
class UWorld;
class UPrimitiveComponent;
class FLightShader;
class FTexture2DAsset;

struct FBuffer
{
	ID3D11Buffer* Buffer;
	uint32 SourceNum;
};

class FGraphicsManager
{
public:
	FGraphicsManager(HWND hWindow);
	~FGraphicsManager();

	void Initialize();
	void Cleanup();

	void Prepare(const FCamera* mCamera, float Aspect, const FMatrix& ViewProjection, const FMatrix& InvViewProjection, FViewport& Viewport, UWorld* World, EViewModeIndex InViewMode, EShaderStage InLightingEvaluationStage, ELightingModel InLightingModel, EViewportType InViewportType);

	void Render(const TArray<UPrimitiveComponent*>& Primitives);

	void Display();

	float GetAspect() const { return mAspect; }

	bool IsPerspectiveProjection() const;
	void SetPerspectiveProjection(bool bPerspectiveProjection);

	float GetPerspectiveRatio() const { return mProjectionRatio; }
	const FMatrix& GetViewProjectionMatrix() const { return mViewUnifiedProjectionMatrix; }
	void SetPerspectiveRatio(float ratio) { mProjectionRatio = FMath::Clamp(ratio, 0.0f, 1.0f); }

	float GetCameraOrthoDistance() const { return mCameraOrthoDistance; }
	void SetCameraOrthoDistance(float distance) { mCameraOrthoDistance = distance; }

	// Todo: Change name
	URenderer* GetRenderer() const;
	void OnResize(UINT width, UINT height);

	// Projection ratio smoothing
	void StartProjectionTransition(bool orthographic);
	bool IsOrthographicTarget() const;
	void UpdateProjectionTransition(float deltaTime);

	inline FRenderCollector& GetRenderCollector() { return mRenderCollector; }
	inline FFogProcess& GetFogProcess() { return mFogProcess; }

	inline int32 GetGridGap() { return GridGap; }
	void SetGridGap(int32 GridGap);

	struct FGpuTimerQuerySet
	{
		Microsoft::WRL::ComPtr<ID3D11Query> Disjoint;
		Microsoft::WRL::ComPtr<ID3D11Query> Begin;
		Microsoft::WRL::ComPtr<ID3D11Query> End;
		bool bIssued = false;
	};

	TArray<FGpuTimerQuerySet> GpuQueries;
	uint32 GpuQueryIndex = 0;

	float GpuRenderTime = 0.0f;
	void BeginGpuRenderTimer();
	void EndGpuRenderTimer();
	void UpdateGpuRenderTime();
	float GetGpuRenderTime() { return GpuRenderTime; }

	//Default Normal Map 가져오기
	TSharedPtr<FTexture2DAsset> GetDefaultNormalMap();

private:
	void ExecuteLightCulling(const FDepthStencil& DepthStencil);
	void RenderHighLight(const TArray<UPrimitiveComponent*>& Primitives);

private:
	struct FOutlineConstants
	{
		FVector4 OutlineColor;
		int32 StencilTexWidth;
		int32 StencilTexHeight;
		int32 OutlineRadius;
		int32 Padding;
	};

	struct FLightInfo
	{
		FVector Position;
		ELightType Type;

		FLinearColor Color;
		FVector Direction;
		float Range;

		float Intensity;
		float FallOf;
		float InnerConeAngle;
		float OuterConeAngle;
	};

	struct FPreDepthConstants
	{
		FMatrix Model;
	};

	struct FLightCullingConstants
	{
		FMatrix InvProjection;
		FMatrix ViewMatrix;
		uint32 Width;
		uint32 Height;
		uint32 LightCount;
		uint32 TileX;
	};

	struct FGlobalLightInfo
	{
		FLinearColor AmbientColor;
		FVector DirectionalDirection;
		float AmbientIntensity;
		FLinearColor DirectionalColor;
		float DirectionalIntensity;
		int32 LightCullingThreadX;
		int32 LightCullingThreadY;
		int32 LightCullingTileX;
		int32 LightCullingTileMaxLightCount;
		int32 Padding[3];
	};

	struct FMeshContants
	{
		FMatrix Matrix;
		FMatrix InvMatrix;
		FVector4 Color;

		FVector2 UVOffset;
		int32 UseVertexColor;
		uint32 BindedTextureMapFlags;

		int32 LightCount;
		float Shininess;
		int32 Padding[2];
	};

	struct FViewConstants
	{
		FMatrix ViewProjectionMatrix;
		FVector ViewPosition;
		float Padding;
	};

	constexpr static uint32 LightCullingThreadX = 16;
	constexpr static uint32 LightCullingThreadY = 16;
	constexpr static uint32 LightCullingTileMaxLightCount = 1024;

	URenderer* mRenderer;
	FMatrix mViewMatrix;
	FMatrix mProjectionMatrix;
	FMatrix mInvProjectionMatrix;
	FMatrix mViewProjectionMatrix;
	FMatrix mInvViewProjectionMatrix;
	FMatrix mViewOrthogonalProjectionMatrix;
	FMatrix mViewUnifiedProjectionMatrix;
	FViewport* mViewport;

	// Prepare에서 갱신. 하이라이트 두께의 픽셀 → 월드 환산에 쓴다
	FVector mCameraLocation;
	FVector mCameraForward;
	float mCameraFovDegree = 60.0f;
	float mCameraOrthoDistance = 10.0f;
	float mCameraNear = 0.1f;
	float mCameraFar = 1000.0f;

	EViewportType mViewportType = EViewportType::Perspective;
	bool mbPerspectiveProjection;
	float mAspect;
	float mProjectionRatio; // 0.0f ~ 1.0f, 0이면 직교, 1이면 원근, 그 사이면 혼합

	// Projection ratio smoothing
	float mProjectionStartRatio = 1.0f;
	float mProjectionTargetRatio = 1.0f;
	float mProjectionElapsed = 0.0f;
	float mProjectionDuration = 1.0f;
	bool mbProjectionTransitioning = false;

	TSharedPtr<FConstantBuffer> mGlobalLightInfoBuffer;

	TArray<FLightInfo> mLightInfos;
	TSharedPtr<FStructuredBuffer> mLightInfoBuffer;

	TSharedPtr<FRenderPipeline> mPreDepthPipeline;

	TSharedPtr<FComputePipeline> mLightCullingPipeline;
	TSharedPtr<FStructuredBuffer> mLightCullingOutput;

	TSharedPtr<FLightShader> mMeshLightShader;
	TSharedPtr<FRenderPipeline> mMeshPipeline;

	TSharedPtr<FRenderPipeline> mHighlightMarkPipeline;
	TSharedPtr<FRenderPipeline> mHighlightDrawPipeline;
	TSharedPtr<FVertexBuffer> mHighlightVertexBuffer;
	TSharedPtr<FIndexBuffer> mHighlightIndexBuffer;

	FRenderCollector mRenderCollector;
	FRenderGraph mRenderGraph;
	FFogProcess mFogProcess;
	FFXAAProcess mFXAAProcess;
	FDepthPreviewProcess mDepthPreviewProcess;
	FNormalPreviewProcess mNormalPreviewProcess;

	int32 GridGap = 1;
	bool bGpuTimerActive = false;

	TSharedPtr<FTexture2DAsset> DefaultNormalMap = nullptr;
};
