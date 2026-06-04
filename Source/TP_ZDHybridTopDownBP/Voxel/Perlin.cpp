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

float FPerlin::Grad3(int32 Hash, float X, float Y, float Z)
{
	const int32 H = Hash & 15;
	const float U = H < 8 ? X : Y;
	const float V = H < 4 ? Y : ((H == 12 || H == 14) ? X : Z);
	return ((H & 1) ? -U : U) + ((H & 2) ? -V : V);
}

float FPerlin::Noise3D(float X, float Y, float Z) const
{
	const int32 Xi = FMath::FloorToInt(X) & 255;
	const int32 Yi = FMath::FloorToInt(Y) & 255;
	const int32 Zi = FMath::FloorToInt(Z) & 255;

	const float Xf = X - FMath::FloorToFloat(X);
	const float Yf = Y - FMath::FloorToFloat(Y);
	const float Zf = Z - FMath::FloorToFloat(Z);

	const float U = Fade(Xf);
	const float V = Fade(Yf);
	const float W = Fade(Zf);

	const int32 A  = P[Xi]     + Yi;
	const int32 AA = P[A]      + Zi;
	const int32 AB = P[A + 1]  + Zi;
	const int32 B  = P[Xi + 1] + Yi;
	const int32 BA = P[B]      + Zi;
	const int32 BB = P[B + 1]  + Zi;

	const float Y1 = Lerp(
		Lerp(Grad3(P[AA],     Xf,       Yf,       Zf),
		     Grad3(P[BA],     Xf - 1.f, Yf,       Zf), U),
		Lerp(Grad3(P[AB],     Xf,       Yf - 1.f, Zf),
		     Grad3(P[BB],     Xf - 1.f, Yf - 1.f, Zf), U), V);

	const float Y2 = Lerp(
		Lerp(Grad3(P[AA + 1], Xf,       Yf,       Zf - 1.f),
		     Grad3(P[BA + 1], Xf - 1.f, Yf,       Zf - 1.f), U),
		Lerp(Grad3(P[AB + 1], Xf,       Yf - 1.f, Zf - 1.f),
		     Grad3(P[BB + 1], Xf - 1.f, Yf - 1.f, Zf - 1.f), U), V);

	return Lerp(Y1, Y2, W); // ~[-1, 1]
}
