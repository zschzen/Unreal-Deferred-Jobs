// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeferredWorkSystem.h"
#include "TimeSlicedJob.h"
#include "DeferredJobsLog.h"
#include "Async/Async.h"
#include "Stats/Stats.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

DECLARE_CYCLE_STAT(TEXT("Tick (all batches)"), STAT_DeferredJobsTick, STATGROUP_DeferredJobs);
DECLARE_CYCLE_STAT(TEXT("Kick slice"), STAT_DeferredJobsKick, STATGROUP_DeferredJobs);
DECLARE_CYCLE_STAT(TEXT("Gather slice"), STAT_DeferredJobsGather, STATGROUP_DeferredJobs);
DECLARE_CYCLE_STAT(TEXT("Finish batch"), STAT_DeferredJobsFinish, STATGROUP_DeferredJobs);
DECLARE_DWORD_COUNTER_STAT(TEXT("Active batches"), STAT_DeferredJobsActiveBatches, STATGROUP_DeferredJobs);
DECLARE_DWORD_COUNTER_STAT(TEXT("Rays gathered"), STAT_DeferredJobsRaysGathered, STATGROUP_DeferredJobs);

FDeferredJobHandle UDeferredWorkSystem::Submit(TSharedPtr<ITimeSlicedJob> Job, int32 NumPerSlice)
{
	check(IsInGameThread());
	FDeferredJobHandle Handle;
	if (!Job.IsValid() || Job->GetTotalWork() <= 0)
	{
		UE_LOG(LogDeferredJobs, Warning, TEXT("Submit rejected: invalid job or empty work"));
		return Handle;
	}

	Handle.Id = NextId++;
	if (NextId == 0)
	{
		NextId = 1; // skip 0, it means invalid
	}

	FActiveBatch Batch;
	Batch.Job = MoveTemp(Job);
	Batch.Handle = Handle;
	Batch.NumPerSlice = FMath::Max(1, NumPerSlice);
	Batch.NextBase = 0;
	Batches.Add(MoveTemp(Batch));
	return Handle;
}

void UDeferredWorkSystem::SetNumPerSlice(FDeferredJobHandle Handle, int32 NumPerSlice)
{
	check(IsInGameThread());
	if (FActiveBatch* Batch = FindBatch(Handle))
	{
		// Applies from the next kick; an already-kicked slice keeps its size.
		Batch->NumPerSlice = FMath::Max(1, NumPerSlice);
	}
}

void UDeferredWorkSystem::Cancel(FDeferredJobHandle Handle)
{
	check(IsInGameThread());
	for (int32 i = Batches.Num() - 1; i >= 0; --i)
	{
		if (Batches[i].Handle == Handle)
		{
			if (Batches[i].Job.IsValid())
			{
				Batches[i].Job->Cancel();
			}
			// Never block: if a worker slice is in flight it finishes on its
			// own copy of Base/Count and its Gather is simply skipped.
			Batches.RemoveAt(i);
			return;
		}
	}
}

bool UDeferredWorkSystem::IsDone(FDeferredJobHandle Handle) const
{
	// Unknown/finished handles read as done (cancelled or completed).
	return FindBatch(Handle) == nullptr;
}

bool UDeferredWorkSystem::GetProgress(FDeferredJobHandle Handle, int32& OutGathered, int32& OutTotal) const
{
	check(IsInGameThread());
	OutGathered = 0;
	OutTotal = 0;
	const FActiveBatch* Batch = FindBatch(Handle);
	if (!Batch || !Batch->Job.IsValid())
	{
		return false;
	}
	OutGathered = Batch->GatheredCount;
	OutTotal = Batch->Job->GetTotalWork();
	return true;
}

bool UDeferredWorkSystem::Poll(FDeferredJobHandle Handle) const
{
	// Tick does the real work; Poll only reports liveness so callers can
	// chain FinishBatch delegates without touching internals.
	return IsDone(Handle);
}

void UDeferredWorkSystem::Deinitialize()
{
	// Drop everything without blocking: in-flight worker lambdas hold only a
	// weak pointer to the job, so they expire harmlessly.
	Batches.Reset();
	Super::Deinitialize();
}

void UDeferredWorkSystem::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_DeferredJobsTick);
	TRACE_CPUPROFILER_EVENT_SCOPE(DeferredJobsTick);
	SET_DWORD_STAT(STAT_DeferredJobsActiveBatches, Batches.Num());
	(void)DeltaTime;
	if (Batches.Num() == 0)
	{
		return;
	}

	FTimeBudget GatherBudget(FMath::Max(0.01, GatherBudgetMs) / 1000.0);
	int32 KicksLeft = FMath::Max(1, MaxKicksPerTick);

	// Backwards by index: TickBatch may RemoveAt(i) on finish.
	for (int32 i = Batches.Num() - 1; i >= 0; --i)
	{
		if (GatherBudget.IsExhausted())
		{
			break;
		}
		if (!Batches.IsValidIndex(i))
		{
			continue;
		}
		TickBatch(Batches[i], GatherBudget, KicksLeft);
	}
}

