// Fill out your copyright notice in the Description page of Project Settings.

#include "GridGenerator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

UGridGenerator::UGridGenerator()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

FGridTile::FGridTile(int32 InRow, int32 InColumn, const FVector& InPosition)
	: Row(InRow), Column(InColumn), WorldPosition(InPosition)
{
}

void UGridGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (TileComponents.Num() == 0 && (!TileInstancesA || TileInstancesA->GetInstanceCount() == 0))
	{
		GenerateGrid();
	}
}

void UGridGenerator::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	ClearGrid();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
void UGridGenerator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (bUpdateInEditor)
	{
		const FName PropertyName = PropertyChangedEvent.Property
			                           ? PropertyChangedEvent.Property->GetFName()
			                           : NAME_None;
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, GridSize) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, TileSize) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, TileGap) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, TileMeshA) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, TileMeshB) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, TileMaterialA) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, TileMaterialB) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, ObstacleMesh) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, ObstacleMaterial) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, bUseInstancedMeshes))
		{
			HandlePropertyChange();
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, ObstacleParams) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UGridGenerator, RandomSeed))
		{
			if (GridSize > 0 && OccupiedCells.Num() > 0) GenerateObstacles();
		}
	}
}
#endif

void UGridGenerator::GenerateGrid()
{
	ClearGrid();
	if (!TileMeshA || !TileMeshB)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridGenerator: Tile meshes not set!"));
		return;
	}

	OccupiedCells.Init(false, GridSize * GridSize);
	GridTiles.Empty(GridSize * GridSize);
	ExposureMap.Empty();
	TileInstanceIndices.Init(INDEX_NONE, GridSize * GridSize);
	TileInstanceUsesMeshB.Init(false, GridSize * GridSize);

	if (bUseInstancedMeshes) SetupInstancedComponents();

	for (int32 Row = 0; Row < GridSize; Row++)
	{
		for (int32 Col = 0; Col < GridSize; Col++)
		{
			const bool bIsOddTile = (Row % 2 == 0) != (Col % 2 == 0);
			UStaticMesh* TileMesh = bIsOddTile ? TileMeshB : TileMeshA;
			UMaterialInterface* TileMaterial = bIsOddTile ? TileMaterialB : TileMaterialA;
			const FVector Position = GetGridPosition(Row, Col);
			const int32 GridIndex = Row * GridSize + Col;

			GridTiles.Add(FGridTile(Row, Col, Position));

			if (bUseInstancedMeshes)
			{
				FTransform Transform;
				Transform.SetLocation(Position);
				Transform.SetScale3D(FVector(TileSize / 100.0f, TileSize / 100.0f, TileThickness / 100.0f));

				UInstancedStaticMeshComponent* TileInstances = bIsOddTile ? TileInstancesB : TileInstancesA;
				if (TileInstances)
				{
					TileInstanceIndices[GridIndex] = TileInstances->AddInstance(Transform);
					TileInstanceUsesMeshB[GridIndex] = bIsOddTile;
				}
			}
			else
			{
				UStaticMeshComponent* TileComponent = NewObject<UStaticMeshComponent>(GetOwner());
				TileComponent->SetupAttachment(this);
				TileComponent->RegisterComponent();
				TileComponent->SetStaticMesh(TileMesh);
				if (TileMaterial) TileComponent->SetMaterial(0, TileMaterial);
				TileComponent->SetRelativeLocation(Position);

				TileComponents.Add(TileComponent);

				TileActors.Add(TileComponent->GetAttachmentRootActor());
			}
		}
	}

	if (bGenerateObstaclesWithGrid) GenerateObstacles();
	else UpdateNonBlockedTiles();
}

