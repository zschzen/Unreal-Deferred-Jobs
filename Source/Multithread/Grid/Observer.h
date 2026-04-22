// Observer.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/SharedPointer.h"
#include "Observer.generated.h"

class UGridGenerator;
// Forward declarations
class UGridComponent;
class URaysControl;
class ULineTraceWorker;

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
    void SetDebugDrawing(bool bEnable, float Duration = 0.1f, float Thickness = 1.0f);

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

    // Worker for threaded line traces
    TSharedPtr<ULineTraceWorker> Worker;

    // Update exposure map using runnable threads
    void UpdateExposureMapRunnable(int32 NumRaysPerTimeSlice);

    // Update exposure map using normal approach
    void UpdateExposureMapNormal();

    // Flag to use FRunnable or not
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Threading")
    bool bUseFRunnable = true;

    // Time slice base index for progressive raytracing
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Observer|Threading")
    int32 TimeSliceBaseIndex = 0;

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

private:
    // UI Control widget
    UPROPERTY()
    TObjectPtr<URaysControl> RaysControl = nullptr;

    // URaysControl class reference
    UPROPERTY(EditDefaultsOnly, Category = "Observer|UI")
    TSubclassOf<URaysControl> RaysControlClass;
    
    // Callback for slider value changes
    UFUNCTION()
    void SetRaysPerTimeSlice(float Value);

    void UpdateRaysControlDisplay(int32 RaysPerTimeSlice, int32 TotalRays) const;
};
