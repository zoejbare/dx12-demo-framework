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
	// Normalize the final coefficient sums.
	ShColorCoefficients finalCoeff = shCoefficients[rootConst.index];

	// Do the division first for the sake of floating point precision.
	// This will keep the values in a good range which is especially
	// necessary for high resolution environment maps since they
	// generate much larger sums.
	const float weightSum = shWeights[rootConst.index];
	finalCoeff.value[0] /= weightSum;
	finalCoeff.value[1] /= weightSum;
	finalCoeff.value[2] /= weightSum;
	finalCoeff.value[3] /= weightSum;
	finalCoeff.value[4] /= weightSum;
	finalCoeff.value[5] /= weightSum;
	finalCoeff.value[6] /= weightSum;
	finalCoeff.value[7] /= weightSum;
	finalCoeff.value[8] /= weightSum;

	// Apply the inverse probability function.
	finalCoeff.value[0] *= M_4_PI;
	finalCoeff.value[1] *= M_4_PI;
	finalCoeff.value[2] *= M_4_PI;
	finalCoeff.value[3] *= M_4_PI;
	finalCoeff.value[4] *= M_4_PI;
	finalCoeff.value[5] *= M_4_PI;
	finalCoeff.value[6] *= M_4_PI;
	finalCoeff.value[7] *= M_4_PI;
	finalCoeff.value[8] *= M_4_PI;

	shCoefficients[rootConst.index] = finalCoeff;
}

//---------------------------------------------------------------------------------------------------------------------