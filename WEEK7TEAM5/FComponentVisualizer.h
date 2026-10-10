#pragma once

#include "RenderInfo.h"
#include "ActorComponent.h"
#include "TMap.h"
#include "Object.h"
#include "UTextComponent.h"
#include "EngineMathLibrary.h"
#include "LightComponents.h"

class FComponentVisualizer
{
public:
	virtual ~FComponentVisualizer() = default;

	virtual void VisualizeComponent(UActorComponent* Component, FRenderCollector& RenderCollector) = 0;
};

class FSpotLightComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void VisualizeComponent(UActorComponent* Component, FRenderCollector& RenderCollector) override
	{
		USpotLightComponent* SpotLightComponent = Component->Cast<USpotLightComponent>();
		if (!SpotLightComponent)
		{
			return;
		}

		const FVector Origin = SpotLightComponent->GetWorldLocation();
		const FMatrix Rotation = ToMatrix(SpotLightComponent->GetWorldRotation());
		const FVector Forward = Rotation.GetUnitAxis(EAxis::X);
		const FVector Right = Rotation.GetUnitAxis(EAxis::Y);
		const FVector Up = Rotation.GetUnitAxis(EAxis::Z);

		const float Range = SpotLightComponent->GetRadius();

		if (Range <= 0.f) 
		{ 
			return; 
		}

		const float OuterAngle = SpotLightComponent->GetOuterConeAngle(); 
		const float InnerAngle = SpotLightComponent->GetInnerConeAngle();

		constexpr int32 CircleSegments = 32;
		constexpr int32 ArcSegments = 16;

		auto AddLine = [&](const FVector& Start, const FVector& End, const FLinearColor& Color)
		{
			FRenderLineInfo Line;
			Line.Start = Start;
			Line.End = End;
			Line.Color = Color;
			Line.Thickness = 5.0f;
			RenderCollector.LineInfos.Add(Line);
		};

		auto DrawCone = [&](float AngleRadians, const FLinearColor& Color)
		{
			const float Theta = AngleRadians;
			const float Height = Range * FMath::Cos(Theta);
			const float Radius = Range * FMath::Sin(Theta);

			FVector CirclePoints[CircleSegments];
			GenerateCircleVertices([&](int32 Index, const FVector2& CircleVertex) {
				CirclePoints[Index] = Origin + Forward * Height + Right * (CircleVertex.X * Radius) + Up * (CircleVertex.Y * Radius);
			}, 1.f, CircleSegments);

			for (int32 Index = 0; Index < CircleSegments; ++Index)
			{
				int32 NextIndex = (Index + 1) % CircleSegments;
				AddLine(CirclePoints[Index], CirclePoints[NextIndex], Color);
			}

			for (int32 Index = 0; Index < CircleSegments; Index += CircleSegments / 4)
			{
				AddLine(Origin, CirclePoints[Index], Color);
			}
		};

		const FLinearColor ConeColor = SpotLightComponent->GetColor();

		DrawCone(OuterAngle, ConeColor);
		if (InnerAngle > 0.f && InnerAngle < OuterAngle)
		{
			DrawCone(InnerAngle, ConeColor);
		}
	}
};

class FPointLightComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void VisualizeComponent(UActorComponent* Component, FRenderCollector& RenderCollector) override
	{
		UPointLightComponent* PointLightComponent = Component->Cast<UPointLightComponent>();
		if (!PointLightComponent)
		{
			return;
		}

		const FVector Origin = PointLightComponent->GetWorldLocation();

		const float Range = PointLightComponent->GetRadius();

		if (Range <= 0.f)
		{
			return;
		}

		constexpr int32 CircleSegments = 32;

		auto AddLine = [&](const FVector& Start, const FVector& End, const FLinearColor& Color)
		{
			FRenderLineInfo Line;
			Line.Start = Start;
			Line.End = End;
			Line.Color = Color;
			Line.Thickness = 5.0f;
			RenderCollector.LineInfos.Add(Line);
		};

		auto DrawCircle = [&](const FVector& Center, const FVector& Right, const FVector& Up, float Radius, const FLinearColor& Color)
		{
			FVector CirclePoints[CircleSegments];
			GenerateCircleVertices([&](int32 Index, const FVector2& CircleVertex) {
				CirclePoints[Index] = Center + Right * (CircleVertex.X * Radius) + Up * (CircleVertex.Y * Radius);
			}, 1.f, CircleSegments);

			for (int32 Index = 0; Index < CircleSegments; ++Index)
			{
				int32 NextIndex = (Index + 1) % CircleSegments;
				AddLine(CirclePoints[Index], CirclePoints[NextIndex], Color);
			}
		};

		DrawCircle(Origin, Right, Up, Range, PointLightComponent->GetColor());
		DrawCircle(Origin, Right, Front, Range, PointLightComponent->GetColor());
		DrawCircle(Origin, Up, Front, Range, PointLightComponent->GetColor());
	}
};

class FDirectionalLightComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void VisualizeComponent(UActorComponent* Component, FRenderCollector& RenderCollector) override
	{
		UDirectionalLightComponent* DirectionalLightComponent = Component->Cast<UDirectionalLightComponent>();
		if (!DirectionalLightComponent)
		{
			return;
		}

		const FVector Origin = DirectionalLightComponent->GetWorldLocation();
		const FMatrix Rotation = ToMatrix(DirectionalLightComponent->GetWorldRotation());
		const FVector Forward = Rotation.GetUnitAxis(EAxis::X);

		const float Length = 2.0f;

		FVector End = Origin + Forward * Length;

		FRenderLineInfo Line;
		Line.Start = Origin;
		Line.End = End;
		Line.Color = DirectionalLightComponent->GetColor();
		Line.Thickness = 5.0f;

		RenderCollector.LineInfos.Add(Line);
	}
};

class FComponentVisualizerManager
{
public:
	FComponentVisualizerManager()
	{
		RegisterVisualizer(USpotLightComponent::GetStaticClass(), MakeShared<FSpotLightComponentVisualizer>());
		RegisterVisualizer(UPointLightComponent::GetStaticClass(), MakeShared<FPointLightComponentVisualizer>());
		RegisterVisualizer(UDirectionalLightComponent::GetStaticClass(), MakeShared<FDirectionalLightComponentVisualizer>());
	}

	void RegisterVisualizer(const FClassInfo* ComponentClass, TSharedPtr<FComponentVisualizer> Visualizer)
	{
		FVisualizerEntry Entry;
		Entry.Visualizer = Visualizer;
		Entry.bMuted = false;

		mVisualizers.Add(ComponentClass, Entry);
	}

	void UnregisterVisualizer(const FClassInfo* ComponentClass)
	{
		mVisualizers.Remove(ComponentClass);
	}

	void MuteVisualizer(const FClassInfo* ComponentClass)
	{
		FVisualizerEntry* Entry = mVisualizers.Find(ComponentClass);
		if (Entry)
		{
			Entry->bMuted = true;
		}
	}

	void UnmuteVisualizer(const FClassInfo* ComponentClass)
	{
		FVisualizerEntry* Entry = mVisualizers.Find(ComponentClass);
		if (Entry)
		{
			Entry->bMuted = false;
		}
	}

	FComponentVisualizer* FindVisualizer(const FClassInfo* ComponentClass)
	{
		FVisualizerEntry* Entry = mVisualizers.Find(ComponentClass);
		return Entry ? Entry->Visualizer.get() : nullptr;
	}

private:
	struct FVisualizerEntry
	{
		TSharedPtr<FComponentVisualizer> Visualizer;
		bool bMuted = false;
	};

	TMap<const FClassInfo*, FVisualizerEntry> mVisualizers;
};
