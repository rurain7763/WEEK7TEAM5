#pragma once

#include "Core.h"

enum class EAxis : int { X = 0, Y = 1, Z = 2 };

enum class EPrimitive
{
	EP_Sphere,
	EP_Cube,
	EP_Triangle,
	EP_GizmoArrow,
	EP_Circle,
	EP_Plane,
	EP_StaticMesh,
};

enum EGIZMO_AXIS //어떤축이 선택되었는지
{
	NONE,
	X,
	Y,
	Z
};

enum EGIZMO_TYPE {
	TRANSLATE,
	ROTATE,
	SCALE,
};

enum class EViewModeIndex
{
	// 기존 뷰포트 설정의 숫자 값을 유지하고 새 모드는 뒤에 추가합니다.
	VMI_Lit_Phong,
	VMI_Unlit,
	VMI_Wireframe,
	VMI_SceneDepth,
	VMI_WorldNormal,
	VMI_Lit_Gouraud,
	VMI_Lit_Lambert,

	// 실제 뷰 모드가 아니다. 뷰 모드별 배열 크기를 잡는 데 쓴다.
	// 모드를 추가하면 이 앞에 넣을 것.
	VMI_Max,
};

enum class ECoordinateSpace
{
	World,
	Screen
};

enum class EViewportType
{
	Perspective,
	Top,
	Front,
	Side,

	// ViewportType을 세기 위한 변수 뷰포트 타입 추가는 위에 작성
	Max
};

enum class EWorldType
{
	Editor,
	EditorPreview,
	PIE,
	Game,
};

enum ELightType
{
	Directional,
	Point,
	Spot,
};

enum class ELevelTick
{
	ViewportsOnly,
	All,
};

enum class EEndPlayReason
{
	Destroyed,
	LevelTransition,
	EndPlayInEditor,
	RemovedFromWorld,
	Quit,
};

enum ETextureMap : uint32
{
	TM_BaseColor = 1 << 0,
	TM_Specular = 1 << 1,
	TM_Normal = 1 << 2,
};

inline void EnumSetFlags(uint32& Flags, int32 FlagToSet, bool bEnable)
{
	if (bEnable)
	{
		Flags |= FlagToSet;
	}
	else
	{
		Flags &= ~FlagToSet;
	}
}

