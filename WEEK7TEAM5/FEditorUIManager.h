#pragma once

#include "Core.h"
#include "FMainToolbar.h"
#include "FOutlinerWindow.h"
#include "FContentBrowser.h"
#include "FPropertyWindow.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "Assets.h"
#include <filesystem>

class FCamera;
class FGraphicsManager;
class URenderer;
class FEditorEngine;
class FFileManager;
struct FWorldContext;
struct FEditorViewportClient;
struct FEditorLayout;
struct FEditorViewport;
struct FShader;

struct FGuiReference
{
	FCamera* EditorCamera;
	FWorldContext* WorldContext;
	FEditorEngine* SceneManager;
	FGraphicsManager* GraphicsManager;
	FEditorViewportClient* ViewportClient;
	FFileManager* FileManager;
	FAssetManager* AssetManager;
	FEditorLayout* EditorLayout;
	FEditorViewport* Viewports;
	int32 ViewportCount = 0;
};

class FEditorUIManager : public FContentBrowserEventHandler
{
public:
	FEditorUIManager(URenderer& InRenderer);

	void Render(FGuiReference& GuiReference);

	inline float GetViewportX() const { return mViewportX; }
	inline float GetViewportY() const { return mViewportY; }
	inline float GetViewportWidth() const { return mViewportWidth; }
	inline float GetViewportHeight() const { return mViewportHeight; }

private:
	void RenderMenuBar(FGuiReference& GuiReference);
	void RenderViewport(FGuiReference& GuiReference, ImGuiID DockspaceID);
	void RenderBottomBar();
	void RenderCameraControl(FCamera& Camera);

	void OnNewAssetFile(const FAssetFileHeader& Header, const std::filesystem::path& FilePath) override;
	void OnDeleteAssetFile(const std::filesystem::path& FilePath) override;
	void RefreshContentBrowser(const std::filesystem::path& TargetDirectory) override;

private:
	static constexpr float BottomBarHeight = 30.0f;
	
	struct FDepthPreviewData
	{
		ID3D11DeviceContext* Context;
		ID3D11PixelShader* Shader;
	};

	URenderer& mRenderer;

	FMainToolbar mMainToolbar;
	FContentBrowser mContentBrowser;
	FPropertyWindow mPropertyWindow;
	FOutlinerWindow mOutlinerWindow;

	float mViewportX;
	float mViewportY;
	float mViewportWidth;
	float mViewportHeight;
};
