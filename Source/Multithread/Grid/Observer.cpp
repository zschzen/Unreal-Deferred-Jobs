// Observer.cpp
#include "Observer.h"

#include "DrawDebugHelpers.h"
#include "GridGenerator.h"
#include "LineTraceWorker.h"
#include "Multithread/MultithreadCharacter.h"
#include "../RaysControl.h"

// Sets default values for this component's properties
UObserver::UObserver()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.
    PrimaryComponentTick.bCanEverTick = true;

    // Enable tick
    UActorComponent::SetComponentTickEnabled(true);
}

// Called when the game starts
void UObserver::BeginPlay()
{
    Super::BeginPlay();

    // Initialize exposure array
    Exposure.Empty();

    // Create rays control widget
    if (RaysControlClass)
    {
        RaysControl = CreateWidget<URaysControl>(GetWorld(), RaysControlClass);
        if (RaysControl)
        {
            RaysControl->AddToViewport();
            RaysControl->OnSliderValueChangedDelegate.AddDynamic(this, &UObserver::SetRaysPerTimeSlice);
            RaysControl->SetSliderValue(PercentageOfRaysPerTimeSlice);
        }
    }
}

void UObserver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // Properly clean up any active workers
    if (Worker)
    {
        Worker->GetThread()->WaitForCompletion();
        Worker.Reset();
    }

    // Destroy rays control widget
    if (RaysControl)
    {
        RaysControl->OnSliderValueChangedDelegate.RemoveAll(this);
        RaysControl->RemoveFromParent();
        RaysControl = nullptr;
    }
}

// Called every frame
void UObserver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Early exit if GridComponent is not set or if not using FRunnable but trying to use it
    if (!GridComponent)
    {
        return;
    }

    if (bUseFRunnable)
    {
        const int32 NumTiles = GridComponent->GetNonBlockedTiles().Num();
        if (NumTiles < 1)
        {
            return;
        }

        // Calculate number of rays per time slice based on percentage
        const int32 RaysPerTimeSlice = FMath::Max(1, FMath::CeilToInt(NumTiles * PercentageOfRaysPerTimeSlice));
        UpdateRaysControlDisplay(RaysPerTimeSlice, NumTiles);
        UpdateExposureMapRunnable(RaysPerTimeSlice);
    }
    else
    {
        UpdateExposureMapNormal();
    }
}

void UObserver::UpdateExposureMapNormal()
{
    if (!GridComponent || !GetWorld())
    {
        return;
    }

    const FVector Start = GetOwner()->GetActorLocation();
    const TArray<FVector>& TileLocations = GridComponent->GetNonBlockedTileLocations();
    const int32 NumTiles = TileLocations.Num();
    
    // Pre-allocate with exact size needed
    Exposure.Empty(NumTiles);
    Exposure.AddUninitialized(NumTiles);
    
    // Create params once outside the loop
    FCollisionQueryParams Params(FName(TEXT("LineTraceSingle")), true, GetOwner());
    TArray<UPrimitiveComponent*> IgnoredComponents;
    GridComponent->AppendTraceIgnoredComponents(IgnoredComponents);
    Params.AddIgnoredComponents(IgnoredComponents);
    
    for (int32 i = 0; i < NumTiles; i++)
    {
        const FVector& End = TileLocations[i];
        FHitResult HitResult;
        const bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult, Start, End, ECC_Visibility, Params);
        
        Exposure[i] = !bHit;
    }

    if (OnExposureMapUpdated.IsBound())
    {
        OnExposureMapUpdated.Broadcast(Exposure);
    }

    GridComponent->UpdateGridColors(Exposure);
    Exposure.Empty(NumTiles); // Keep capacity for next use
}