void UGridGenerator::GenerateObstacles()
{
	if (bUseInstancedMeshes && ObstacleInstances) ObstacleInstances->ClearInstances();
	else
	{
		for (UStaticMeshComponent* ObstacleComponent : ObstacleComponents) ObstacleComponent->DestroyComponent();
		ObstacleComponents.Empty();
	}

	for (int32 i = 0; i < OccupiedCells.Num(); i++) OccupiedCells[i] = false;
	if (!ObstacleMesh)
	{
		UpdateNonBlockedTiles();
		return;
	}

	FRandomStream RandomStream(RandomSeed != 0 ? RandomSeed : FMath::Rand());

	for (int32 Row = 0; Row < GridSize; Row++)
	{
		for (int32 Col = 0; Col < GridSize; Col++)
		{
			if (RandomStream.GetFraction() > ObstacleParams.ObstacleDensity) continue;

			const int32 SizeRow = FMath::Clamp(RandomStream.RandRange(ObstacleParams.MinSize, ObstacleParams.MaxSize),
			                                   1, GridSize - Row);
			const int32 SizeCol = FMath::Clamp(RandomStream.RandRange(ObstacleParams.MinSize, ObstacleParams.MaxSize),
			                                   1, GridSize - Col);
			if (!IsRegionFree(Row, Col, SizeRow, SizeCol)) continue;

			MarkRegionOccupied(Row, Col, SizeRow, SizeCol);
			const float Height = RandomStream.FRandRange(ObstacleParams.MinHeight, ObstacleParams.MaxHeight);

			const FVector Position = GetGridPosition(Row, Col) + FVector(
				(SizeCol - 1) * (TileSize + TileGap) / 2.0f,
				(SizeRow - 1) * (TileSize + TileGap) / 2.0f,
				Height / 2.0f
			);

			const FVector Scale(
				(SizeCol * TileSize + (SizeCol - 1) * TileGap) / 100.0f,
				(SizeRow * TileSize + (SizeRow - 1) * TileGap) / 100.0f,
				Height / 100.0f
			);

			if (bUseInstancedMeshes && ObstacleInstances)
			{
				FTransform Transform;
				Transform.SetLocation(Position);
				Transform.SetScale3D(Scale);
				ObstacleInstances->AddInstance(Transform);
			}
			else
			{
				UStaticMeshComponent* ObstacleComponent = NewObject<UStaticMeshComponent>(GetOwner());
				ObstacleComponent->SetupAttachment(this);
				ObstacleComponent->RegisterComponent();
				ObstacleComponent->SetStaticMesh(ObstacleMesh);
				if (ObstacleMaterial) ObstacleComponent->SetMaterial(0, ObstacleMaterial);
				ObstacleComponent->SetRelativeLocation(Position);
				ObstacleComponent->SetRelativeScale3D(Scale);
				ObstacleComponents.Add(ObstacleComponent);
			}
		}
	}
	UpdateNonBlockedTiles();
}

void UGridGenerator::ClearGrid()
{
	CleanupInstancedComponents();
	OccupiedCells.Empty();
	GridTiles.Empty();
	NonBlockedTiles.Empty();
	NonBlockedLocations.Empty();
	ExposureMap.Empty();
	TileInstanceIndices.Empty();
	TileInstanceUsesMeshB.Empty();
}

TArray<FGridTile> UGridGenerator::GetNonBlockedTiles() const
{
	return NonBlockedTiles;
}

TArray<FVector> UGridGenerator::GetNonBlockedTileLocations() const
{
	return NonBlockedLocations;
}

TArray<FVector> UGridGenerator::GetNonExposedLocations() const
{
	if (ExposureMap.Num() != NonBlockedLocations.Num())
	{
		return NonBlockedLocations;
	}

	TArray<FVector> NonExposedLocations;
	NonExposedLocations.Reserve(NonBlockedLocations.Num());
	for (int32 Index = 0; Index < ExposureMap.Num(); ++Index)
	{
		if (!ExposureMap[Index])
		{
			NonExposedLocations.Add(NonBlockedLocations[Index]);
		}
	}
	return NonExposedLocations;
}

void UGridGenerator::UpdateGridColors(const TArray<bool>& Exposure)
{
	if (Exposure.Num() != NonBlockedTiles.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridGenerator: Exposure size mismatch. Exposure=%d NonBlocked=%d"),
		       Exposure.Num(), NonBlockedTiles.Num());
		return;
	}

	ExposureMap = Exposure;

	static const FLinearColor ExposedColor = FLinearColor(0.92f, 0.33f, 0.33f, 1.0f);
	static const FLinearColor UnexposedColor = FLinearColor(0.21f, 0.68f, 0.49f, 1.0f);
	static constexpr float DarkFactor = 0.25f;

	if (bUseInstancedMeshes)
	{
		SetupExposureVisualizationComponents(ExposedColor, UnexposedColor);
	}

	for (int32 Index = 0; Index < NonBlockedTiles.Num(); ++Index)
	{
		const FGridTile& Tile = NonBlockedTiles[Index];
		FLinearColor Color = ExposureMap[Index] ? ExposedColor : UnexposedColor;
		if ((Tile.Row + Tile.Column) % 2)
		{
			Color *= DarkFactor;
			Color.A = 1.0f;
		}

		if (bUseInstancedMeshes)
		{
			AddExposureVisualizationTile(Tile, ExposureMap[Index]);
		}
		else
		{
			SetTileColor(Tile.Row, Tile.Column, Color);
		}
	}
}

