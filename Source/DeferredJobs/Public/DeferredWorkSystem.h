// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Async/Future.h"
#include "JobHandle.h"
#include "JobSlice.h"
#include "TimeBudget.h"

DECLARE_STATS_GROUP(TEXT("DeferredJobs"), STATGROUP_DeferredJobs, STATCAT_Advanced);

#include "DeferredWorkSystem.generated.h"

class ITimeSlicedJob;

/**
 * Owns all active batches. Per tick, per batch (kick, execute, delayed gather, finish):
 *  1. Gather the previously kicked slice if ready (never blocks).
 *  2. If NextBase == -1 and nothing pending, FinishBatch + remove.
 *  3. Else kick one slice: trim excess at the tail, run Execute inline
 *     (GameThread) or via ThreadPool (WorkerThread), advance NextBase,
 *     set -1 when the tail is reached. The gather lands next tick.
 */
UCLASS(config=Game)
class DEFERREDJOBS_API UDeferredWorkSystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** ms of GatherRange work allowed per tick across all batches. */
	UPROPERTY(EditAnywhere, Config, Category = "DeferredJobs")
	float GatherBudgetMs = 0.5f;

	/** Max slice kicks per tick across all batches. */
	UPROPERTY(EditAnywhere, Config, Category = "DeferredJobs", meta = (ClampMin = "1"))
	int32 MaxKicksPerTick = 4;

	UE_NODISCARD_CTOR FDeferredJobHandle Submit(TSharedPtr<ITimeSlicedJob> Job, int32 NumPerSlice);
	void SetNumPerSlice(FDeferredJobHandle Handle, int32 NumPerSlice);
	void Cancel(FDeferredJobHandle Handle);
	UE_NODISCARD_CTOR bool IsDone(FDeferredJobHandle Handle) const;
	UE_NODISCARD_CTOR bool Poll(FDeferredJobHandle Handle) const;
	UE_NODISCARD_CTOR bool GetProgress(FDeferredJobHandle Handle, int32& OutGathered, int32& OutTotal) const;

	// USubsystem
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

private:
	struct FActiveBatch
	{
		TSharedPtr<ITimeSlicedJob> Job;
		FDeferredJobHandle Handle;
		int32 NumPerSlice = 1;
		int32 NextBase = 0; // -1 signals tail kicked; finish once pending drains
		int32 GatheredCount = 0; // rays merged so far (live progress, game thread only)
		bool bHasPending = false;
		FJobSlice PendingSlice;
		TFuture<void> PendingFuture;
		bool bHasFuture = false;
	};

	TArray<FActiveBatch> Batches;
	uint32 NextId = 1;

	FActiveBatch* FindBatch(FDeferredJobHandle Handle);
	const FActiveBatch* FindBatch(FDeferredJobHandle Handle) const;
	void TickBatch(FActiveBatch& Batch, FTimeBudget& GatherBudget, int32& KicksLeft);
	
	static bool IsPendingReady(FActiveBatch& Batch);
};
