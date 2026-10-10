#include "FEditorUIManager.h"
#include "Console.h"
#include "Renderer.h"
#include "FileManager.h"
#include "EngineStatics.h"
#include "FEditorViewportClient.h"
#include "LaunchEngineLoop.h"
#include "WindowApplication.h"
#include "GraphicsManager.h"
#include "Camera.h"
#include "FInstrumentor.h"
#include "ShowFlags.h"
#include "PrimitiveComponent.h"
#include "USpotLightComponent.h"
#include "World.h"
#include "FEngine.h"

FEditorUIManager::FEditorUIManager(URenderer& InRenderer)
	: mRenderer(InRenderer)
{
	mViewportX = 0;
	mViewportY = 0;
	mViewportWidth = static_cast<float>(WindowApplication.PendingWidth);
	mViewportHeight = static_cast<float>(WindowApplication.PendingHeight);

	mContentBrowser.Initialize(kDefaultAssetsPath);
	mContentBrowser.SetEventHandler(this);
}

void FEditorUIManager::Render(FGuiReference& GuiReference)
{
	//ImGui
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	RenderMenuBar(GuiReference);
	mMainToolbar.Render(GuiReference);
	
	const ImGuiID DockspaceID = ImGui::GetID("EditorDockSpace");

	// 저장된 도킹 노드가 없을 때만 기본 배치 생성
	if (!ImGui::DockBuilderGetNode(DockspaceID))
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;

		ImGui::DockBuilderAddNode(DockspaceID, ImGuiDockNodeFlags_DockSpace | flags);
		ImGui::DockBuilderSetNodeSize(DockspaceID, viewport->WorkSize);

		ImGuiID center = DockspaceID;
		ImGuiID left;
		ImGuiID bottom;

		// 왼쪽 패널 2%
		ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.2f, &left, &center);

		// 나머지 영역 아래쪽에 콘솔 30%
		ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.3f, &bottom, &center);

		ImGuiID leftMiddle;
		ImGuiID leftBottom;
		ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.5f, &leftMiddle, &leftBottom);

		ImGui::DockBuilderDockWindow("Viewport", center);
		ImGui::DockBuilderDockWindow("Console Window", bottom);
		ImGui::DockBuilderDockWindow("Jungle Property Window", leftMiddle);
		ImGui::DockBuilderDockWindow("Object List Panel", leftBottom);

		ImGui::DockBuilderFinish(DockspaceID);
	}

	RenderViewport(GuiReference, DockspaceID);

	{
		PROFILE_SCOPE("Frame/EditorUI/RenderBottonBar");
		RenderBottomBar();
	}

	ConsoleWindow& console = ConsoleWindow::Get();
	if (console.bShowStatFPS || console.bShowStatMemory || console.bShowStatRender)
	{
		// Viewport 창 안쪽 좌상단에 붙는 입력을 받지 않는 오버레이 창
		ImGui::SetNextWindowPos(ImVec2(mViewportX + 12.0f, mViewportY + 12.0f), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.55f);

		const ImGuiWindowFlags overlayFlags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoInputs;

		ImGui::Begin("##StatOverlay", nullptr, overlayFlags);
		if (console.bShowStatFPS)
		{
			if (console.bShowStatMemory || console.bShowStatRender)
			{
				ImGui::Separator();
			}

			ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.35f, 1.0f), "FPS");
			ImGui::Text("FPS: %.1f", GEngineLoop.GetFrameTimer()->GetFPS());
			ImGui::Text("Frame: %.2f ms", GEngineLoop.GetFrameTimer()->GetDeltaTime() * 1000.0f);
		}

		if (console.bShowStatMemory)
		{
			if (console.bShowStatFPS || console.bShowStatRender)
			{
				ImGui::Separator();
			}

			ImGui::TextColored(ImVec4(0.35f, 0.8f, 1.0f, 1.0f), "Memory");
			ImGui::Text("Total allocated memory count: %d", UEngineStatics::sTotalAllocationCount);
			ImGui::Text("Total allocated memory size: %d bytes", UEngineStatics::sTotalAllocationBytes);

			const FAssetStats Stats = GuiReference.AssetManager->GetStats();

			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Assets");
			ImGui::Text("Registered: %u", Stats.RegisteredCount);
			ImGui::Text("Loaded: %u", Stats.LoadedCount);

			ImGui::Text("Registered Static Mesh: %u", Stats.RegisteredStaticMesh);
			ImGui::Text("Loaded Static Mesh: %u", Stats.LoadedStaticMesh);
			ImGui::Text("Registered Static Texture 2D: %u", Stats.RegisteredTexture2D);
			ImGui::Text("Loaded Static Texture 2D: %u", Stats.LoadedTexture2D);
			ImGui::Text("Registered Static Material: %u", Stats.RegisteredMaterial);
			ImGui::Text("Loaded Static Material: %u", Stats.LoadedMaterial);
		}

		if (console.bShowStatRender)
		{
			if (console.bShowStatFPS || console.bShowStatMemory)
			{
				ImGui::Separator();
			}
			ImGui::TextColored(ImVec4(0.35f, 0.8f, 0.5f, 1.0f), "Render");
			ImGui::Text("Draw Calls: %u", GuiReference.GraphicsManager->GetRenderer()->GetDrawCallCount());

			ImGui::Text("GPU Render: %.3f ms", GuiReference.GraphicsManager->GetGpuRenderTime());

			UINT PrimitiveCount = 0;
			for (TObjectIterator<UPrimitiveComponent> It(true); It; ++It)
			{
				++PrimitiveCount;
			}
			ImGui::Text("Primitives: %u", PrimitiveCount);

			UINT LightCount = 0;
			UINT SpotLightCount = 0;
			UWorld* World = GuiReference.WorldContext ? GuiReference.WorldContext->World() : nullptr;
			if (World)
			{
				for (ULightComponentBase* Light : World->GetLightComponents())
				{
					++LightCount;
					if (Light->IsA<USpotLightComponent>())
					{
						++SpotLightCount;
					}
				}
			}
			ImGui::Text("Lights: %u (Spot: %u)", LightCount, SpotLightCount);
		}
		ImGui::End();
	}
	ImGui::PopStyleVar();

