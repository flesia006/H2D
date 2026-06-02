#include "Perlin.h"
#include "VoxelHash.h"

FPerlin::FPerlin(uint64 Seed)
{
	// Identity table 0..255, then Fisher-Yates shuffle with a seeded PRNG.
	uint8 Perm[256];
	for (int32 i = 0; i < 256; ++i)
	{
		Perm[i] = static_cast<uint8>(i);
	}

	VoxelNoise::FSplitMix Rng(Seed);
	for (int32 i = 255; i > 0; --i)
	{
		const int32 j = static_cast<int32>(Rng.Next() % static_cast<uint64>(i + 1));
		const uint8 Tmp = Perm[i];
		Perm[i] = Perm[j];
		Perm[j] = Tmp;
	}

	// Duplicate so index math never wraps.
	for (int32 i = 0; i < 512; ++i)
	{
		P[i] = Perm[i & 255];
	}
}

float FPerlin::Grad(int32 Hash, float X, float Y)
{
	// 8 gradient directions from the low 3 bits.
	switch (Hash & 7)
	{
		case 0: return  X + Y;
		case 1: return -X + Y;
		case 2: return  X - Y;
		case 3: return -X - Y;
		case 4: return  X;
		case 5: return -X;
		case 6: return  Y;
		default: return -Y;
	}
}

float FPerlin::Noise2D(float X, float Y) const
{
	const int32 Xi = FMath::FloorToInt(X) & 255;
	const int32 Yi = FMath::FloorToInt(Y) & 255;

	const float Xf = X - FMath::FloorToFloat(X);
	const float Yf = Y - FMath::FloorToFloat(Y);

	const float U = Fade(Xf);
	const float V = Fade(Yf);

	const int32 AA = P[P[Xi]     + Yi];
	const int32 AB = P[P[Xi]     + Yi + 1];
	const int32 BA = P[P[Xi + 1] + Yi];
	const int32 BB = P[P[Xi + 1] + Yi + 1];

	const float X1 = Lerp(Grad(AA, Xf, Yf),        Grad(BA, Xf - 1.f, Yf),        U);
	const float X2 = Lerp(Grad(AB, Xf, Yf - 1.f),  Grad(BB, Xf - 1.f, Yf - 1.f),  U);

	return Lerp(X1, X2, V); // ~[-1, 1]
}
