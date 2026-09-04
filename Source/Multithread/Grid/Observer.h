// Observer.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/SharedPointer.h"
#include "JobHandle.h"
#include "Jobs/ExposureTraceJob.h"
#include "Observer.generated.h"

class UGridGenerator;
// Forward declarations
class UGridComponent;
class URaysControl;
class UDeferredWorkSystem;

UENUM(BlueprintType)
enum class EExposureDebugDrawMode : uint8
{
	Both,
	ExposedOnly,
	BlockedOnly
};

// Delegate for exposure map updates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExposureMapUpdatedDelegate, const TArray<bool>&, ExposureMap);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MULTITHREAD_API UObserver : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UObserver();

    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // Set the grid component
    void SetGridComponent(const TObjectPtr<UGridGenerator> NewGridComponent);

    // Enable or disable debug drawing
    UFUNCTION(BlueprintCallable, Category = "Observer|Debug")
    void SetDebugDrawing(bool bEnable, float Duration = 0.1f, float Thickness = 1.0f, EExposureDebugDrawMode Mode = EExposureDebugDrawMode::Both);

    // Delegate for when exposure map is updated
    UPROPERTY(BlueprintAssignable, Category = "Observer")
    FOnExposureMapUpdatedDelegate OnExposureMapUpdated;

protected:
    // Called when the game starts
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Reference to the grid component
    UPROPERTY()
    TObjectPtr<UGridGenerator> GridComponent = nullptr;

    // In-flight exposure sweep (one FExposureTraceJob per full grid pass).
    TSharedPtr<FExposureTraceJob> ActiveJob;

    // Handle of the in-flight sweep owned by UDeferredWorkSystem.
    FDeferredJobHandle ActiveBatch;

    // Update exposure map via DeferredJobs time slices (never blocks).
    void UpdateExposureDeferred(int32 NumRaysPerTimeSlice);

    // Fires on sweep completion: broadcasts and recolors grid (no drawing).
    void HandleExposureBatchFinished(const TArray<bool>& FinishedExposure);

    // Fires per gathered slice: draws that slice's rays live.
    void HandleExposureSliceGathered(const TArray<FTraceDebugRecord>& SliceRecords);

    // Draws recorded rays with hit-aware endpoints (never re-reads live state).
    void DrawTraceRecords(const TArray<FTraceDebugRecord>& Records, float Duration) const;

    // Update exposure map using normal approach
    void UpdateExposureMapNormal();

    // Flag to use DeferredJobs time slices (delayed gather) or immediate traces
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Threading")
    bool bUseDeferredJobs = true;

    // Batch progress (Chou TimeSliceBaseIndex) lives in UDeferredWorkSystem now.

    // The percentage of rays to cast per time slice
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Threading", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float PercentageOfRaysPerTimeSlice = 0.01f;

    // Exposure map array
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Observer")
    TArray<bool> Exposure;

    // Debug drawing options
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Debug")
    bool bDrawDebugLines = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Debug", meta = (EditCondition = "bDrawDebugLines"))
    float DebugLineDuration = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Debug", meta = (EditCondition = "bDrawDebugLines"))
    float DebugLineThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Debug", meta = (EditCondition = "bDrawDebugLines"))
    EExposureDebugDrawMode DebugDrawMode = EExposureDebugDrawMode::Both;

    // How many of the current sweep's slices stay visible: line life matches the sweep period.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Debug", meta = (EditCondition = "bDrawDebugLines", ClampMin = "1", ClampMax = "12"))
    int32 DebugTrailSlices = 3;

    // Vertical offset applied to the trace origin (eye height); shared by tracing and drawing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Threading", meta = (ClampMin = "0.0"))
    float TraceHeightOffset = 0.0f;

private:
    // Sweep-period tracking for period-matched trail lifetime (transient, not serialized).
    double LastSubmitSeconds = 0.0;
    double SweepPeriodEMASeconds = 10.0 / 60.0;
    int32 LastSweepSliceCount = 1;

    // Completed sweep count (drives the live progress readout).
    uint32 SweepCount = 0;

    // UI Control widget
    UPROPERTY()
    TObjectPtr<URaysControl> RaysControl = nullptr;

    // URaysControl class reference
    UPROPERTY(EditDefaultsOnly, Category = "Observer|UI")
    TSubclassOf<URaysControl> RaysControlClass;
    
    // Callback for slider value changes
    UFUNCTION()
    void SetRaysPerTimeSlice(float Value);

    void UpdateRaysControlDisplay(int32 RaysPerTimeSlice, int32 TotalRays, int32 Gathered, uint32 Sweep) const;
};