#if IS_OBJ_VIEWER
	ConsoleWindow::Get().Process(BottomBarHeight);
	mContentBrowser.Render(BottomBarHeight);
#else
	mPropertyWindow.Render(GuiReference);
	mOutlinerWindow.Render(GuiReference);
	ConsoleWindow::Get().Process(BottomBarHeight);
	mContentBrowser.Render(BottomBarHeight);
#endif
}

void FEditorUIManager::RenderMenuBar(FGuiReference& GuiReference)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (GuiReference.WorldContext->GetWorldType() == EWorldType::Editor)
			{
				const std::filesystem::path SceneDirectory = std::filesystem::absolute(std::filesystem::path(kDefaultAssetsPath) / std::filesystem::path(kSceneDataDir));

				if (ImGui::MenuItem("New scene"))
				{
					GEditor.EnqueuePendingTask([ViewportClient = GuiReference.ViewportClient]() {
						GEditor.CreateNewMapForEditing();
						ViewportClient->Reset();
					});
				}

				if (ImGui::MenuItem("Save scene"))
				{
					try
					{
						const std::optional<std::filesystem::path> selectedPath = FNativeFileDialog::SaveScene(SceneDirectory);

						// 취소 버튼을 누른 경우에는 아무 작업도 하지 않는다.
						if (selectedPath.has_value())
						{
							SaveMap(GuiReference.WorldContext->World(), GuiReference.EditorCamera, selectedPath.value(), *GuiReference.FileManager);
							UE_LOG("Scene saved: %s", selectedPath->string().c_str());
						}
					}
					catch (const std::exception& e)
					{
						UE_LOG_ERROR("Failed to save scene: %s", e.what());
					}
				}

				if (ImGui::MenuItem("Load scene"))
				{
					try
					{
						const std::optional<std::filesystem::path> selectedPath = FNativeFileDialog::OpenScene(SceneDirectory);

						// 취소한 경우에는 현재 씬과 카메라 상태를 건드리지 않는다.
						if (selectedPath.has_value())
						{
							GEditor.EnqueuePendingTask([PrevWorld = GuiReference.WorldContext->World(), Camera = GuiReference.EditorCamera, selectedPath, FileManager = GuiReference.FileManager]() {
								GEditor.CreateNewMapForEditing();
								FWorldContext* NewWorldContext = GEditor.GetEditorWorldContext();
								UWorld* NewWorld = NewWorldContext->World();
								LoadMap(NewWorld, Camera, selectedPath.value(), *FileManager);
								});

							// 파일 로드가 실행된 뒤에만 카메라를 초기화한다.
							GuiReference.ViewportClient->Reset();

							UE_LOG("Scene loaded: %s", selectedPath->string().c_str());
						}
					}
					catch (const std::exception& e)
					{
						UE_LOG_ERROR("Failed to load scene: %s", e.what());
					}
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void FEditorUIManager::RenderViewport(FGuiReference& GuiReference, ImGuiID DockspaceID)
{
	// Docking
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// 저장된 도킹 노드가 없을 때만 기본 배치 생성
	const ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;

	ImGui::DockSpaceOverViewport(DockspaceID, viewport, flags);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	const ImGuiWindowFlags ViewportWindowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	if (ImGui::Begin("Viewport", nullptr, ViewportWindowFlags))
	{
		PROFILE_SCOPE("Frame/EditorUI/Viewport");

		const ImVec2 Origin = ImGui::GetCursorScreenPos();
		const ImVec2 TotalSize = ImGui::GetContentRegionAvail();

		mViewportX = Origin.x;
		mViewportY = Origin.y;
		mViewportWidth = TotalSize.x;
		mViewportHeight = TotalSize.y;

		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		ImGuiIO& IO = ImGui::GetIO();

		for (int32 i = 0; i < GuiReference.ViewportCount; ++i)
		{
			const int32 CurrentViewportIndex = GuiReference.EditorLayout->bIsSplitView ? i : GuiReference.EditorLayout->MaximizedViewportIndex;

			FEditorViewport* EditorViewport = &GuiReference.Viewports[i];

			FRect DrawRect = EditorViewport->Window->Rect;
			if (DrawRect.Width <= 0 || DrawRect.Height <= 0)
			{
				continue;
			}

			// NOTE: Scene 렌더링
			ImGui::SetCursorScreenPos(ImVec2(DrawRect.X, DrawRect.Y));

			bool bHovered = ImGui::IsMouseHoveringRect(ImVec2(DrawRect.X, DrawRect.Y), ImVec2(DrawRect.X + DrawRect.Width, DrawRect.Y + DrawRect.Height)) && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId); // 팝업창, 콤보 드롭다운 등 열리면 false
			const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
			const float Bottom = MainViewport->WorkPos.y + MainViewport->WorkSize.y;
			float BlockedTop = Bottom - BottomBarHeight;

			if (mContentBrowser.IsDrawerOpen())
			{
				BlockedTop -= mContentBrowser.GetDrawerHeight();
			}

			if (ConsoleWindow::Get().bIsDrawerOpen)
			{
				BlockedTop = (std::min)(BlockedTop, Bottom - BottomBarHeight - ConsoleWindow::Get().GetDrawerHeight());
			}

			bHovered = bHovered && IO.MousePos.y < BlockedTop;
			EditorViewport->Client->SetActive(bHovered);

			FRenderTarget2D* RenderTarget = EditorViewport->Viewport->GetFrontRenderTarget();
			DrawList->AddImage((ImTextureID)(intptr_t)RenderTarget->SRV.Get(), ImVec2(DrawRect.X, DrawRect.Y), ImVec2(DrawRect.X + DrawRect.Width, DrawRect.Y + DrawRect.Height));

			// NOTE: Viewport 툴바 배경 그리기
			ImGui::PushID(CurrentViewportIndex);

			const float ViewportTypeWidth = 95.0f;
			const float CameraButtonWidth = ImGui::CalcTextSize("Camera").x + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f;
			const float ViewModeWidth = 85.0f;
			const float MaximizeButtonWidth = 28.0f;
			const float SplitButtonWidth = 28.0f;

			const float Spacing = ImGui::GetStyle().ItemSpacing.x;
			const float MarginX = 8.0f;
			const float MarginY = 4.0f;
			const float ToolBarHeight = 28.0f;

			const float ToolBarWidth = CameraButtonWidth + ViewportTypeWidth + ViewModeWidth + MaximizeButtonWidth + SplitButtonWidth + (Spacing * 4.0f) + MarginX;
			const float StartCursorPos = DrawRect.X + DrawRect.Width - ToolBarWidth;
			const float ItemHeight = 22.0f;

			DrawList->AddRectFilled(ImVec2(DrawRect.X, DrawRect.Y), ImVec2(DrawRect.X + DrawRect.Width, DrawRect.Y + ToolBarHeight), IM_COL32(30, 30, 30, 180));

			ImGui::SetCursorScreenPos(ImVec2(StartCursorPos, DrawRect.Y + MarginY));

			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(15, 15, 15, 230));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(45, 45, 45, 240));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(60, 60, 60, 255));

			ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(20, 20, 20, 250));
			ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(50, 50, 50, 255));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(75, 75, 75, 255));

			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(15, 15, 15, 230));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(55, 55, 55, 240));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(80, 80, 80, 255));

			// NOTE: Gizmo 버튼 그리기
			EGIZMO_TYPE CurrentGizmoType = GuiReference.ViewportClient->mGizmo.GetOperation();
			int32 CurrentGizmoIndex = static_cast<int32>(CurrentGizmoType);

			const char* GizmoButtonIDs[] = { "##GizmoTranslate", "##GizmoRotate", "##GizmoScale" };
			const char* GizmoTooltips[] = { "Translate", "Rotate", "Scale" };
			const float GizmoButtonWidth = 28.0f;
			const ImU32 GizmoIconColor = IM_COL32(220, 220, 220, 255);
			for (int32 GizmoIndex = 0; GizmoIndex < 3; ++GizmoIndex)
			{
				ImGui::SetCursorScreenPos(ImVec2(DrawRect.X + MarginX + GizmoIndex * (GizmoButtonWidth + 4.0f), DrawRect.Y + MarginY));

				if (ImGui::Button(GizmoButtonIDs[GizmoIndex], ImVec2(GizmoButtonWidth, ItemHeight)))
				{
					GuiReference.ViewportClient->mGizmo.SetOperation(static_cast<EGIZMO_TYPE>(GizmoIndex));
				}

				const ImVec2 ButtonMin = ImGui::GetItemRectMin();
				const ImVec2 Center(ButtonMin.x + GizmoButtonWidth * 0.5f, ButtonMin.y + ItemHeight * 0.5f);
				if (GizmoIndex == 0)
				{
					DrawList->AddLine(ImVec2(Center.x - 6, Center.y), ImVec2(Center.x + 6, Center.y), GizmoIconColor, 1.5f);
					DrawList->AddLine(ImVec2(Center.x, Center.y - 6), ImVec2(Center.x, Center.y + 6), GizmoIconColor, 1.5f);
					DrawList->AddTriangleFilled(ImVec2(Center.x - 8, Center.y), ImVec2(Center.x - 4, Center.y - 3), ImVec2(Center.x - 4, Center.y + 3), GizmoIconColor);
					DrawList->AddTriangleFilled(ImVec2(Center.x + 8, Center.y), ImVec2(Center.x + 4, Center.y - 3), ImVec2(Center.x + 4, Center.y + 3), GizmoIconColor);
					DrawList->AddTriangleFilled(ImVec2(Center.x, Center.y - 8), ImVec2(Center.x - 3, Center.y - 4), ImVec2(Center.x + 3, Center.y - 4), GizmoIconColor);
					DrawList->AddTriangleFilled(ImVec2(Center.x, Center.y + 8), ImVec2(Center.x - 3, Center.y + 4), ImVec2(Center.x + 3, Center.y + 4), GizmoIconColor);
				}
				else if (GizmoIndex == 1)
				{
					DrawList->PathArcTo(Center, 6.0f, 0.0f, 4.71f, 20);
					DrawList->PathStroke(GizmoIconColor, 1.5f);
					DrawList->AddTriangleFilled(ImVec2(Center.x + 4, Center.y - 6), ImVec2(Center.x - 1, Center.y - 9), ImVec2(Center.x - 1, Center.y - 3), GizmoIconColor);
				}
				else
				{
					DrawList->AddRect(ImVec2(Center.x - 7, Center.y + 2), ImVec2(Center.x - 2, Center.y + 7), GizmoIconColor, 0.0f, 0, 1.5f);
					DrawList->AddLine(ImVec2(Center.x - 2, Center.y + 2), ImVec2(Center.x + 6, Center.y - 6), GizmoIconColor, 1.5f);
					DrawList->AddLine(ImVec2(Center.x + 1, Center.y - 6), ImVec2(Center.x + 6, Center.y - 6), GizmoIconColor, 1.5f);
					DrawList->AddLine(ImVec2(Center.x + 6, Center.y - 6), ImVec2(Center.x + 6, Center.y - 1), GizmoIconColor, 1.5f);
				}

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", GizmoTooltips[GizmoIndex]);
				}

				if (CurrentGizmoIndex == GizmoIndex)
				{
					DrawList->AddRect(
						ImVec2(DrawRect.X + MarginX + GizmoIndex * (GizmoButtonWidth + 4.0f), DrawRect.Y + MarginY),
						ImVec2(DrawRect.X + MarginX + GizmoIndex * (GizmoButtonWidth + 4.0f) + GizmoButtonWidth, DrawRect.Y + MarginY + ItemHeight),
						IM_COL32(100, 100, 255, 255), 0.0f, 0, 2.0f
					);
				}
			}

			// NOTE: Grid Gap Combo Box
			ImGui::SetCursorScreenPos(ImVec2(DrawRect.X + MarginX + 3.0f * (GizmoButtonWidth + 4.0f) + 4.0f, DrawRect.Y + MarginY));
			static constexpr int32 GridGapValues[] = { 1, 5, 10, 50, 100, 500 };
			const int32 GridGap = GuiReference.GraphicsManager->GetGridGap();
			const std::string GridPreview = std::format("Grid {}", GridGap);
			ImGui::SetNextItemWidth(ImGui::CalcTextSize("Grid 500").x + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f);
			if (ImGui::BeginCombo("##ViewportGridGap", GridPreview.c_str()))
			{
				for (const int32 Gap : GridGapValues)
				{
					const bool bSelected = Gap == GridGap;
					if (ImGui::Selectable(std::to_string(Gap).c_str(), bSelected))
					{
						GuiReference.GraphicsManager->SetGridGap(Gap);
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Grid spacing");
			}

			// NOTE: Camera 버튼 그리기
			ImGui::SetCursorScreenPos(ImVec2(StartCursorPos, DrawRect.Y + MarginY));
			if (ImGui::Button("Camera##ViewportCamera", ImVec2(CameraButtonWidth, ItemHeight)))
			{
				ImGui::OpenPopup("##ViewportCameraSettings");
			}

			const ImVec2 CameraButtonMax = ImGui::GetItemRectMax();
			const float CameraArrowY = ImGui::GetItemRectMin().y + ItemHeight * 0.5f;
			DrawList->AddTriangleFilled(ImVec2(CameraButtonMax.x - 10.0f, CameraArrowY - 2.0f), ImVec2(CameraButtonMax.x - 4.0f, CameraArrowY - 2.0f), ImVec2(CameraButtonMax.x - 7.0f, CameraArrowY + 2.0f), IM_COL32(220, 220, 220, 255));
			ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 28.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
			
			if (ImGui::BeginPopup("##ViewportCameraSettings"))
			{
				RenderCameraControl(EditorViewport->Client->GetCamera());
				ImGui::EndPopup();
			}

			ImGui::PopStyleVar();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);

			// NOTE: Viewport Type & View Mode Combo Box
			const char* ViewportTypeNames[] = { "Perspective", "Top", "Front", "Side" };
			int32 CurrentTypeIndex = static_cast<int32>(EditorViewport->Client->GetViewportType());

			if (ImGui::Combo("##ViewportType", &CurrentTypeIndex, ViewportTypeNames, IM_ARRAYSIZE(ViewportTypeNames)))
			{
				EditorViewport->Client->SetViewportType(static_cast<EViewportType>(CurrentTypeIndex));
			}

			ImGui::SameLine();

			ImGui::SetNextItemWidth(80.0f);

			const char* ViewModeNames[] = { "Lit", "UnLit", "Wireframe", "SceneDepth", "WorldNormal" };
			int32 CurrentModeIndex = static_cast<int32>(EditorViewport->Client->GetViewMode());

			if (ImGui::Combo("##ViewMode", &CurrentModeIndex, ViewModeNames, IM_ARRAYSIZE(ViewModeNames)))
			{
				EditorViewport->Client->SetViewMode(static_cast<EViewModeIndex>(CurrentModeIndex));
			}

			ImGui::SameLine();

			if (ImGui::Button("##Maximize", ImVec2(MaximizeButtonWidth, ItemHeight)))
			{
				GuiReference.EditorLayout->MaximizedViewportIndex = CurrentViewportIndex;
				GuiReference.EditorLayout->bIsSplitView = false;
			}
			FEditorIconUtils::DrawMaximizeButtonIcon(DrawList);

			ImGui::SameLine();

			if (ImGui::Button("##Split", ImVec2(MaximizeButtonWidth, ItemHeight)))
			{
				GuiReference.EditorLayout->bIsSplitView = true;
			}
			FEditorIconUtils::DrawSplitButtonIcon(DrawList);

			ImGui::PopStyleColor(10);
			ImGui::PopID();
			ImGui::Dummy(ImVec2(DrawRect.Width, DrawRect.Height));
		}

		// NOTE: Spliter 그리기
		if (GuiReference.EditorLayout->bIsSplitView)
		{
			float HorizontalRatio, VerticalRatio;
			GuiReference.EditorLayout->GetSplitRatios(HorizontalRatio, VerticalRatio);

			const float SplitThickness = 1.0f;
			const float SplitHandleThickness = 8.0f;

			float SplitX = mViewportX + mViewportWidth * VerticalRatio;
			float SplitY = mViewportY + mViewportHeight * HorizontalRatio;

			bool bHoverSplitLine = false;

			ImGui::SetCursorScreenPos(ImVec2(SplitX - SplitHandleThickness * 0.5f, mViewportY));
			ImGui::InvisibleButton("##SplitVertical", ImVec2(SplitHandleThickness, mViewportHeight));
			if (ImGui::IsItemActive())
			{
				VerticalRatio = (IO.MousePos.x - mViewportX) / mViewportWidth;
				VerticalRatio = std::clamp(VerticalRatio, 0.1f, 0.9f);
			}
			if (ImGui::IsItemHovered())
			{
				bHoverSplitLine = true;
			}

			ImGui::SetCursorScreenPos(ImVec2(mViewportX, SplitY - SplitHandleThickness * 0.5f));
			ImGui::InvisibleButton("##SplitHorizontal", ImVec2(mViewportWidth, SplitHandleThickness));
			if (ImGui::IsItemActive())
			{
				HorizontalRatio = (IO.MousePos.y - mViewportY) / mViewportHeight;
				HorizontalRatio = std::clamp(HorizontalRatio, 0.1f, 0.9f);
			}
			if (ImGui::IsItemHovered())
			{
				bHoverSplitLine = true;
			}

			SplitX = mViewportX + mViewportWidth * VerticalRatio;
			SplitY = mViewportY + mViewportHeight * HorizontalRatio;

			DrawList->AddLine(ImVec2(SplitX, mViewportY), ImVec2(SplitX, mViewportY + mViewportHeight), ImColor(0.8f, 0.8f, 0.8f, 1.0f), SplitThickness);
			DrawList->AddLine(ImVec2(mViewportX, SplitY), ImVec2(mViewportX + mViewportWidth, SplitY), ImColor(0.8f, 0.8f, 0.8f, 1.0f), SplitThickness);

			GuiReference.EditorLayout->SetSplitRatios(HorizontalRatio, VerticalRatio);

			if (bHoverSplitLine)
			{
				GEditor.SetMouseCursor(ImGuiMouseCursor_ResizeAll);
			}
		}
	}
	ImGui::End();
}

