// Fill out your copyright notice in the Description page of Project Settings.


#include "MultithreadAIController.h"

#include "EngineUtils.h"
#include "Grid/GridGenerator.h"
#include "Grid/GridGeneratorActor.h"


AMultithreadAIController::AMultithreadAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	AActor::SetActorTickEnabled(true);
}

void AMultithreadAIController::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AGridGeneratorActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		const AGridGeneratorActor* grid = Cast<AGridGeneratorActor>(*ActorItr);
		if (!grid) continue;

		GridComponent = grid->GetGridComponent();
	}
}

void AMultithreadAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//return;
	if (!GridComponent) return;
	if (GridComponent->GetNonBlockedTileLocations().Num() < 1) return;
	if (!GetPawn()) return;

	// Get the non-exposed tiles
	const TArray<FVector> NonExposedTiles = GridComponent->GetNonExposedLocations();

	if (NonExposedTiles.Num() < 1) return;

	// Get the current location of the pawn
	const FVector PawnLocation = GetPawn()->GetActorLocation();

	// Get the closest tile to the pawn
	FVector ClosestTileLocation = FVector::ZeroVector;
	float NearestDistance = TNumericLimits<float>::Max();
	const int Length = NonExposedTiles.Num();

	for (int i = 0; i < Length; i++)
	{
		const auto& TileLocation = NonExposedTiles[i];
		const float Distance = FVector::Dist(PawnLocation, TileLocation);

		if (Distance >= NearestDistance) continue;
		NearestDistance = Distance;
		ClosestTileLocation = TileLocation;
	}

	MoveToTargetLocation(ClosestTileLocation);
}

void AMultithreadAIController::MoveToTargetLocation(const FVector& TargetLocation)
{
	MoveToLocation(TargetLocation);
}
