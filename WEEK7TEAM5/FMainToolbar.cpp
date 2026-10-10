#include "FMainToolbar.h"
#include "FEditorEngine.h"
#include "FEditorUIManager.h"
#include "UTextComponent.h"
#include "UAtlasAnimationComponent.h"
#include "UStaticMeshComponent.h"
#include "UPointLightComponent.h"
#include "UDirectionalLightComponent.h"
#include "USpotLightComponent.h"
#include "ULightComponentBase.h"
#include "ASpotLight.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "URotationMovementComponent.h"
#include "UProjectileMovementComponent.h"
#include "AHeightFog.h"

void FMainToolbar::Render(const FGuiReference& GuiReference)
{
	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	const float Height = ImGui::GetFrameHeight() + 16.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

	const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	if (ImGui::BeginViewportSideBar("##MainToolbar", Viewport, ImGuiDir_Up, Height, Flags))
	{
		RenderShowFlagsControl(GuiReference);

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		RenderSpawnActorControl(GuiReference);

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		RenderPlayControl(GuiReference);
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void FMainToolbar::RenderShowFlagsControl(const FGuiReference& GuiReference)
{
	const float ComboWidth = ImGui::CalcTextSize("Show Flags").x + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f;
	ImGui::SetNextItemWidth(ComboWidth);
	if (ImGui::BeginCombo("##ShowFlags", "Show Flags"))
	{
		// 표시 옵션은 표를 그대로 훑어 체크박스를 만든다.
		// 옵션을 추가할 때 ShowFlags.h의 GShowFlagInfos에만 한 줄 적으면 여기 바로 나온다.
		FShowFlags& showFlags = FShowFlags::Get();
		for (const FShowFlagInfo& flagInfo : GShowFlagInfos)
		{
			bool bEnabled = showFlags.IsEnabled(flagInfo.Flag);
			if (ImGui::Checkbox(flagInfo.Name, &bEnabled))
			{
				showFlags.SetEnabled(flagInfo.Flag, bEnabled);
			}
		}

		bool bOrthographic = GuiReference.GraphicsManager->IsOrthographicTarget();
		if (ImGui::Checkbox("Orthogonal", &bOrthographic))
		{
			// Preserve the camera and ortho zoom; animate only the projection ratio.
			GuiReference.GraphicsManager->StartProjectionTransition(bOrthographic);
		}

		ImGui::EndCombo();
	}
}

void FMainToolbar::RenderSpawnActorControl(const FGuiReference& GuiReference)
{
	const char* ActorTypeNames[] = {
		"Sphere",
		"Cube",
		"Triangle",
		"GizmoArrow",
		"Circle",
		"SpotLight",
		"Explosion",
		"HeightFog",
		"FireBall",
		"Text",
	};

	const float FontSize = ImGui::GetFontSize();
	const float ControlHeight = ImGui::GetFrameHeight();
	float ActorTypeWidth = 0.0f;
	for (const char* ActorTypeName : ActorTypeNames)
	{
		ActorTypeWidth = FGenericPlatformMath::Max(ActorTypeWidth, ImGui::CalcTextSize(ActorTypeName).x);
	}
	ActorTypeWidth += ControlHeight + ImGui::GetStyle().FramePadding.x * 2.0f;

	ImGui::SetNextItemWidth(ActorTypeWidth);
	ImGui::Combo("##ToolbarActorType", &mSelectedTargetSpawnIndex, ActorTypeNames, IM_ARRAYSIZE(ActorTypeNames));
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Actor type");
	}

	ImGui::SameLine();

	if (ImGui::Button("Spawn", ImVec2(FontSize * 4.5f, ControlHeight)))
	{
		for (int32 i = 0; i < mSpawnCount; ++i)
		{
			const char* ActorTypeName = ActorTypeNames[mSelectedTargetSpawnIndex];

			AActor* NewActor = nullptr;
			if (strcmp(ActorTypeName, "Explosion") == 0)
			{
				TSharedPtr<FSpriteAtlasAsset> ExplosionAtlas = FAssetManager::Get().GetAssetAs<FSpriteAtlasAsset>(FName("ExplosionSpriteAtlas"));

				NewActor = FObjectFactory::ConstructObject<AActor>();
				NewActor->Rename(FName("ExplosionActor"));

				UAtlasAnimationComponent* AnimComponent = NewActor->CreateDefaultSubobject<UAtlasAnimationComponent>(FName("AtlasAnimationComponent"));
				AnimComponent->SetAtlas(ExplosionAtlas);
				AnimComponent->SetDepthState(true, false);
				AnimComponent->Play();

				NewActor->SetRootComponent(AnimComponent);
			}
			else if (strcmp(ActorTypeName, "Sphere") == 0 || strcmp(ActorTypeName, "Cube") == 0 || strcmp(ActorTypeName, "Triangle") == 0 || strcmp(ActorTypeName, "GizmoArrow") == 0 || strcmp(ActorTypeName, "Circle") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();
				NewActor->Rename(FName(std::format("{}Actor", ActorTypeName)));

				UStaticMeshComponent* MeshComponent = NewActor->CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComponent"));
				MeshComponent->SetMesh(FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(FName(std::format("{}Mesh", ActorTypeName)), true));
				NewActor->SetRootComponent(MeshComponent);
			}
			else if (strcmp(ActorTypeName, "SpotLight") == 0)
			{
				NewActor = FObjectFactory::ConstructUnInitializedObject<ASpotLight>();
				NewActor->Rename(FName("SpotLightActor"));
			}
			else if (strcmp(ActorTypeName, "StaticMesh") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();
				NewActor->Rename(FName("StaticMeshActor"));
				UStaticMeshComponent* MeshComponent = NewActor->CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComponent"));
				NewActor->SetRootComponent(MeshComponent);
			}
			else if (strcmp(ActorTypeName, "HeightFog") == 0)
			{
				NewActor = FObjectFactory::ConstructUnInitializedObject<AHeightFog>();
				NewActor->Rename(FName("HeightFogActor"));
			}
			else if (strcmp(ActorTypeName, "FireBall") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();
				NewActor->Rename(FName("FireBallActor"));

				UStaticMeshComponent* MeshComponent = NewActor->CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComponent"));
				MeshComponent->SetMesh(FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(BuiltInAssetID::SphereMesh, true));
				NewActor->SetRootComponent(MeshComponent);

				UPointLightComponent* PointLightComponent = NewActor->CreateDefaultSubobject<UPointLightComponent>(FName("PointLightComponent"));
				PointLightComponent->SetupAttachment(MeshComponent);
				NewActor->AddOwnedComponent(PointLightComponent);

				UProjectileMovementComponent* ProjectileMovementComponent = NewActor->CreateDefaultSubobject<UProjectileMovementComponent>(FName("ProjectileMovementComponent"));
				NewActor->AddOwnedComponent(ProjectileMovementComponent);

				URotationMovementComponent* RotationMovementComponent = NewActor->CreateDefaultSubobject<URotationMovementComponent>(FName("RotationMovementComponent"));
				NewActor->AddOwnedComponent(RotationMovementComponent);
			}
			else if (strcmp(ActorTypeName, "Text") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();
				NewActor->Rename(FName("TextActor"));

				UTextRenderComponent* TextComponent = NewActor->CreateDefaultSubobject<UTextRenderComponent>(FName("TextRenderComponent"));
				TextComponent->SetText(L"Hello World!");
				TextComponent->SetFontAtlasAsset(FAssetManager::Get().GetAssetAs<FFontAtlasAsset>(FName("TestFontAtlas")));

				NewActor->SetRootComponent(TextComponent);
			}
			else
			{
				UE_LOG_ERROR("Unknown actor class: %s", ActorTypeName);
			}

			if (NewActor)
			{
				GuiReference.WorldContext->World()->GetLevel()->AddActor(NewActor);
			}
		}
	}

	ImGui::SameLine();

	ImGui::SetNextItemWidth(FontSize * 3.5f + ControlHeight * 2.0f + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f);
	if (ImGui::InputInt("##ToolbarSpawnCount", &mSpawnCount))
	{
		mSpawnCount = FGenericPlatformMath::Max(1, mSpawnCount);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Number of actors to spawn");
	}
}

void FMainToolbar::RenderPlayControl(const FGuiReference& GuiReference)
{
	const bool bPlaying = GEditor.IsPlayingInEditor();
	const ImVec4 Accent = bPlaying ? ImVec4(0.96f, 0.34f, 0.34f, 1.0f) : ImVec4(0.35f, 0.88f, 0.51f, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.28f, 0.28f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.32f, 0.32f, 0.32f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, Accent);
	const bool bClicked = ImGui::Button(bPlaying ? "##Stop" : "##Play", ImVec2(32.0f, ImGui::GetFrameHeight()));
	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar(2);

	const ImVec2 ButtonMin = ImGui::GetItemRectMin();
	const ImVec2 ButtonMax = ImGui::GetItemRectMax();
	const ImVec2 IconCenter((ButtonMin.x + ButtonMax.x) * 0.5f, (ButtonMin.y + ButtonMax.y) * 0.5f);
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	const ImU32 IconColor = ImGui::ColorConvertFloat4ToU32(Accent);
	if (bPlaying)
	{
		DrawList->AddRectFilled(ImVec2(IconCenter.x - 4.5f, IconCenter.y - 4.5f), ImVec2(IconCenter.x + 4.5f, IconCenter.y + 4.5f), IconColor, 1.0f);
	}
	else
	{
		DrawList->AddTriangleFilled(ImVec2(IconCenter.x - 4.0f, IconCenter.y - 6.0f), ImVec2(IconCenter.x - 4.0f, IconCenter.y + 6.0f), ImVec2(IconCenter.x + 6.0f, IconCenter.y), IconColor);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(bPlaying ? "Stop play session" : "Play in editor");
	}

	if (bPlaying)
	{
		if (bClicked)
		{
			GEditor.EnqueuePendingTask([ViewportClient = GuiReference.ViewportClient]() {
				GEditor.EndPIE();
				GEditor.ResetSelectedComponent();
				ViewportClient->Reset();
			});
		}
	}
	else
	{
		if (bClicked)
		{
			GEditor.EnqueuePendingTask([ViewportClient = GuiReference.ViewportClient]() {
				GEditor.StartPIE();
				GEditor.ResetSelectedComponent();
				ViewportClient->Reset();
			});
		}
	}
}