void FEditorUIManager::RenderBottomBar()
{
	const ImGuiViewport* Viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(ImVec2(Viewport->WorkPos.x, Viewport->WorkPos.y + Viewport->WorkSize.y - BottomBarHeight));
	ImGui::SetNextWindowSize(ImVec2(Viewport->WorkSize.x, BottomBarHeight));
	ImGui::SetNextWindowViewport(Viewport->ID);

	const ImGuiWindowFlags BottomBarFlags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoDocking;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 3.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(24, 24, 24, 255));

	if (ImGui::Begin("##EditorBottomBar", nullptr, BottomBarFlags))
	{
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(45, 45, 48, 255));

		if (ImGui::Button("[ Content Drawer] (Ctrl+Space)"))
		{
			ConsoleWindow::Get().SetIsDrawerOpen(false);
			mContentBrowser.ToggleDrawer();
		}

		ImGui::SameLine();

		if (ImGui::Button("[ Console ]"))
		{
			ConsoleWindow::Get().ToggleDrawer();
			mContentBrowser.SetIsDrawerOpen(false);
		}

		ImGui::PopStyleColor();
	}

	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void FEditorUIManager::OnNewAssetFile(const FAssetFileHeader& Header, const std::filesystem::path& FilePath)
{
	FAssetManager& AssetManager = FAssetManager::Get();

	if (Header.AssetType == EAssetType::Texture2D ||
		Header.AssetType == EAssetType::Material ||
		Header.AssetType == EAssetType::StaticMesh)
	{

		FAssetManager::Get().ScanDirectory("Assets", mRenderer);

		// 현재 자신이 있는 폴더만 refresh 하는 문제가 있어서 위와 같이 수정함.
		// (mesh면 mesh 폴더만 refresh. texture 폴더는 안 하는 문제)
		// guid 검사하여 이미 있는 건 Register pass 하기 때문에 등록 비용 거의 없음.
#if 0
		FAssetManager::Get().ScanDirectory(
			FilePath.parent_path(),
			mRenderer);
#endif
	}
	else
	{
		UE_LOG_ERROR("Unsupported asset type");
	}
}

