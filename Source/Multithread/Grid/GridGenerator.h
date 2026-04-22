#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GridGenerator.generated.h"

class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct FGridTile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	int32 Row = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	int32 Column = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	FVector WorldPosition = FVector::ZeroVector;

	FGridTile() = default;
	FGridTile(int32 InRow, int32 InColumn, const FVector& InPosition);
};

USTRUCT(BlueprintType)
struct FObstacleParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle", meta=(ClampMin="1"))
	int32 MinSize = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle", meta=(ClampMin="1"))
	int32 MaxSize = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle", meta=(ClampMin="0"))
	float MinHeight = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle", meta=(ClampMin="0"))
	float MaxHeight = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ObstacleDensity = 0.1f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="Grid Generator"))
class MULTITHREAD_API UGridGenerator : public USceneComponent
{
	GENERATED_BODY()

public:
	UGridGenerator();
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category="Grid")
	void GenerateGrid();

	UFUNCTION(BlueprintCallable, Category="Grid")
	void GenerateObstacles();

	UFUNCTION(BlueprintCallable, Category="Grid")
	void ClearGrid();

	UFUNCTION(BlueprintCallable, Category="Grid")
	TArray<FGridTile> GetNonBlockedTiles() const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FVector> GetNonBlockedTileLocations() const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FVector> GetNonExposedLocations() const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void UpdateGridColors(const TArray<bool>& Exposure);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	FORCEINLINE TArray<AActor*> GetTileActors() { return TileActors; }

	void AppendTraceIgnoredComponents(TArray<UPrimitiveComponent*>& OutIgnoredComponents) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid", meta=(ClampMin="1"))
	int32 GridSize = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	float TileSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid", meta=(ClampMin="0.1"))
	float TileThickness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	float TileGap = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	UStaticMesh* TileMeshA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	UStaticMesh* TileMeshB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	UMaterialInterface* TileMaterialA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	UMaterialInterface* TileMaterialB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle")
	UStaticMesh* ObstacleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle")
	UMaterialInterface* ObstacleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle")
	FObstacleParameters ObstacleParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle")
	int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle")
	bool bGenerateObstaclesWithGrid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	bool bUpdateInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Performance")
	bool bUseInstancedMeshes = true;

private:
	UPROPERTY()
	TArray<bool> OccupiedCells;

	UPROPERTY()
	TArray<FGridTile> GridTiles;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> TileActors;

	UPROPERTY()
	TArray<FGridTile> NonBlockedTiles;

	UPROPERTY()
	TArray<FVector> NonBlockedLocations;

	UPROPERTY()
	TArray<bool> ExposureMap;

	UPROPERTY()
	TArray<UStaticMeshComponent*> TileComponents;
	UPROPERTY()
	TArray<UStaticMeshComponent*> ObstacleComponents;
	UPROPERTY()
	class UInstancedStaticMeshComponent* TileInstancesA;
	UPROPERTY()
	class UInstancedStaticMeshComponent* TileInstancesB;
	UPROPERTY()
	class UInstancedStaticMeshComponent* ObstacleInstances;
	UPROPERTY()
	class UInstancedStaticMeshComponent* ExposedTileInstancesA;
	UPROPERTY()
	class UInstancedStaticMeshComponent* ExposedTileInstancesB;
	UPROPERTY()
	class UInstancedStaticMeshComponent* UnexposedTileInstancesA;
	UPROPERTY()
	class UInstancedStaticMeshComponent* UnexposedTileInstancesB;
	UPROPERTY()
	TArray<int32> TileInstanceIndices;
	UPROPERTY()
	TArray<bool> TileInstanceUsesMeshB;

	FVector GetGridPosition(int32 Row, int32 Col) const;
	bool IsRegionFree(int32 StartRow, int32 StartCol, int32 SizeRow, int32 SizeCol) const;
	void MarkRegionOccupied(int32 StartRow, int32 StartCol, int32 SizeRow, int32 SizeCol);
	void SetupInstancedComponents();
	void CleanupInstancedComponents();
	void SetupExposureVisualizationComponents(const FLinearColor& ExposedColor, const FLinearColor& UnexposedColor);
	void ClearExposureVisualizationComponents();
	void AddExposureVisualizationTile(const FGridTile& Tile, bool bExposed);
	void HandlePropertyChange();

	void UpdateNonBlockedTiles();

	void SetTileColor(int32 Row, int32 Col, const FLinearColor& Color);
};
