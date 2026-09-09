// Observer.cpp
#include "Observer.h"

#include "DrawDebugHelpers.h"
#include "GridGenerator.h"
#include "DeferredWorkSystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Jobs/ExposureTraceJob.h"
#include "Multithread/MultithreadCharacter.h"
#include "UIBaseController.h"
#include "UIBaseSubsystem.h"
#include "../UI/RaysViewModel.h"

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

    // The rays widget is created by the UIBase Controller, not here. This
    // component only mirrors numbers into the ViewModel, resolved lazily from
    // TickComponent.
}

void UObserver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // Cancel any in-flight sweep without blocking: worker lambdas hold only
    // weak pointers and expire harmlessly.
    if (ActiveBatch.IsValid())
    {
        if (const UWorld* World = GetWorld())
        {
            if (UDeferredWorkSystem* System = World->GetSubsystem<UDeferredWorkSystem>())
            {
                System->Cancel(ActiveBatch);
            }
        }
        ActiveBatch = FDeferredJobHandle();
        ActiveJob.Reset();
    }

    // Drop the subscription only. The ViewModel belongs to the Controller, which
    // deinitializes it on the next PlayerControllerChanged.
    if (RaysViewModel)
    {
        RaysViewModel->OnSliderValueRequested.Remove(SliderHandle);
        SliderHandle.Reset();
        RaysViewModel->OnDebugDrawRequested.Remove(DebugHandle);
        DebugHandle.Reset();
        RaysViewModel = nullptr;
    }
}

// Called every frame
void UObserver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Early exit if no grid to sweep
    if (!GridComponent)
    {
        return;
    }

    if (bUseDeferredJobs)
    {
        const int32 NumTiles = GridComponent->GetNonBlockedTiles().Num();
        if (NumTiles < 1)
        {
            return;
        }

        // Calculate number of rays per time slice based on percentage
        const int32 RaysPerTimeSlice = FMath::Max(1, FMath::CeilToInt(NumTiles * PercentageOfRaysPerTimeSlice));
        int32 Gathered = 0;
        if (UDeferredWorkSystem* ProgressSystem = GetWorld()->GetSubsystem<UDeferredWorkSystem>())
        {
            int32 ProgressTotal = 0;
            if (!ProgressSystem->GetProgress(ActiveBatch, Gathered, ProgressTotal))
            {
                Gathered = 0;
            }
            // Retune the in-flight batch: the next kicked slice uses the new
            // size, so slider drags take effect mid-sweep instead of waiting
            // for HandleExposureBatchFinished. No-op on invalid handles.
            ProgressSystem->SetNumPerSlice(ActiveBatch, RaysPerTimeSlice);
        }
        if (URaysViewModel* ViewModel = ResolveViewModel())
        {
            ViewModel->SetBatchStats(RaysPerTimeSlice, NumTiles, Gathered, SweepCount);
        }
        UpdateExposureDeferred(RaysPerTimeSlice);
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

    const FVector Start = GetOwner()->GetActorLocation() + FVector(0.0, 0.0, TraceHeightOffset);
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

    TArray<FTraceDebugRecord> Records;
    if (bDrawDebugLines)
    {
        Records.Reserve(NumTiles);
    }
    
    for (int32 i = 0; i < NumTiles; i++)
    {
        const FVector& End = TileLocations[i];
        FHitResult HitResult;
        const bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult, Start, End, ECC_Visibility, Params);
        
        Exposure[i] = !bHit;

        if (bDrawDebugLines)
        {
            FTraceDebugRecord Record;
            Record.Start = Start;
            Record.End = End;
            Record.bHit = bHit;
            Record.Location = End;
            if (bHit)
            {
                Record.Location = HitResult.Location;
            }
            Records.Add(Record);
        }
    }

    if (bDrawDebugLines)
    {
        DrawTraceRecords(Records, DebugLineDuration);
    }

    if (OnExposureMapUpdated.IsBound())
    {
        OnExposureMapUpdated.Broadcast(Exposure);
    }

    GridComponent->UpdateGridColors(Exposure);
    Exposure.Empty(NumTiles); // Keep capacity for next use
}

