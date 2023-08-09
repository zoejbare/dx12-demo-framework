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

ConstantBuffer<ShReduceRootConstant> rootConst : register(b0, space0);

RWStructuredBuffer<ShColorCoefficients> shCoefficients : register(u0, space0);
RWStructuredBuffer<float> shWeights : register(u1, space0);

//---------------------------------------------------------------------------------------------------------------------

[numthreads(DF_REFL_SH_LINEAR_THREAD_COUNT, 1, 1)]
void ComputeMain(uint threadId : SV_DispatchThreadID)
{
	// Calculate the start of the current segment based on the index range.
	const uint inputIndex = rootConst.headIndex + (threadId * DF_SH_REDUCE_SEGMENT_SIZE);
	const uint outputIndex = rootConst.tailIndex + threadId;

	if(inputIndex + DF_SH_REDUCE_SEGMENT_SIZE > rootConst.tailIndex)
	{
		// Disregard any compute threads that would step beyond the boundary of the current reduction group.
		return;
	}

	ShColorCoefficients outCoeff;
	outCoeff.value[0] = 0.0f;
	outCoeff.value[1] = 0.0f;
	outCoeff.value[2] = 0.0f;
	outCoeff.value[3] = 0.0f;
	outCoeff.value[4] = 0.0f;
	outCoeff.value[5] = 0.0f;
	outCoeff.value[6] = 0.0f;
	outCoeff.value[7] = 0.0f;
	outCoeff.value[8] = 0.0f;

	float outWeight = 0.0f;

	// Add together all the coefficients and weights from the current segment.
	[unroll]
	for(uint entryIndex = 0; entryIndex < DF_SH_REDUCE_SEGMENT_SIZE; ++entryIndex)
	{
		[unroll]
		for(uint coeffIndex = 0; coeffIndex < DF_SH_COEFF_COUNT; ++coeffIndex)
		{
			outCoeff.value[coeffIndex] += shCoefficients[inputIndex + entryIndex].value[coeffIndex];
		}

		outWeight += shWeights[inputIndex + entryIndex];
	}

	shCoefficients[outputIndex] = outCoeff;
	shWeights[outputIndex] = outWeight;
}

//---------------------------------------------------------------------------------------------------------------------