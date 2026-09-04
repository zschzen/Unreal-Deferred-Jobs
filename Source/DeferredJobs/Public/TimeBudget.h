// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

/**
 * Time budget for one tick's gather/kick work (time slicing).
 * Deadline in seconds via FPlatformTime::Seconds, so slices yield
 * on elapsed time, not on fixed counts or Sleep hacks.
 */
struct FTimeBudget
{
	UE_NODISCARD_CTOR FORCEINLINE explicit FTimeBudget(double InSeconds)
		: DeadlineSeconds(FPlatformTime::Seconds() + InSeconds)
	{
	}

	UE_NODISCARD_CTOR FORCEINLINE bool IsExhausted() const
	{
		return FPlatformTime::Seconds() >= DeadlineSeconds;
	}

private:
	double DeadlineSeconds = 0.0;
};