void UObserver::UpdateExposureDeferred(int32 NumRaysPerTimeSlice)
{
    UWorld* World = GetWorld();
    if (!GridComponent || !World)
    {
        return;
    }

    UDeferredWorkSystem* System = World->GetSubsystem<UDeferredWorkSystem>();
    if (!System)
    {
        return;
    }

    // A sweep is already running: its slices gather kicked work and kick the
    // next slice inside the system tick. Nothing to do here but wait for
    // HandleExposureBatchFinished. Never blocks (Chou delayed gather).
    if (ActiveBatch.IsValid() && !System->IsDone(ActiveBatch))
    {
        return;
    }
    ActiveBatch = FDeferredJobHandle();
    ActiveJob.Reset();

    const TArray<FVector>& TileLocations = GridComponent->GetNonBlockedTileLocations();
    const int32 NumTiles = TileLocations.Num();
    if (NumTiles < 1)
    {
        return;
    }
    NumRaysPerTimeSlice = FMath::Clamp(NumRaysPerTimeSlice, 1, NumTiles);

    // Snapshot everything on the game thread: the job never touches UObjects.
    FCollisionQueryParams Params(FName(TEXT("LineTraceSingle")), true, GetOwner());
    TArray<UPrimitiveComponent*> IgnoredComponents;
    GridComponent->AppendTraceIgnoredComponents(IgnoredComponents);
    Params.AddIgnoredComponents(IgnoredComponents);

    TArray<FVector> EndsCopy = TileLocations;
    TSharedPtr<FExposureTraceJob> Job = MakeShared<FExposureTraceJob>(
        World, GetOwner()->GetActorLocation(), MoveTemp(EndsCopy), Params, ECC_Visibility, TraceHeightOffset);
    FOnExposureBatchFinished DoneDelegate;
    DoneDelegate.BindUObject(this, &UObserver::HandleExposureBatchFinished);
    Job->SetOnFinished(DoneDelegate);

    // Bound unconditionally: HandleExposureSliceGathered checks bDrawDebugLines
    // itself, so a checkbox toggle mid-sweep takes effect immediately instead
    // of only on the next submit.
    FOnExposureSliceGathered SliceDelegate;
    SliceDelegate.BindUObject(this, &UObserver::HandleExposureSliceGathered);
    Job->SetOnSliceGathered(SliceDelegate);

    ActiveJob = Job;
    ActiveBatch = System->Submit(Job, NumRaysPerTimeSlice);
    ++SweepCount;

    // Track the sweep period so line lifetime matches it at any slice count.
    const double SubmitNowSeconds = FPlatformTime::Seconds();
    if (LastSubmitSeconds > 0.0)
    {
        const double Period = FMath::Max(SubmitNowSeconds - LastSubmitSeconds, KINDA_SMALL_NUMBER);
        SweepPeriodEMASeconds = 0.7 * SweepPeriodEMASeconds + 0.3 * Period;
    }
    LastSubmitSeconds = SubmitNowSeconds;
    LastSweepSliceCount = FMath::Max(1, FMath::DivideAndRoundUp(NumTiles, NumRaysPerTimeSlice));
}

void UObserver::HandleExposureSliceGathered(const TArray<FTraceDebugRecord>& SliceRecords)
{
    if (!bDrawDebugLines)
    {
        return;
    }

    // Period-matched trail: the visible fraction of a sweep stays constant at any
    // slice count, full-bright always, so static frames fuse instead of strobing.
    const float Duration = static_cast<float>(FMath::Clamp(
        SweepPeriodEMASeconds * FMath::Max(1, DebugTrailSlices) / FMath::Max(1, LastSweepSliceCount),
        1.0 / 60.0, 0.5));
    DrawTraceRecords(SliceRecords, Duration);
}

void UObserver::HandleExposureBatchFinished(const TArray<bool>& FinishedExposure)
{
    ActiveBatch = FDeferredJobHandle();
    ActiveJob.Reset();

    if (!GridComponent || !GetWorld())
    {
        return;
    }

    Exposure = FinishedExposure;

    if (OnExposureMapUpdated.IsBound())
    {
        OnExposureMapUpdated.Broadcast(Exposure);
    }

    GridComponent->UpdateGridColors(Exposure);
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
    // Only ever reached from the ViewModel's own delegate, so it is live here.
    if (RaysViewModel)
    {
        RaysViewModel->SetBatchStats(RaysPerTimeSlice, TotalRays, 0, SweepCount);
    }
}

void UObserver::SetDebugDrawEnabled(bool bEnabled)
{
    bDrawDebugLines = bEnabled;
}

URaysViewModel* UObserver::ResolveViewModel()
{
    if (RaysViewModel)
    {
        return RaysViewModel;
    }

    const UWorld* World = GetWorld();
    const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
    const UUIBaseSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UUIBaseSubsystem>() : nullptr;
    UUIBaseController* Controller = Subsystem ? Subsystem->GetController() : nullptr;
    if (!Controller)
    {
        // The Controller is built in PlayerControllerChanged, which may not have run
        // yet. Stays silent on purpose: this is polled every frame.
        return nullptr;
    }

    RaysViewModel = Cast<URaysViewModel>(Controller->GetOrCreateViewModel(URaysViewModel::StaticClass()));
    if (RaysViewModel)
    {
        SliderHandle = RaysViewModel->OnSliderValueRequested.AddUObject(this, &UObserver::SetRaysPerTimeSlice);
        RaysViewModel->SetSliderValue(PercentageOfRaysPerTimeSlice);
        DebugHandle = RaysViewModel->OnDebugDrawRequested.AddUObject(this, &UObserver::SetDebugDrawEnabled);
        RaysViewModel->SetDebugDraw(bDrawDebugLines);
    }

    return RaysViewModel;
}

void UObserver::DrawTraceRecords(const TArray<FTraceDebugRecord>& Records, float Duration) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (const FTraceDebugRecord& Record : Records)
    {
        const bool bExposed = !Record.bHit;
        if (DebugDrawMode == EExposureDebugDrawMode::ExposedOnly && !bExposed)
        {
            continue;
        }
        if (DebugDrawMode == EExposureDebugDrawMode::BlockedOnly && bExposed)
        {
            continue;
        }

        if (Record.bHit)
        {
            DrawDebugLine(World, Record.Start, Record.Location, FColor::Green, false, Duration, 0, DebugLineThickness);
            DrawDebugPoint(World, Record.Location, 8.0f, FColor::Green, false, Duration);
        }
        else
        {
            DrawDebugLine(World, Record.Start, Record.End, FColor::Red, false, Duration, 0, DebugLineThickness);
        }
    }
}

void UObserver::SetDebugDrawing(bool bEnable, float Duration, float Thickness, EExposureDebugDrawMode Mode)
{
    bDrawDebugLines = bEnable;
    DebugLineDuration = Duration;
    DebugLineThickness = Thickness;
    DebugDrawMode = Mode;
}
