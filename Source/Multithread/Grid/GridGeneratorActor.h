// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGeneratorActor.generated.h"

class UGridGenerator;

UCLASS()
class MULTITHREAD_API AGridGeneratorActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGridGeneratorActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UGridGenerator> GridComponent;

public:
	FORCEINLINE TObjectPtr<UGridGenerator> GetGridComponent() const { return GridComponent; }
};
