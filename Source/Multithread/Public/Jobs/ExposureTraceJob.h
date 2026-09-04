// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "HAL/PlatformTime.h"
#include "TimeSlicedJob.h"
#include "Jobs/DoubleBuffer.h"

/** One traced ray, recorded for accurate debug drawing (origin/end as traced). */
struct FTraceDebugRecord
{
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FVector Location = FVector::ZeroVector; // impact point if blocked, else End
	bool bHit = false;
};

DECLARE_DELEGATE_OneParam(FOnExposureBatchFinished, const TArray<bool>& /*Exposure*/);
DECLARE_DELEGATE_OneParam(FOnExposureSliceGathered, const TArray<FTraceDebugRecord>& /*SliceRecords*/);

/**
 * Reference ITimeSlicedJob: exposure sweep over tile locations.
 * Snapshot everything on the game thread at construction; Execute traces
 * one Chou slice, Gather merges it into the back buffer next tick,
 * OnBatchFinished swaps buffers and fires the delegate with the front.
 *
 * v1 runs GameThread-only: UWorld line traces are not thread-safe.
 * A WorkerThread policy would first need AsyncLineTrace or a scene lock.
 */
class MULTITHREAD_API FExposureTraceJob : public ITimeSlicedJob
{
public:
	FExposureTraceJob(UWorld* InWorld, const FVector& InStart, TArray<FVector>&& InEnds,
		const FCollisionQueryParams& InParams, ECollisionChannel InChannel = ECC_Visibility,
		float InTraceHeightOffset = 0.0f);

	// ITimeSlicedJob
	virtual int32 GetTotalWork() const override;
	virtual EDeferredJobThread GetDesiredThread() const override;
	virtual const TCHAR* GetStatName() const override { return TEXT("ExposureSweep"); }
	virtual void ExecuteRange(const FJobSlice& Slice) override;
	virtual void GatherRange(const FJobSlice& Slice, FTimeBudget& Budget) override;
	virtual void OnBatchFinished() override;
	virtual void Cancel() override;

	void SetOnFinished(const FOnExposureBatchFinished& InDelegate) { OnFinished = InDelegate; }
	void SetOnSliceGathered(const FOnExposureSliceGathered& InDelegate) { OnSliceGathered = InDelegate; }

private:
	TWeakObjectPtr<UWorld> World;
	FVector Start = FVector::ZeroVector;
	TArray<FVector> Ends;
	FCollisionQueryParams QueryParams;
	ECollisionChannel Channel = ECC_Visibility;

	TDoubleBuffer<TArray<uint8>> Buffers; // 1 = exposed, 0 = blocked
	TArray<uint8> PendingResults;
	TArray<FHitResult> PendingHits;
	FJobSlice PendingSlice;

	FOnExposureBatchFinished OnFinished;
	FOnExposureSliceGathered OnSliceGathered;

	TAtomic<bool> bCancelled;
};
