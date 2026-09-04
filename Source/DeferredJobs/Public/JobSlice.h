// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/** Closed range [Base, Base+Count) of a batch — the batch's time-slice window. */
struct FJobSlice
{
	int32 Base = 0;
	int32 Count = 0;

	FJobSlice() = default;
	UE_NODISCARD_CTOR FORCEINLINE FJobSlice(int32 InBase, int32 InCount) : Base(InBase), Count(InCount) {}

	UE_NODISCARD_CTOR FORCEINLINE bool IsValid() const { return Base >= 0 && Count > 0; }
};
