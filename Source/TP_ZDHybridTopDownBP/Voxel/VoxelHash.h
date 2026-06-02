#pragma once

#include "CoreMinimal.h"

// Deterministic hashing for procedural generation. Same inputs -> same output,
// so a single world seed reproduces an identical world.
namespace VoxelNoise
{
	FORCEINLINE uint64 SplitMix64(uint64 X)
	{
		X += 0x9E3779B97F4A7C15ull;
		X = (X ^ (X >> 30)) * 0xBF58476D1CE4E5B9ull;
		X = (X ^ (X >> 27)) * 0x94D049BB133111EBull;
		return X ^ (X >> 31);
	}

	// World seed + coords + a per-layer salt -> a well-mixed 64-bit value.
	FORCEINLINE uint64 DeriveSeed(int64 WorldSeed, int32 X, int32 Y, int32 Z, uint32 LayerSalt)
	{
		uint64 H = static_cast<uint64>(WorldSeed) ^ (static_cast<uint64>(LayerSalt) * 0xD2B74407B1CE6E93ull);
		H = SplitMix64(H ^ static_cast<uint64>(static_cast<uint32>(X)));
		H = SplitMix64(H ^ static_cast<uint64>(static_cast<uint32>(Y)));
		H = SplitMix64(H ^ static_cast<uint64>(static_cast<uint32>(Z)));
		return H;
	}

	// Stateful 64-bit PRNG (SplitMix64 stream). Cheap, deterministic.
	struct FSplitMix
	{
		uint64 State;

		explicit FSplitMix(uint64 Seed) : State(Seed) {}

		FORCEINLINE uint64 Next()
		{
			State += 0x9E3779B97F4A7C15ull;
			uint64 Z = State;
			Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ull;
			Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBull;
			return Z ^ (Z >> 31);
		}
	};
}
