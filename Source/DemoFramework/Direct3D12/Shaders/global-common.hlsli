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

#define M_PI          3.1415926535897932384626433832795f
#define M_PI_OVER_2   1.5707963267948966192313216916398f
#define M_PI_OVER_4   0.78539816339744830961566084581988f
#define M_PI_OVER_8   0.39269908169872415480783042290994f
#define M_PI_OVER_16  0.19634954084936207740391521145497f
#define M_PI_OVER_32  0.09817477042468103870195760572748f
#define M_2_PI        6.283185307179586476925286766559f
#define M_4_PI        12.566370614359172953850573533118f
#define M_8_PI        25.132741228718345907701147066236
#define M_16_PI       50.265482457436691815402294132472
#define M_32_PI       100.53096491487338363080458826494
#define M_2_PI_OVER_3 2.0943951023931954923084289221863f
#define M_3_PI_OVER_2 4.7123889803846898576939650749193f
#define M_TAU         M_2_PI
#define M_INV_PI      0.31830988618379067153776752674503f
#define M_INV_TAU     0.15915494309189533576888376337251f
#define M_EPSILON     1.192092896e-07f // Smallest value such that (1.0f +/- M_EPSILON) is not equal to 1.0f.

//---------------------------------------------------------------------------------------------------------------------

#define DF_CUBE_FACE_LEFT   1
#define DF_CUBE_FACE_RIGHT  0
#define DF_CUBE_FACE_TOP    2
#define DF_CUBE_FACE_BOTTOM 3
#define DF_CUBE_FACE_FRONT  4
#define DF_CUBE_FACE_BACK   5

#define DF_CUBE_FACE__COUNT 6

//---------------------------------------------------------------------------------------------------------------------

#ifndef _WIN32

//---------------------------------------------------------------------------------------------------------------------

struct VsOutputViewFill
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

//---------------------------------------------------------------------------------------------------------------------

struct PsOutputStandard
{
	float4 color : SV_TARGET;
};

//---------------------------------------------------------------------------------------------------------------------

struct PointLight
{
	/**
	 * Light position:
	 *  [.xyz] => World position
	 *  [.w]   => Light radius
	 */
	float4 position;

	/**
	 * Light diffuse color:
	 * [.rgb] => Color
	 * [.a]   => Light intensity
	 */
	float4 color;
};

//---------------------------------------------------------------------------------------------------------------------

float CalculateNormalZ(const float2 xy)
{
	return sqrt(1.0f - dot(xy, xy));
}

//---------------------------------------------------------------------------------------------------------------------

struct WorldBasis
{
	float3 normal;
	float3 tangent;
	float3 binormal;
};

//---------------------------------------------------------------------------------------------------------------------

float3 TangentToWorld(const float3 p, const WorldBasis basis)
{
	// Convert the input vector from tangent space to world space.
	return normalize(
		(p.x * basis.tangent) +
		(p.y * basis.binormal) +
		(p.z * basis.normal));
}

//---------------------------------------------------------------------------------------------------------------------

#endif

//---------------------------------------------------------------------------------------------------------------------