void UGridGenerator::AppendTraceIgnoredComponents(TArray<UPrimitiveComponent*>& OutIgnoredComponents) const
{
	if (TileInstancesA)
	{
		OutIgnoredComponents.Add(TileInstancesA);
	}
	if (TileInstancesB)
	{
		OutIgnoredComponents.Add(TileInstancesB);
	}

	for (UStaticMeshComponent* TileComponent : TileComponents)
	{
		if (TileComponent)
		{
			OutIgnoredComponents.Add(TileComponent);
		}
	}
}

FVector UGridGenerator::GetGridPosition(int32 Row, int32 Col) const
{
	// Calculate the total size of a tile with gap
	float TotalTileSize = TileSize + TileGap;
    
	// Calculate the offset of the grid origin based on the grid size and gap
	float HalfExtent = GridSize * TotalTileSize / 2.0f;
    
	// Calculate position in local space with Z offset for the center of the tile
	// Note: We're working in component local space, so we don't need to account for the component's world position here
	FVector LocalPosition = FVector(
		Col * TotalTileSize - HalfExtent + TileGap / 2.0f,
		Row * TotalTileSize - HalfExtent + TileGap / 2.0f,
		TileThickness / 2.0f
	);
    
	// Transform to world space using the component's transform
	// This automatically accounts for the component's own position, rotation, and scale
	return GetComponentTransform().TransformPosition(LocalPosition);
}

bool UGridGenerator::IsRegionFree(int32 StartRow, int32 StartCol, int32 SizeRow, int32 SizeCol) const
{
	if (StartRow < 0 || StartCol < 0 ||
		StartRow + SizeRow > GridSize || StartCol + SizeCol > GridSize)
	{
		return false;
	}

	for (int32 Row = StartRow; Row < StartRow + SizeRow; Row++)
	{
		for (int32 Col = StartCol; Col < StartCol + SizeCol; Col++)
		{
			if (OccupiedCells[Row * GridSize + Col]) return false;
		}
	}
	return true;
}

void UGridGenerator::MarkRegionOccupied(int32 StartRow, int32 StartCol, int32 SizeRow, int32 SizeCol)
{
	// Make sure the OccupiedCells array is initialized
	if (OccupiedCells.Num() != GridSize * GridSize)
	{
		OccupiedCells.Init(false, GridSize * GridSize);
	}

	for (int32 Row = StartRow; Row < StartRow + SizeRow; Row++)
	{
		for (int32 Col = StartCol; Col < StartCol + SizeCol; Col++)
		{
			const int32 Index = Row * GridSize + Col;
			if (Index < OccupiedCells.Num())
			{
				OccupiedCells[Index] = true;
			}
		}
	}
}

void UGridGenerator::SetupInstancedComponents()
{
	if (!bUseInstancedMeshes) return;

	if (!TileInstancesA)
	{
		TileInstancesA = NewObject<UInstancedStaticMeshComponent>(GetOwner(),
		                                                          FName(*FString::Printf(
			                                                          TEXT("%s_TileInstancesA"), *GetName())));
		TileInstancesA->SetupAttachment(this);
		TileInstancesA->SetStaticMesh(TileMeshA);
		TileInstancesA->SetMaterial(0, TileMaterialA);
		TileInstancesA->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TileInstancesA->SetNumCustomDataFloats(4);
		TileInstancesA->RegisterComponent();
	}

	if (!TileInstancesB)
	{
		TileInstancesB = NewObject<UInstancedStaticMeshComponent>(GetOwner(),
		                                                          FName(*FString::Printf(
			                                                          TEXT("%s_TileInstancesB"), *GetName())));
		TileInstancesB->SetupAttachment(this);
		TileInstancesB->SetStaticMesh(TileMeshB);
		TileInstancesB->SetMaterial(0, TileMaterialB);
		TileInstancesB->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TileInstancesB->SetNumCustomDataFloats(4);
		TileInstancesB->RegisterComponent();
	}

	if (!ObstacleInstances)
	{
		ObstacleInstances = NewObject<UInstancedStaticMeshComponent>(GetOwner(),
		                                                             FName(*FString::Printf(
			                                                             TEXT("%s_ObstacleInstances"), *GetName())));
		ObstacleInstances->SetupAttachment(this);
		ObstacleInstances->SetStaticMesh(ObstacleMesh);
		ObstacleInstances->SetMaterial(0, ObstacleMaterial);
		ObstacleInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ObstacleInstances->RegisterComponent();
	}
}

