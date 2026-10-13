#pragma once

#include "ImGui/imgui.h"
#include "FAssetManager.h"
#include <format>

class FEditorUtils
{
public:
	static void DrawObjIcon(const ImVec2 PMin, const ImVec2 PMax, ImDrawList* DrawList, const bool bHovered, const bool bActive)
	{
		ImVec2 Center = ImVec2((PMin.x + PMax.x) * 0.5f, (PMin.y + PMax.y) * 0.5f - 4.0f);
		const float HalfSize = 16.0f;
		const float Offset = 8.0f;

		ImVec2 F_TL = ImVec2(Center.x - HalfSize, Center.y - HalfSize + Offset * 0.5f);
		ImVec2 F_BR = ImVec2(Center.x + HalfSize - Offset, Center.y + HalfSize);
		ImVec2 B_TL = ImVec2(F_TL.x + Offset, F_TL.y - Offset);
		ImVec2 B_BR = ImVec2(F_BR.x + Offset, F_BR.y - Offset);

		ImU32 CubeColor = bActive ? 0xFF00FFFF : (bHovered ? 0xFF66FFFF : 0xFF00D2D2);
		ImU32 FillColor = (CubeColor & 0x00FFFFFF) | 0x22000000;

		DrawList->AddRectFilled(F_TL, F_BR, FillColor);
		DrawList->AddRect(F_TL, F_BR, CubeColor, 0.0f, 0, 1.5f);
		DrawList->AddRect(B_TL, B_BR, CubeColor, 0.0f, 0, 1.5f);

		DrawList->AddLine(F_TL, B_TL, CubeColor, 1.5f);
		DrawList->AddLine(ImVec2(F_BR.x, F_TL.y), ImVec2(B_BR.x, B_TL.y), CubeColor, 1.5f);
		DrawList->AddLine(ImVec2(F_TL.x, F_BR.y), ImVec2(B_TL.x, B_BR.y), CubeColor, 1.5f);
		DrawList->AddLine(F_BR, B_BR, CubeColor, 1.5f);
	}

	static void DrawFolderIcon(const ImVec2 PMin, const ImVec2 PMax, ImDrawList* DrawList, const bool bHovered, const bool bActive)
	{
		ImU32 TabColor = bActive ? 0xFF80C0E0 : (bHovered ? 0xFF99D0F0 : 0xFF70B0D0);
		ImU32 BodyColor = bActive ? 0xFF90D0F8 : (bHovered ? 0xFFAAE0FF : 0xFF80C0E8);

		ImVec2 TabMin = ImVec2(PMin.x + 8.0f, PMin.y + 12.0f);
		ImVec2 TabMax = ImVec2(PMin.x + 36.0f, PMin.y + 24.0f);
		DrawList->AddRectFilled(TabMin, TabMax, TabColor, 4.0f);

		ImVec2 BodyMin = ImVec2(PMin.x + 8.0f, PMin.y + 20.0f);
		ImVec2 BodyMax = ImVec2(PMax.x - 8.0f, PMax.y - 12.0f);
		DrawList->AddRectFilled(BodyMin, BodyMax, BodyColor, 6.0f);
		DrawList->AddRect(BodyMin, BodyMax, 0x55000000, 6.0f, 0, 1.5f);
	}

	static void DrawMaterialIcon(const ImVec2 PMin, const ImVec2 PMax, ImDrawList* DrawList, const bool bHovered, const bool bActive)
	{
		ImVec2 Center = ImVec2((PMin.x + PMax.x) * 0.5f, (PMin.y + PMax.y) * 0.5f - 4.0f);
		DrawList->AddCircleFilled(Center, 18.0f, 0xFF44AA44);
		DrawList->AddCircle(Center, 18.0f, 0xFF88FF88, 0, 2.0f);
	}

	static void DrawDocumentIcon(const ImVec2 PMin, const ImVec2 PMax, ImDrawList* DrawList)
	{
		ImVec2 Center = ImVec2((PMin.x + PMax.x) * 0.5f, (PMin.y + PMax.y) * 0.5f - 4.0f);
		const float HalfW = 14.0f, HalfH = 18.0f, Fold = 8.0f;
		ImVec2 DocTL = ImVec2(Center.x - HalfW, Center.y - HalfH);
		ImVec2 DocBR = ImVec2(Center.x + HalfW, Center.y + HalfH);

		DrawList->AddRectFilled(DocTL, DocBR, 0xFFE0E0E0, 2.0f);
		DrawList->AddRect(DocTL, DocBR, 0x88000000, 2.0f, 0, 1.2f);

		ImVec2 FoldA = ImVec2(DocBR.x - Fold, DocTL.y);
		ImVec2 FoldB = ImVec2(DocBR.x, DocTL.y + Fold);
		ImVec2 FoldC = ImVec2(DocBR.x - Fold, DocTL.y + Fold);
		DrawList->AddTriangleFilled(FoldA, FoldB, FoldC, 0xFFB0B0B0);
		DrawList->AddTriangle(FoldA, FoldB, FoldC, 0x88000000, 1.0f);
	}

