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

ConstantBuffer<EquiToCubeRootConstant> rootConst : register(b0, space0);

Texture2D equirectMap : register(t0, space0);
SamplerState mapSampler : register(s0, space0);

RWTexture2D<float4> envFaceMap : register(u0, space0);

//---------------------------------------------------------------------------------------------------------------------

[numthreads(DF_REFL_THREAD_COUNT_X, DF_REFL_THREAD_COUNT_Y, 1)]
void ComputeMain(uint2 threadId : SV_DispatchThreadID)
{
	if(threadId.x >= rootConst.edgeLength || threadId.y >= rootConst.edgeLength)
	{
		return;
	}

	const float3 normal = CalculateNormalFromPixelCoord(threadId, rootConst.faceIndex, rootConst.invEdgeLength);
	const float2 texCoord = CalculateEquirectUvFromNormal(normal);

	envFaceMap[threadId] = equirectMap.SampleLevel(mapSampler, texCoord, rootConst.mipIndex);
}

//---------------------------------------------------------------------------------------------------------------------