void UObserver::UpdateExposureMapRunnable(int32 NumRaysPerTimeSlice)
{
    if (!GridComponent || !GetWorld())
    {
        return;
    }

    const int32 NumTiles = GridComponent->GetNonBlockedTileLocations().Num();
    NumRaysPerTimeSlice = FMath::Min(NumRaysPerTimeSlice, NumTiles);

    // Process results from previous worker if available
    if (Worker)
    {
        // Check if worker is done before waiting
        if (!Worker->IsWorkDone())
        {
            Worker->GetThread()->WaitForCompletion();
        }
        
        // Append the results to our exposure array
        TArray<bool> WorkerResults = Worker->GetExposureResults();
        Exposure.Append(WorkerResults);

        // Draw debug lines if enabled
        if (bDrawDebugLines && TimeSliceBaseIndex >= 0)
        {
            const FVector Start = GetOwner()->GetActorLocation();
            const TArray<FVector>& TileLocations = GridComponent->GetNonBlockedTileLocations();
            
            for (int32 i = 0; i < WorkerResults.Num(); i++)
            {
                if (const int32 Index = TimeSliceBaseIndex - WorkerResults.Num() + i; Index >= 0 && Index < NumTiles)
                {
                    const FVector& End = TileLocations[Index];
                    const FColor Color = WorkerResults[i] ? FColor::Red : FColor::Green;
                    
                    DrawDebugLine(GetWorld(), Start, End, Color, false, 
                                 DebugLineDuration, 0, DebugLineThickness);
                }
            }
        }
    }

    // Trim excess ray count for the last time slice of batch
    int32 NumExcessRays = TimeSliceBaseIndex + NumRaysPerTimeSlice - NumTiles;
    if (NumExcessRays > 0)
    {
        NumRaysPerTimeSlice -= NumExcessRays;
    }

    // Check if batch ended
    if (TimeSliceBaseIndex < 0)
    {
        // Process final results
        if (OnExposureMapUpdated.IsBound())
        {
            OnExposureMapUpdated.Broadcast(Exposure);
        }
        
        GridComponent->UpdateGridColors(Exposure);
        
        // Clean up
        Worker.Reset();
        Exposure.Empty(NumTiles); // Empty but preserve capacity
        
        // Reset time slicing index
        TimeSliceBaseIndex = 0;
        return;
    }

    // Create new worker for the next batch
    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(GetOwner());

    TArray<UPrimitiveComponent*> IgnoredComponents;
    GridComponent->AppendTraceIgnoredComponents(IgnoredComponents);

    Worker = MakeShared<ULineTraceWorker>(
        GetOwner()->GetActorLocation(),
        GridComponent->GetNonBlockedTileLocations(),
        GetWorld(),
        IgnoredActors,
        IgnoredComponents,
        TimeSliceBaseIndex,
        NumRaysPerTimeSlice);

    // Advance time slice index
    TimeSliceBaseIndex += NumRaysPerTimeSlice;

    // Check for end of batch
    if (TimeSliceBaseIndex >= NumTiles)
    {
        TimeSliceBaseIndex = -1; // Signal end of batch
    }
}

void UObserver::SetGridComponent(const TObjectPtr<UGridGenerator> NewGridComponent)
{
    GridComponent = NewGridComponent;
}

void UObserver::SetRaysPerTimeSlice(float Value)
{
    PercentageOfRaysPerTimeSlice = Value;

    if (!GridComponent)
    {
        return;
    }

    const int32 TotalRays = GridComponent->GetNonBlockedTileLocations().Num();
    const int32 RaysPerTimeSlice = TotalRays > 0
        ? FMath::Max(1, FMath::CeilToInt(TotalRays * PercentageOfRaysPerTimeSlice))
        : 0;
    UpdateRaysControlDisplay(RaysPerTimeSlice, TotalRays);
}

void UObserver::UpdateRaysControlDisplay(int32 RaysPerTimeSlice, int32 TotalRays) const
{
    if (!RaysControl)
    {
        return;
    }

    RaysControl->SetRayBatchDisplay(RaysPerTimeSlice, TotalRays, PercentageOfRaysPerTimeSlice);
}

void UObserver::SetDebugDrawing(bool bEnable, float Duration, float Thickness)
{
    bDrawDebugLines = bEnable;
    DebugLineDuration = Duration;
    DebugLineThickness = Thickness;
}
