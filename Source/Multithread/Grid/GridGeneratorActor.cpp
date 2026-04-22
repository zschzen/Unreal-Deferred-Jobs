// Fill out your copyright notice in the Description page of Project Settings.


#include "GridGeneratorActor.h"

#include "GridGenerator.h"
#include "Multithread/MultithreadGameMode.h"


// Sets default values
AGridGeneratorActor::AGridGeneratorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	GridComponent = CreateDefaultSubobject<UGridGenerator>(TEXT("GridGenerator"));
	SetRootComponent(GridComponent);
}

// Called when the game starts or when spawned
void AGridGeneratorActor::BeginPlay()
{
	Super::BeginPlay();
	
	auto GameMode = Cast<AMultithreadGameMode>(GetWorld()->GetAuthGameMode());
}
