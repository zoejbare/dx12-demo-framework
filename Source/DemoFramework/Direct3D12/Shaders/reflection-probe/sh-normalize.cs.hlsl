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

ConstantBuffer<ShNormalizeRootConstant> rootConst : register(b0, space0);
StructuredBuffer<float> shWeights : register(t0, space0);

RWStructuredBuffer<ShColorCoefficients> shCoefficients : register(u0, space0);

//---------------------------------------------------------------------------------------------------------------------

[numthreads(1, 1, 1)]
void ComputeMain(uint threadId : SV_DispatchThreadID)
{
	ShColorCoefficients finalCoeff = shCoefficients[rootConst.index];

	const float normalizationFactor = M_4_PI / shWeights[rootConst.index];

	// Normalize the final coefficient sums.
	finalCoeff.value[0] *= normalizationFactor;
	finalCoeff.value[1] *= normalizationFactor;
	finalCoeff.value[2] *= normalizationFactor;
	finalCoeff.value[3] *= normalizationFactor;
	finalCoeff.value[4] *= normalizationFactor;
	finalCoeff.value[5] *= normalizationFactor;
	finalCoeff.value[6] *= normalizationFactor;
	finalCoeff.value[7] *= normalizationFactor;
	finalCoeff.value[8] *= normalizationFactor;

	shCoefficients[rootConst.index] = finalCoeff;
}

//---------------------------------------------------------------------------------------------------------------------