TStatId UDeferredWorkSystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDeferredWorkSystem, STATGROUP_Tickables);
}

bool UDeferredWorkSystem::IsTickable() const
{
	return !IsTemplate() && Batches.Num() > 0;
}

UWorld* UDeferredWorkSystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

UDeferredWorkSystem::FActiveBatch* UDeferredWorkSystem::FindBatch(FDeferredJobHandle Handle)
{
	check(IsInGameThread());
	for (FActiveBatch& Batch : Batches)
	{
		if (Batch.Handle == Handle)
		{
			return &Batch;
		}
	}
	return nullptr;
}

const UDeferredWorkSystem::FActiveBatch* UDeferredWorkSystem::FindBatch(FDeferredJobHandle Handle) const
{
	for (const FActiveBatch& Batch : Batches)
	{
		if (Batch.Handle == Handle)
		{
			return &Batch;
		}
	}
	return nullptr;
}

bool UDeferredWorkSystem::IsPendingReady(FActiveBatch& Batch)
{
	if (!Batch.bHasPending)
	{
		return true;
	}
	if (Batch.bHasFuture)
	{
		return Batch.PendingFuture.IsReady();
	}
	// Game-thread slices execute at kick time; gather is due next tick.
	return true;
}

void UDeferredWorkSystem::TickBatch(FActiveBatch& Batch, FTimeBudget& GatherBudget, int32& KicksLeft)
{
	if (!Batch.Job.IsValid())
	{
		Cancel(Batch.Handle);
		return;
	}

	// 1. Delayed gather: results of the slice kicked on an earlier tick.
	//    Game-thread slices are always "ready" here because they executed
	//    during a previous tick's kick phase, never the current one: a batch
	//    holds at most one pending slice, and kick is skipped while pending.
	if (Batch.bHasPending && IsPendingReady(Batch))
	{
		SCOPE_CYCLE_COUNTER(STAT_DeferredJobsGather);
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("Gather %s [%d rays]"), Batch.Job->GetStatName(), Batch.PendingSlice.Count));
		Batch.Job->GatherRange(Batch.PendingSlice, GatherBudget);
		INC_DWORD_STAT_BY(STAT_DeferredJobsRaysGathered, Batch.PendingSlice.Count);
		Batch.GatheredCount += Batch.PendingSlice.Count;
		Batch.bHasPending = false;
		Batch.bHasFuture = false;
	}

	if (GatherBudget.IsExhausted())
	{
		return;
	}

	// 2. Tail check: -1 means the last slice was kicked and now gathered.
	if (Batch.NextBase < 0 && !Batch.bHasPending)
	{
		const TSharedPtr<ITimeSlicedJob> FinishedJob = Batch.Job;
		const FDeferredJobHandle FinishedHandle = Batch.Handle;
		
		// Remove first so OnBatchFinished can safely Submit a follow-up sweep.
		Cancel(FinishedHandle);
		{
			SCOPE_CYCLE_COUNTER(STAT_DeferredJobsFinish);
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("Finish %s"), FinishedJob->GetStatName()));
			FinishedJob->OnBatchFinished();
		}
		return;
	}

	// 3. Kick one slice per tick per batch (while global kick budget lasts).
	if (Batch.bHasPending || Batch.NextBase < 0 || KicksLeft <= 0)
	{
		return;
	}

	const int32 Total = Batch.Job->GetTotalWork();
	if (Batch.NextBase >= Total)
	{
		Batch.NextBase = -1;
		return;
	}

	const int32 Count = FMath::Min(Batch.NumPerSlice, Total - Batch.NextBase);
	if (Count <= 0)
	{
		Batch.NextBase = -1;
		return;
	}

	const FJobSlice Slice(Batch.NextBase, Count);
	SCOPE_CYCLE_COUNTER(STAT_DeferredJobsKick);
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("Kick %s [%d rays]"), Batch.Job->GetStatName(), Slice.Count));
	if (Batch.Job->GetDesiredThread() == EDeferredJobThread::WorkerThread)
	{
		TWeakPtr<ITimeSlicedJob> WeakJob = Batch.Job;
		Batch.PendingFuture = Async(EAsyncExecution::ThreadPool, [WeakJob, Slice]()
		{
			if (TSharedPtr<ITimeSlicedJob> Pinned = WeakJob.Pin())
			{
				Pinned->ExecuteRange(Slice);
			}
		});
		Batch.bHasFuture = true;
	}
	else
	{
		// Game-thread work (e.g. physics queries) still splits Execute now /
		// Gather next tick, so the tick that traces never also merges.
		Batch.Job->ExecuteRange(Slice);
		Batch.bHasFuture = false;
	}

	Batch.PendingSlice = Slice;
	Batch.bHasPending = true;
	--KicksLeft;

	Batch.NextBase += Count;
	if (Batch.NextBase >= Total)
	{
		Batch.NextBase = -1; // End-of-batch signal
	}
}