void FEditorUIManager::OnDeleteAssetFile(const std::filesystem::path& FilePath)
{
	FAssetManager& AssetManager = FAssetManager::Get();

	std::string CanonicalPath = std::filesystem::weakly_canonical(FilePath).string();
	AssetManager.UnregisterAsset(FName(CanonicalPath.c_str()));
}

void FEditorUIManager::RefreshContentBrowser(const std::filesystem::path& TargetDirectory)
{
    std::error_code Error;
	if (!std::filesystem::is_directory(TargetDirectory, Error))
	{
        return;
	}

    FAssetManager& AssetManager = FAssetManager::Get();
    AssetManager.PurgeStaleAssetsInDirectory(TargetDirectory);
    AssetManager.ScanDirectory(TargetDirectory, mRenderer);
    mContentBrowser.RefreshCache();
}

void FEditorUIManager::RenderCameraControl(FCamera& camera)
{
	ImGui::Text("FOV");
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::SliderFloat("##FOV", &camera.mFovDegree, 0.0f, 180.0f);

	ImGui::Text("Sensitivity");
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::SliderFloat("##CameraSensitivity", &camera.Sensitivity, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

	ImGui::Text("Location (X / Y / Z)");

	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float itemWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f;

	FVector Location = camera.Transform.GetLocation();
	bool bLocationChanged = false;

	ImGui::SetNextItemWidth(itemWidth);
	bLocationChanged |= ImGui::DragFloat("##CamLocX", &Location.x, 0.1f, 10.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	bLocationChanged |= ImGui::DragFloat("##CamLocY", &Location.y, 0.1f, 10.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	bLocationChanged |= ImGui::DragFloat("##CamLocZ", &Location.z, 0.1f, 10.0f);

	if (bLocationChanged)
	{
		camera.Transform.SetLocation(Location);
	}

	FQuaternion Rotation = camera.Transform.GetRotation();
	FRotator Rotator = ToEulerAngles(Rotation);
	bool bRotationChanged = false;

	ImGui::Text("Rotation (Roll / Pitch / Yaw)");
	ImGui::SetNextItemWidth(itemWidth);
	bRotationChanged |= ImGui::DragFloat("##CamRotX", &Rotator.Roll, 0.1f, 180.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	bRotationChanged |= ImGui::DragFloat("##CamRotY", &Rotator.Pitch, 0.1f, 180.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	bRotationChanged |= ImGui::DragFloat("##CamRotZ", &Rotator.Yaw, 0.1f, 180.0f);

	if (bRotationChanged)
	{
		camera.Transform.SetRotation(ToQuaternion(Rotator));
	}
}