	static void DrawMaximizeButtonIcon(ImDrawList* DrawList)
	{
		const ImU32 IconLineColor = IM_COL32(230, 230, 230, 255);
		const float LineThickness = 1.5f;

		ImVec2 bMin = ImGui::GetItemRectMin();
		ImVec2 bMax = ImGui::GetItemRectMax();

		ImVec2 iconMin = ImVec2(bMin.x + 5.0f, bMin.y + 4.0f);
		ImVec2 iconMax = ImVec2(bMax.x - 5.0f, bMax.y - 4.0f);

		DrawList->AddRect(iconMin, iconMax, IconLineColor, 0.0f, 0, LineThickness);
	}

	static void DrawSplitButtonIcon(ImDrawList* DrawList)
	{
		const ImU32 IconLineColor = IM_COL32(230, 230, 230, 255);
		const float LineThickness = 1.5f;

		ImVec2 bMin = ImGui::GetItemRectMin();
		ImVec2 bMax = ImGui::GetItemRectMax();

		ImVec2 iconMin = ImVec2(bMin.x + 5.0f, bMin.y + 4.0f);
		ImVec2 iconMax = ImVec2(bMax.x - 5.0f, bMax.y - 4.0f);

		DrawList->AddRect(iconMin, iconMax, IconLineColor, 0.0f, 0, LineThickness);

		float midX = (iconMin.x + iconMax.x) * 0.5f;
		float midY = (iconMin.y + iconMax.y) * 0.5f;

		DrawList->AddLine(ImVec2(midX, iconMin.y), ImVec2(midX, iconMax.y), IconLineColor, LineThickness);
		DrawList->AddLine(ImVec2(iconMin.x, midY), ImVec2(iconMax.x, midY), IconLineColor, LineThickness);
	}

	static bool DrawAssetCombo(const char* label, EAssetType TargetAssetType, FGuid& SelectedAssetGuid)
	{
		FAssetManager& AssetManager = FAssetManager::Get();

		bool IsDirty = false;

		ImGui::Text("%s", label);
		ImGui::SameLine();

		FName CurrentSelectedAssetName;
		TArray<const FAssetMetaInfo*> AssetInfos;
		AssetManager.ForEachMetaInfo([&](const FAssetMetaInfo& metaInfo)
		{
			if (metaInfo.AssetType != TargetAssetType)
			{
				return;
			}

			if (metaInfo.AssetID == SelectedAssetGuid)
			{
				CurrentSelectedAssetName = metaInfo.AssetName;
			}

			AssetInfos.Add(&metaInfo);
		});

		if (ImGui::BeginCombo(std::format("##{}", label).c_str(), CurrentSelectedAssetName.IsValid() ? CurrentSelectedAssetName.ToString().c_str() : "None"))
		{
			for (int32 i = 0; i < AssetInfos.Num(); ++i)
			{
				const FAssetMetaInfo* Info = AssetInfos[i];

				bool IsSelected = Info->AssetID == SelectedAssetGuid;

				ImGui::PushID(i);

				// Path에서 확장자 빼고 파일명만 parsing 하여 보여주기
				std::filesystem::path AssetPath = Info->AssetName.ToString().c_str();
				if (ImGui::Selectable(AssetPath.stem().generic_string().c_str(), IsSelected))
				{
					SelectedAssetGuid = Info->AssetID;
					IsDirty = true;
				}
				ImGui::PopID();

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", Info->AssetName.ToString().c_str());
				}

				if (IsSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("ASSET_GUID"))
			{
				const FGuid& AssetGuid = *static_cast<const FGuid*>(Payload->Data);
				const FAssetMetaInfo& AssetMetaInfo = AssetManager.GetMetaInfo(AssetGuid);

				if (AssetMetaInfo.AssetType == TargetAssetType)
				{
					SelectedAssetGuid = AssetGuid;
					IsDirty = true;
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		if (ImGui::Button(std::format("Clear##{}", label).c_str()))
		{
			SelectedAssetGuid = FGuid();
			IsDirty = true;
		}

		return IsDirty;
	}
};