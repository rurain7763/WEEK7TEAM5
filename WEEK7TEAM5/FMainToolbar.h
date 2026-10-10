#pragma once

#include "Core.h"

class FEditorEngine;
struct FGuiReference;

class FMainToolbar
{
public:
	void Render(const FGuiReference& GuiReference);

private:
	void RenderShowFlagsControl(const FGuiReference& GuiReference);
	void RenderSpawnActorControl(const FGuiReference& GuiReference);
	void RenderPlayControl(const FGuiReference& GuiReference);

private:
	int32 mSelectedTargetSpawnIndex = 1;
	int32 mSpawnCount = 1;
};
