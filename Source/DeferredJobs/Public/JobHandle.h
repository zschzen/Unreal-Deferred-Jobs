// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/** Opaque handle to a submitted batch. Id 0 is never valid. */
struct FDeferredJobHandle
{
	uint32 Id = 0;

	UE_NODISCARD_CTOR FORCEINLINE bool IsValid() const { return Id != 0; }
	FORCEINLINE bool operator==(const FDeferredJobHandle& Other) const { return Id == Other.Id; }
	FORCEINLINE friend uint32 GetTypeHash(const FDeferredJobHandle& H) { return H.Id; }
};
