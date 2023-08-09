//
// Copyright (c) 2023, Zoe J. Bare
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
#pragma once

//---------------------------------------------------------------------------------------------------------------------

#include "../global-common.hlsli"

//---------------------------------------------------------------------------------------------------------------------

#define DF_REFL_THREAD_COUNT_X 32
#define DF_REFL_THREAD_COUNT_Y 32

#define DF_REFL_SH_LINEAR_THREAD_COUNT 1024

#define DF_SH_COEFF_COUNT 9

#define DF_SH_REDUCE_SEGMENT_SIZE 4

#define DF_SH_CONST_Y00  0.28209479177387814347403972578039f // sqrt(1 / 4pi)
#define DF_SH_CONST_Y1_1 0.48860251190291992158638462283835f // sqrt(3 / 4pi)
#define DF_SH_CONST_Y10  0.48860251190291992158638462283835f // sqrt(3 / 4pi)
#define DF_SH_CONST_Y11  0.48860251190291992158638462283835f // sqrt(3 / 4pi)
#define DF_SH_CONST_Y2_2 1.0925484305920790705433857058027f // sqrt(15 / 4pi)
#define DF_SH_CONST_Y2_1 1.0925484305920790705433857058027f // sqrt(15 / 4pi)
#define DF_SH_CONST_Y20  0.31539156525252000603089369029571f // sqrt(5 / 16pi)
#define DF_SH_CONST_Y21  1.0925484305920790705433857058027f // sqrt(15 / 4pi)
#define DF_SH_CONST_Y22  0.54627421529603953527169285290134f // sqrt(15 / 16pi)

#define DF_SH_COSINE_LOBE_A0 M_PI
#define DF_SH_COSINE_LOBE_A1 M_2_PI_OVER_3
#define DF_SH_COSINE_LOBE_A2 M_PI_OVER_4

//---------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32
	#define uint uint32_t

	struct float3
	{
		float32_t x, y, z;
	};
#endif

//---------------------------------------------------------------------------------------------------------------------

struct EquiToCubeRootConstant
{
	uint mipIndex;
	uint faceIndex;
	uint edgeLength;
	float invEdgeLength;
};

struct ClearRootConstant
{
	uint edgeLength;
};

struct ShProjectRootConstant
{
	uint edgeLength;
	float invEdgeLength;
};

struct ShReduceRootConstant
{
	uint headIndex;
	uint tailIndex;
};

struct ShNormalizeRootConstant
{
	uint index;
};

struct ShReconstructRootConstant
{
	uint faceIndex;
	uint coeffIndex;
	uint edgeLength;
	float invEdgeLength;
};

struct ShColorCoefficients
{
	// SH coefficients for a given normal on the sphere.
	float3 value[DF_SH_COEFF_COUNT];
};

//---------------------------------------------------------------------------------------------------------------------

#ifndef _WIN32

//---------------------------------------------------------------------------------------------------------------------

struct ShColorCoefficientsWithWeight
{
	// SH coefficients for a given normal on the sphere.
	float3 value[DF_SH_COEFF_COUNT];

	float weight;
};

//---------------------------------------------------------------------------------------------------------------------

float2 CalculateEquirectUvFromNormal(const float3 normal)
{
	// Convert the input normal into a 2D cartesian coordinate.
	float2 uv = float2(atan2(normal.x, normal.z), asin(normal.y));

	uv *= float2(M_INV_TAU, M_INV_PI);
	uv += (float2)0.5f;
	uv.y = 1.0f - uv.y;

	return uv;
}

//---------------------------------------------------------------------------------------------------------------------

float3 CalculateNormalFromPixelCoord(const uint2 coord, const uint faceIndex, const float invSize)
{
	// Add the half texel offset when computing the normalized UV directions.
	// The easy way to think of the justification for this is to imagine a 1x1
	// cube map face.  If we were to use the incoming texture coordinate of
	// (0, 0) without the half texel offset, the math here which adjusts the
	// coordinate from [0, size] on both axes to [-1, 1] on both axes, the result
	// would have the coordinates pointing to the upper left corner of the face.
	// With a 1x1 cube face texture, we want the resulting coordinate to be
	// (0, 0) so the final normal vector is facing straight into the middle
	// of the texture.
	float2 uv = (float2((float)coord.x + 0.5f, (float)coord.y + 0.5f) * invSize * 2.0f) - 1.0f;
	float3 normal;

	// The texture coordinates start in the upper left which means they increase
	// downward to the right. Since the Y axis increases in the upward direction,
	// we need to flip the V coordinate to get a normal which is not upside down.
	uv.y *= -1.0f;

	switch(faceIndex)
	{
		case DF_CUBE_FACE_LEFT:   normal = float3(-1.0f, uv.y, uv.x);  break;
		case DF_CUBE_FACE_RIGHT:  normal = float3(1.0f, uv.y, -uv.x);  break;
		case DF_CUBE_FACE_TOP:    normal = float3(uv.x, 1.0f, -uv.y);  break;
		case DF_CUBE_FACE_BOTTOM: normal = float3(uv.x, -1.0f, uv.y);  break;
		case DF_CUBE_FACE_FRONT:  normal = float3(uv.x, uv.y, 1.0f);   break;
		case DF_CUBE_FACE_BACK:   normal = float3(-uv.x, uv.y, -1.0f); break;

		default:
			return 0.0f;
	}

	return normalize(normal);
}