void UGridGenerator::CleanupInstancedComponents()
{
	ClearExposureVisualizationComponents();

	if (TileInstancesA)
	{
		TileInstancesA->ClearInstances();
		if (!bUseInstancedMeshes)
		{
			TileInstancesA->DestroyComponent();
			TileInstancesA = nullptr;
		}
	}
	if (TileInstancesB)
	{
		TileInstancesB->ClearInstances();
		if (!bUseInstancedMeshes)
		{
			TileInstancesB->DestroyComponent();
			TileInstancesB = nullptr;
		}
	}
	if (ObstacleInstances)
	{
		ObstacleInstances->ClearInstances();
		if (!bUseInstancedMeshes)
		{
			ObstacleInstances->DestroyComponent();
			ObstacleInstances = nullptr;
		}
	}

	for (UStaticMeshComponent* TileComponent : TileComponents) TileComponent->DestroyComponent();
	TileComponents.Empty();

	for (UStaticMeshComponent* ObstacleComponent : ObstacleComponents) ObstacleComponent->DestroyComponent();
	ObstacleComponents.Empty();
}

void UGridGenerator::SetupExposureVisualizationComponents(const FLinearColor& ExposedColor,
                                                          const FLinearColor& UnexposedColor)
{
	if (!bUseInstancedMeshes)
	{
		return;
	}

	auto SetupVisualizationComponent =
		[this](UInstancedStaticMeshComponent*& Component, const TCHAR* Suffix, UStaticMesh* Mesh,
		       UMaterialInterface* BaseMaterial, const FLinearColor& Color)
		{
			if (!Component)
			{
				Component = NewObject<UInstancedStaticMeshComponent>(
					GetOwner(), FName(*FString::Printf(TEXT("%s_%s"), *GetName(), Suffix)));
				Component->SetupAttachment(this);
				Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Component->RegisterComponent();
			}

			Component->ClearInstances();
			Component->SetStaticMesh(Mesh);

			UMaterialInterface* SourceMaterial = BaseMaterial
				                                     ? BaseMaterial
				                                     : UMaterial::GetDefaultMaterial(MD_Surface);
			UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
			Component->SetMaterial(0, DynamicMaterial);
		};

	FLinearColor DarkExposedColor = ExposedColor * 0.25f;
	DarkExposedColor.A = 1.0f;
	FLinearColor DarkUnexposedColor = UnexposedColor * 0.25f;
	DarkUnexposedColor.A = 1.0f;

	SetupVisualizationComponent(ExposedTileInstancesA, TEXT("ExposedTileInstancesA"), TileMeshA, TileMaterialA,
	                            ExposedColor);
	SetupVisualizationComponent(ExposedTileInstancesB, TEXT("ExposedTileInstancesB"), TileMeshB, TileMaterialB,
	                            DarkExposedColor);
	SetupVisualizationComponent(UnexposedTileInstancesA, TEXT("UnexposedTileInstancesA"), TileMeshA, TileMaterialA,
	                            UnexposedColor);
	SetupVisualizationComponent(UnexposedTileInstancesB, TEXT("UnexposedTileInstancesB"), TileMeshB, TileMaterialB,
	                            DarkUnexposedColor);
}

