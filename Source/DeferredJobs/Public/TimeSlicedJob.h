// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "JobSlice.h"
#include "TimeBudget.h"

/** Where ExecuteRange may run. Traces stay GameThread in v1 (UWorld is not thread-safe). */
enum class EDeferredJobThread : uint8
{
	GameThread,
	WorkerThread,
};

/**
 * Generic time-sliced job (setup once, Execute per slice,
 * Gather delayed, FinishBatch swaps buffers).
 *
 * Contract:
 * - Snapshot all inputs at construction on the game thread (no UObject
 *   dereference inside WorkerThread ExecuteRange).
 * - ExecuteRange writes only [Base, Base+Count); GatherRange reads only
 *   that window. One slice is ever pending per batch, so no overlap.
 * - GatherRange + OnBatchFinished always run on the game thread.
 */
class DEFERREDJOBS_API ITimeSlicedJob : public TSharedFromThis<ITimeSlicedJob>
{
public:
	virtual ~ITimeSlicedJob() = default;

	virtual int32 GetTotalWork() const = 0;
	virtual EDeferredJobThread GetDesiredThread() const = 0;

	/** Short display name for Insights per-batch rows (no spaces). */
	virtual const TCHAR* GetStatName() const { return TEXT("Job"); }

	virtual void ExecuteRange(const FJobSlice& Slice) = 0;
	virtual void GatherRange(const FJobSlice& Slice, FTimeBudget& Budget) = 0;
	virtual void OnBatchFinished() = 0;
	virtual void Cancel() {}
};
