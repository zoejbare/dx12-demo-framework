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
#define M_2_PI        6.283185307179586476925286766559
#define M_2_PI_OVER_3 2.0943951023931954923084289221863
#define M_3_PI_OVER_2 4.7123889803846898576939650749193
#define M_INV_PI      0.31830988618379067153776752674503f
#define M_TAU         M_2_PI
#define M_EPSILON     1.192092896e-07f // Smallest value such that (1.0f +/- M_EPSILON) is not equal to 1.0f.

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
