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

struct ConstantData
{
	row_major float4x4 viewInverse;
	row_major float4x4 projInverse;
};

ConstantBuffer<ConstantData> constBuffer : register(b0, space0);

//---------------------------------------------------------------------------------------------------------------------

VsEnvMapOutput VertexMain(VsEnvMapInput vsIn)
{
	// Force the screen quad to the very back of the scene so everything else is guaranteed to render on top of it.
	const float4 outputPosition = float4(vsIn.position, 1.0f, 1.0f);

	// First, reverse the projection transform on the input position. Then, reverse only the rotational
	// component of the view transform so the final vertex position is in centered around the camera.
	float4 viewPosition = mul(outputPosition, constBuffer.projInverse);
	float4 worldPosition = mul(float4(viewPosition.xyz, 0.0), constBuffer.viewInverse);

	// The input position should already be provided in normalized device coordinates.
	VsEnvMapOutput vsOut;
	vsOut.position = outputPosition;
	vsOut.cubeCoord = normalize(worldPosition.xyz);

	return vsOut;
}

//---------------------------------------------------------------------------------------------------------------------
