#pragma once

#include "CoreMinimal.h"

// Ken Perlin "improved noise" (2002), seeded permutation table.
// Immutable after construction -> safe to sample from multiple threads.
class FPerlin
{
public:
	explicit FPerlin(uint64 Seed);

	// Both return roughly [-1, 1].
	float Noise2D(float X, float Y) const;
	float Noise3D(float X, float Y, float Z) const;

private:
	uint8 P[512];

	static FORCEINLINE float Fade(float T) { return T * T * T * (T * (T * 6.f - 15.f) + 10.f); }
	static FORCEINLINE float Lerp(float A, float B, float T) { return A + T * (B - A); }
	static float Grad(int32 Hash, float X, float Y);
	static float Grad3(int32 Hash, float X, float Y, float Z);
};
