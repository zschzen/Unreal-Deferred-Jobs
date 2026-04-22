// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MultithreadAIController.generated.h"

UCLASS()
class MULTITHREAD_API AMultithreadAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMultithreadAIController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;
	
	TObjectPtr<class UGridGenerator> GridComponent;

public:
	// Move the pawn to the target location
	void MoveToTargetLocation(const FVector& TargetLocation);
};
