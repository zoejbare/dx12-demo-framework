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

#include "common.hlsli"

//---------------------------------------------------------------------------------------------------------------------

ConstantBuffer<ShProjectRootConstant> rootConst : register(b0, space0);

TextureCube envMap : register(t0, space0);
SamplerState mapSampler : register(s0, space0);

RWStructuredBuffer<ShColorCoefficients> shCoefficients : register(u0, space0);
RWStructuredBuffer<float> shWeights : register(u1, space0);

//---------------------------------------------------------------------------------------------------------------------

[numthreads(DF_REFL_THREAD_COUNT_X, DF_REFL_THREAD_COUNT_Y, 1)]
void ComputeMain(uint2 threadId : SV_DispatchThreadID)
{
	if(threadId.x >= rootConst.edgeLength || threadId.y >= rootConst.edgeLength)
	{
		return;
	}

	const uint2 coord = threadId.xy;

	// Project the left-facing normal and color into a set of spherical harmonic coefficients.
	const float3 normalLeft = CalculateNormalFromPixelCoord(coord, DF_CUBE_FACE_LEFT, rootConst.invEdgeLength);
	const float3 colorLeft = envMap.SampleLevel(mapSampler, normalLeft, 0).xyz;
	const ShColorCoefficientsWithWeight coeffLeft = ProjectColorToSphericalHarmonics(coord, normalLeft, colorLeft, rootConst.invEdgeLength);

	// Project the right-facing normal and color into a set of spherical harmonic coefficients.
	const float3 normalRight = CalculateNormalFromPixelCoord(coord, DF_CUBE_FACE_RIGHT, rootConst.invEdgeLength);
	const float3 colorRight = envMap.SampleLevel(mapSampler, normalRight, 0).xyz;
	const ShColorCoefficientsWithWeight coeffRight = ProjectColorToSphericalHarmonics(coord, normalRight, colorRight, rootConst.invEdgeLength);

	// Project the top-facing normal and color into a set of spherical harmonic coefficients.
	const float3 normalTop = CalculateNormalFromPixelCoord(coord, DF_CUBE_FACE_TOP, rootConst.invEdgeLength);
	const float3 colorTop = envMap.SampleLevel(mapSampler, normalTop, 0).xyz;
	const ShColorCoefficientsWithWeight coeffTop = ProjectColorToSphericalHarmonics(coord, normalTop, colorTop, rootConst.invEdgeLength);

	// Project the bottom-facing normal and color into a set of spherical harmonic coefficients.
	const float3 normalBottom = CalculateNormalFromPixelCoord(coord, DF_CUBE_FACE_BOTTOM, rootConst.invEdgeLength);
	const float3 colorBottom = envMap.SampleLevel(mapSampler, normalBottom, 0).xyz;
	const ShColorCoefficientsWithWeight coeffBottom = ProjectColorToSphericalHarmonics(coord, normalBottom, colorBottom, rootConst.invEdgeLength);

	// Project the front-facing normal and color into a set of spherical harmonic coefficients.
	const float3 normalFront = CalculateNormalFromPixelCoord(coord, DF_CUBE_FACE_FRONT, rootConst.invEdgeLength);
	const float3 colorFront = envMap.SampleLevel(mapSampler, normalFront, 0).xyz;
	const ShColorCoefficientsWithWeight coeffFront = ProjectColorToSphericalHarmonics(coord, normalFront, colorFront, rootConst.invEdgeLength);

	// Project the back-facing normal and color into a set of spherical harmonic coefficients.
	const float3 normalBack = CalculateNormalFromPixelCoord(coord, DF_CUBE_FACE_BACK, rootConst.invEdgeLength);
	const float3 colorBack = envMap.SampleLevel(mapSampler, normalBack, 0).xyz;
	const ShColorCoefficientsWithWeight coeffBack = ProjectColorToSphericalHarmonics(coord, normalBack, colorBack, rootConst.invEdgeLength);

	// Calculate the output index for the coefficient data.
	const uint shIndex = ((coord.y * rootConst.edgeLength) + coord.x);

	// Store the sum of texels at the current coordinate across each face to the coefficient UAV.
	[unroll]
	for(uint i = 0; i < DF_SH_COEFF_COUNT; ++i)
	{
		shCoefficients[shIndex].value[i]
			= coeffLeft.value[i]
			+ coeffRight.value[i]
			+ coeffTop.value[i]
			+ coeffBottom.value[i]
			+ coeffFront.value[i]
			+ coeffBack.value[i];
	}

	// Also sum the weights used for the texels at each face. There might be an optimization opportunity here since
	// this sum value will likely always be exactly the same for all texels in a single compute thread, meaning we
	// could probably calculate the weight once, then multiply by 6 to get the initial summed weight.
	shWeights[shIndex]
		= coeffLeft.weight
		+ coeffRight.weight
		+ coeffTop.weight
		+ coeffBottom.weight
		+ coeffFront.weight
		+ coeffBack.weight;
}

//---------------------------------------------------------------------------------------------------------------------