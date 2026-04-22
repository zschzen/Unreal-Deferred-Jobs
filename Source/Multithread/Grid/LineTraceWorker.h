#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/Thread.h"
#include "Async/TaskGraphInterfaces.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"

class MULTITHREAD_API ULineTraceWorker : public FRunnable
{
public:
    /** Delegate for when the trace work has completed */
    DECLARE_DELEGATE_OneParam(FOnTraceWorkCompleted, const TArray<FHitResult>&);

    ULineTraceWorker(const FVector& Start, const TArray<FVector>& End, UWorld* World,
                    const TArray<AActor*>& IgnoredActors, const TArray<UPrimitiveComponent*>& IgnoredComponents,
                    int32 TimeSliceBaseIndex, int32 NumRaysPerTimeSlice)
        : StartLocation(Start)
        , EndLocations(End)
        , World(World)
        , IgnoredActors(IgnoredActors)
        , TimeSliceBaseIndex(TimeSliceBaseIndex)
        , NumRaysPerTimeSlice(NumRaysPerTimeSlice)
        , bIsWorkDone(false)
    {
        // Ensure we don't go out of bounds
        ensureMsgf(TimeSliceBaseIndex + NumRaysPerTimeSlice <= End.Num(), 
                 TEXT("LineTraceWorker time slice out of bounds! Base: %d, Count: %d, Total: %d"), 
                 TimeSliceBaseIndex, NumRaysPerTimeSlice, End.Num());

        // Initialize results array to proper size
        HitResults.Init(FHitResult(), NumRaysPerTimeSlice);
        ExposureResults.Init(false, NumRaysPerTimeSlice);

        for (UPrimitiveComponent* IgnoredComponent : IgnoredComponents)
        {
            if (IgnoredComponent)
            {
                IgnoredTraceComponents.Add(IgnoredComponent);
            }
        }
        
        // Create the thread with a proper name and appropriate priority
        // Using TPri_Normal which is better for most workloads than TimeCritical
        Thread = FRunnableThread::Create(this, *FString::Printf(TEXT("LineTraceWorker_%d"), FPlatformTLS::GetCurrentThreadId()), 
                                        0, TPri_Normal, FPlatformAffinity::GetPoolThreadMask());
    }

    virtual ~ULineTraceWorker() override
    {
        // Ensure thread is properly shut down
        if (Thread)
        {
            // Signal the thread to stop
            ULineTraceWorker::Stop();
            
            // Wait for thread to complete
            Thread->WaitForCompletion();
            
            delete Thread;
            Thread = nullptr;
        }
    }

    virtual bool Init() override
    {
        // Verify we have valid data to work with
        return (World != nullptr && EndLocations.Num() > 0);
    }

    virtual uint32 Run() override
    {
        // Check if we should stop
        if (StopTaskCounter.GetValue() > 0)
        {
            return 0;
        }

        // Perform the line traces for this time slice
        FCollisionQueryParams Params;
        Params.bReturnPhysicalMaterial = false;
        Params.AddIgnoredActors(IgnoredActors);
        Params.AddIgnoredComponents(IgnoredTraceComponents);
        Params.bTraceComplex = false;  // More accurate but slower trace against triangles rather than simple shapes

        // Use local copies of class members to avoid data races
        const FVector LocalStart = StartLocation;
        const int32 LocalTimeSliceBaseIndex = TimeSliceBaseIndex;
        const int32 LocalNumRaysPerTimeSlice = NumRaysPerTimeSlice;
        UWorld* LocalWorld = World;
        
        if (!LocalWorld || StopTaskCounter.GetValue() > 0)
        {
            return 0;
        }

        // Perform traces with a configurable batch size for better performance
        const int32 BatchSize = FMath::Min(32, LocalNumRaysPerTimeSlice);
        
        for (int32 i = 0; i < LocalNumRaysPerTimeSlice; i += BatchSize)
        {
            // Check if we should stop before each batch
            if (StopTaskCounter.GetValue() > 0)
            {
                break;
            }

            // Process this batch
            const int32 CurrentBatchSize = FMath::Min(BatchSize, LocalNumRaysPerTimeSlice - i);
            for (int32 j = 0; j < CurrentBatchSize; ++j)
            {
                const int32 RayIndex = LocalTimeSliceBaseIndex + i + j;
                if (RayIndex >= EndLocations.Num())
                {
                    break;
                }

                const FVector& End = EndLocations[RayIndex];
                const int32 ResultIndex = i + j;

                if (ResultIndex < HitResults.Num())
                {
                    // Store index to keep track of original request order
                    HitResults[ResultIndex].TraceStart = LocalStart;
                    HitResults[ResultIndex].TraceEnd = End;

                    // Perform the actual line trace
                    const bool bHit = LocalWorld->LineTraceSingleByChannel(HitResults[ResultIndex], 
                                                                        LocalStart, End, 
                                                                        ECC_Visibility, 
                                                                        Params);
                    
                    ExposureResults[ResultIndex] = !bHit;
                }
            }

            // Add a small sleep between batches to avoid hogging the thread
            FPlatformProcess::Sleep(0.0001f);
        }

        // Safely mark work as done
        FScopeLock Lock(&CriticalSection);
        bIsWorkDone = true;

        return 0;
    }

    virtual void Stop() override
    {
        StopTaskCounter.Increment();
    }

    virtual void Exit() override
    {
        // Clean up any resources if needed
    }

    /** Get the thread object */
    FORCEINLINE FRunnableThread* GetThread() const
    {
        return Thread;
    }

    /** Set the completion delegate */
    void SetCompletionDelegate(const FOnTraceWorkCompleted& Delegate)
    {
        OnTraceWorkCompleted = Delegate;
    }

    /** Check if work is done */
    FORCEINLINE bool IsWorkDone() const
    {
        FScopeLock Lock(&CriticalSection);
        return bIsWorkDone;
    }

    /** Poll for results - should be called on game thread */
    void PollForCompletion()
    {
        if (IsWorkDone() && !bResultsGathered)
        {
            // Atomically set flag to ensure we only gather results once
            bResultsGathered = true;
            
            // Fire completion delegate
            if (OnTraceWorkCompleted.IsBound())
            {
                OnTraceWorkCompleted.Execute(HitResults);
            }
        }
    }

    /** Get hit results - only valid after work is done */
    FORCEINLINE TArray<FHitResult> GetHitResults() const 
    { 
        FScopeLock Lock(&CriticalSection);
        return HitResults; 
    }

    /** Get exposure results (simple hit/no hit) - only valid after work is done */
    FORCEINLINE TArray<bool> GetExposureResults() const 
    { 
        FScopeLock Lock(&CriticalSection);
        return ExposureResults; 
    }

private:
    // Input parameters
    FVector StartLocation;
    TArray<FVector> EndLocations;
    UWorld* World;
    TArray<AActor*> IgnoredActors;
    TArray<TWeakObjectPtr<UPrimitiveComponent>> IgnoredTraceComponents;
    int32 TimeSliceBaseIndex;
    int32 NumRaysPerTimeSlice;
    
    // Threading
    FRunnableThread* Thread;
    FThreadSafeCounter StopTaskCounter;
    mutable FCriticalSection CriticalSection;
    
    // Results
    TArray<FHitResult> HitResults;
    TArray<bool> ExposureResults;
    
    // State tracking
    bool bIsWorkDone;
    bool bResultsGathered = false;
    
    // Completion notification
    FOnTraceWorkCompleted OnTraceWorkCompleted;
};
