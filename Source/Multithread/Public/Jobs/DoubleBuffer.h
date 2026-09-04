// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/** Double buffer: worker/system writes Back, game reads Front, Swap on batch end. */
template <typename T>
struct TDoubleBuffer
{
	T Front;
	T Back;

	void Swap() { ::Swap(Front, Back); }
	void Reset() { Front.Reset(); Back.Reset(); }
};