void UGridGenerator::ClearExposureVisualizationComponents()
{
	UInstancedStaticMeshComponent* Components[] = {
		ExposedTileInstancesA,
		ExposedTileInstancesB,
		UnexposedTileInstancesA,
		UnexposedTileInstancesB
	};

	for (UInstancedStaticMeshComponent* Component : Components)
	{
		if (Component)
		{
			Component->ClearInstances();
			if (!bUseInstancedMeshes)
			{
				Component->DestroyComponent();
			}
		}
	}

	if (!bUseInstancedMeshes)
	{
		ExposedTileInstancesA = nullptr;
		ExposedTileInstancesB = nullptr;
		UnexposedTileInstancesA = nullptr;
		UnexposedTileInstancesB = nullptr;
	}
}

void UGridGenerator::AddExposureVisualizationTile(const FGridTile& Tile, bool bExposed)
{
	const int32 GridIndex = Tile.Row * GridSize + Tile.Column;
	if (!TileInstanceUsesMeshB.IsValidIndex(GridIndex))
	{
		return;
	}

	const bool bUsesMeshB = TileInstanceUsesMeshB[GridIndex];
	UInstancedStaticMeshComponent* Component = nullptr;
	if (bExposed)
	{
		Component = bUsesMeshB ? ExposedTileInstancesB : ExposedTileInstancesA;
	}
	else
	{
		Component = bUsesMeshB ? UnexposedTileInstancesB : UnexposedTileInstancesA;
	}

	if (!Component)
	{
		return;
	}

	FTransform Transform;
	Transform.SetLocation(Tile.WorldPosition + FVector(0.0f, 0.0f, TileThickness + 0.5f));
	Transform.SetScale3D(FVector(TileSize / 100.0f, TileSize / 100.0f, 0.01f));
	Component->AddInstance(Transform);
}

void UGridGenerator::HandlePropertyChange()
{
	CleanupInstancedComponents();
	GenerateGrid();
}

void UGridGenerator::UpdateNonBlockedTiles()
{
	NonBlockedTiles.Empty();
	NonBlockedLocations.Empty();

	// Make sure we have valid data
	if (GridTiles.Num() == 0 || OccupiedCells.Num() != GridSize * GridSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridGenerator: Cannot update non-blocked tiles - invalid grid data"));
		return;
	}

	for (const FGridTile& Tile : GridTiles)
	{
		const int32 Index = Tile.Row * GridSize + Tile.Column;
		if (Index < OccupiedCells.Num() && !OccupiedCells[Index])
		{
			NonBlockedTiles.Add(Tile);
			NonBlockedLocations.Add(Tile.WorldPosition);
		}
	}

	ExposureMap.Init(false, NonBlockedLocations.Num());
}

void UGridGenerator::SetTileColor(int32 Row, int32 Col, const FLinearColor& Color)
{
	if (Row < 0 || Row >= GridSize || Col < 0 || Col >= GridSize)
	{
		return;
	}
    
	const int32 Index = Row * GridSize + Col;
    
	if (bUseInstancedMeshes)
	{
		if (!TileInstanceIndices.IsValidIndex(Index) || !TileInstanceUsesMeshB.IsValidIndex(Index))
		{
			return;
		}

		const int32 InstanceIndex = TileInstanceIndices[Index];
		if (InstanceIndex == INDEX_NONE)
		{
			return;
		}

		UInstancedStaticMeshComponent* TileInstances = TileInstanceUsesMeshB[Index] ? TileInstancesB : TileInstancesA;
		if (!TileInstances)
		{
			return;
		}

		TileInstances->SetCustomDataValue(InstanceIndex, 0, Color.R, false);
		TileInstances->SetCustomDataValue(InstanceIndex, 1, Color.G, false);
		TileInstances->SetCustomDataValue(InstanceIndex, 2, Color.B, false);
		TileInstances->SetCustomDataValue(InstanceIndex, 3, Color.A, true);
	}
	else
	{
		// For individual static mesh components
		if (Index < TileComponents.Num())
		{
			if (UStaticMeshComponent* TileComponent = TileComponents[Index])
			{
				UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(TileComponent->GetMaterial(0));
				if (!DynamicMaterial)
				{
					DynamicMaterial = TileComponent->CreateAndSetMaterialInstanceDynamic(0);
				}
				if (DynamicMaterial)
				{
					DynamicMaterial->SetVectorParameterValue("Color", Color);
				}
			}
		}
	}
}
