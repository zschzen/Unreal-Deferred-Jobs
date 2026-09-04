// Copyright Epic Games, Inc. All Rights Reserved.

#include "Jobs/ExposureTraceJob.h"
#include "Engine/World.h"

FExposureTraceJob::FExposureTraceJob(UWorld* InWorld, const FVector& InStart, TArray<FVector>&& InEnds,
	const FCollisionQueryParams& InParams, ECollisionChannel InChannel, float InTraceHeightOffset)
	: World(InWorld)
	, Start(InStart + FVector(0.0, 0.0, InTraceHeightOffset))
	, Ends(MoveTemp(InEnds))
	, QueryParams(InParams)
	, Channel(InChannel)
{
	bCancelled.Store(false);
	Buffers.Back.Init(0, Ends.Num());
	Buffers.Front.Init(0, Ends.Num());
}

int32 FExposureTraceJob::GetTotalWork() const
{
	return Ends.Num();
}

EDeferredJobThread FExposureTraceJob::GetDesiredThread() const
{
	return EDeferredJobThread::GameThread;
}

void FExposureTraceJob::ExecuteRange(const FJobSlice& Slice)
{
	PendingResults.Reset();
	PendingHits.Reset();
	if (bCancelled.Load())
	{
		return;
	}

	UWorld* PinnedWorld = World.Get();
	if (!PinnedWorld || Slice.Base < 0 || Slice.Count <= 0)
	{
		return;
	}

	const int32 EndIndex = FMath::Min(Slice.Base + Slice.Count, Ends.Num());
	const int32 ActualCount = EndIndex - Slice.Base;
	if (ActualCount <= 0)
	{
		return;
	}

	PendingResults.AddUninitialized(ActualCount);
	PendingHits.AddUninitialized(ActualCount);
	for (int32 i = 0; i < ActualCount; ++i)
	{
		const FVector& End = Ends[Slice.Base + i];
		FHitResult& Hit = PendingHits[i];
		const bool bHit = PinnedWorld->LineTraceSingleByChannel(Hit, Start, End, Channel, QueryParams);
		PendingResults[i] = bHit ? 0 : 1;
	}
	PendingSlice = FJobSlice(Slice.Base, ActualCount);
}

void FExposureTraceJob::GatherRange(const FJobSlice& Slice, FTimeBudget& Budget)
{
	(void)Slice;
	// Merge is a bounded O(NumPerSlice) copy; always completes so the
	// pipeline advances even under a spent gather budget.
	(void)Budget;
	if (bCancelled.Load() || PendingResults.Num() == 0)
	{
		return;
	}
	if (!Buffers.Back.IsValidIndex(PendingSlice.Base))
	{
		PendingResults.Reset();
		PendingHits.Reset();
		return;
	}
	const int32 CopyCount = FMath::Min(PendingResults.Num(), Buffers.Back.Num() - PendingSlice.Base);
	FMemory::Memcpy(Buffers.Back.GetData() + PendingSlice.Base, PendingResults.GetData(), CopyCount);

	// Report what was actually traced so the game can draw it live.
	if (OnSliceGathered.IsBound() && PendingHits.Num() > 0)
	{
		TArray<FTraceDebugRecord> SliceRecords;
		SliceRecords.Reserve(PendingHits.Num());
		for (int32 i = 0; i < PendingHits.Num(); ++i)
		{
			const int32 GlobalIndex = PendingSlice.Base + i;
			if (!Ends.IsValidIndex(GlobalIndex))
			{
				break;
			}
			const FHitResult& Hit = PendingHits[i];
			FTraceDebugRecord Record;
			Record.Start = Start;
			Record.End = Ends[GlobalIndex];
			Record.bHit = Hit.bBlockingHit;
			Record.Location = Record.End;
			if (Hit.bBlockingHit)
			{
				Record.Location = Hit.Location;
			}
			SliceRecords.Add(Record);
		}
		OnSliceGathered.Execute(SliceRecords);
	}

	PendingResults.Reset();
	PendingHits.Reset();
}

void FExposureTraceJob::OnBatchFinished()
{
	Buffers.Swap();
	if (OnFinished.IsBound())
	{
		TArray<bool> Exposure;
		Exposure.Init(false, Buffers.Front.Num());
		for (int32 i = 0; i < Buffers.Front.Num(); ++i)
		{
			Exposure[i] = Buffers.Front[i] != 0;
		}
		OnFinished.Execute(Exposure);
	}
}

void FExposureTraceJob::Cancel()
{
	bCancelled.Store(true);
}