//---------------------------------------------------------------------------------------------------------------------

struct ShBasis
{
	float value[DF_SH_COEFF_COUNT];
};

//---------------------------------------------------------------------------------------------------------------------

ShBasis CalculateShProjectionBasis(const float3 normal)
{
	ShBasis output;

	// Band 0
	output.value[0] = DF_SH_CONST_Y00;

	// Band 1
	output.value[1] = DF_SH_CONST_Y1_1 * normal.y;
	output.value[2] = DF_SH_CONST_Y10 * normal.z;
	output.value[3] = DF_SH_CONST_Y11 * normal.x;

	// Band 2
	output.value[4] = DF_SH_CONST_Y2_2 * normal.y * normal.x;
	output.value[5] = DF_SH_CONST_Y2_1 * normal.y * normal.z;
	output.value[6] = DF_SH_CONST_Y20 * (3.0f * normal.z * normal.z - 1.0f);
	output.value[7] = DF_SH_CONST_Y21 * normal.x * normal.z;
	output.value[8] = DF_SH_CONST_Y22 * (normal.x * normal.x - normal.y * normal.y);

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

ShBasis CalculateShReconstructionBasis(const float3 normal)
{
	ShBasis output;

	// Band 0
	output.value[0] = DF_SH_CONST_Y00 * DF_SH_COSINE_LOBE_A0;

	// Band 1
	output.value[1] = DF_SH_CONST_Y1_1 * DF_SH_COSINE_LOBE_A1 * normal.y;
	output.value[2] = DF_SH_CONST_Y10 * DF_SH_COSINE_LOBE_A1 * normal.z;
	output.value[3] = DF_SH_CONST_Y11 * DF_SH_COSINE_LOBE_A1 * normal.x;

	// Band 2
	output.value[4] = DF_SH_CONST_Y2_2 * DF_SH_COSINE_LOBE_A2 * normal.y * normal.x;
	output.value[5] = DF_SH_CONST_Y2_1 * DF_SH_COSINE_LOBE_A2 * normal.y * normal.z;
	output.value[6] = DF_SH_CONST_Y20 * DF_SH_COSINE_LOBE_A2 * (3.0f * normal.z * normal.z - 1.0f);
	output.value[7] = DF_SH_CONST_Y21 * DF_SH_COSINE_LOBE_A2 * normal.x * normal.z;
	output.value[8] = DF_SH_CONST_Y22 * DF_SH_COSINE_LOBE_A2 * (normal.x * normal.x - normal.y * normal.y);

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

ShColorCoefficientsWithWeight ProjectColorToSphericalHarmonics(
	const uint2 coord,
	const float3 normal,
	const float3 color,
	const float invSize)
{
	// Transform the face coordinate from the range [0, edgeLength) to (-1, +1).
	const float2 uv = (float2((float)coord.x + 0.5f, (float)coord.y + 0.5f) * invSize * 2.0f) - 1.0f;
	const float tmp = 1.0f + (uv.x * uv.x) + (uv.y * uv.y);

	// The differential solid angle for the current texel.
	const float diffAngle = 4.0f / (sqrt(tmp) * tmp);

	// Calculate the spherical harmonic coefficient basis for the current normal.
	const ShBasis shBasis = CalculateShProjectionBasis(normal);

	// Precompute the weighted color value only once instead of once per basis function.
	const float3 weightedColor = color * diffAngle;

	ShColorCoefficientsWithWeight output;
	output.weight = diffAngle;

	// Calculate the SH color coefficients for the color value at the given normal and store them in the output buffer.
	output.value[0] = weightedColor * shBasis.value[0];
	output.value[1] = weightedColor * shBasis.value[1];
	output.value[2] = weightedColor * shBasis.value[2];
	output.value[3] = weightedColor * shBasis.value[3];
	output.value[4] = weightedColor * shBasis.value[4];
	output.value[5] = weightedColor * shBasis.value[5];
	output.value[6] = weightedColor * shBasis.value[6];
	output.value[7] = weightedColor * shBasis.value[7];
	output.value[8] = weightedColor * shBasis.value[8];

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

float3 ReconstructColorFromSphericalHarmonics(const ShColorCoefficients colorCoeff, const float3 normal)
{
	// Calculate the SH basis needed for color reconstruction.
	const ShBasis shBasis = CalculateShReconstructionBasis(normal);

	// Reconstruct the color from the computed SH coefficients.
	const float3 output
		// Band 0
		= (colorCoeff.value[0].xyz * shBasis.value[0])

		// Band 1
		+ (colorCoeff.value[1].xyz * shBasis.value[1])
		+ (colorCoeff.value[2].xyz * shBasis.value[2])
		+ (colorCoeff.value[3].xyz * shBasis.value[3])

		// Band 2
		+ (colorCoeff.value[4].xyz * shBasis.value[4])
		+ (colorCoeff.value[5].xyz * shBasis.value[5])
		+ (colorCoeff.value[6].xyz * shBasis.value[6])
		+ (colorCoeff.value[7].xyz * shBasis.value[7])
		+ (colorCoeff.value[8].xyz * shBasis.value[8]);

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

#endif

//---------------------------------------------------------------------------------------------------------------------