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

#include "common.h"

//---------------------------------------------------------------------------------------------------------------------

VsOutputViewFill VertexMain(const uint vertexId : SV_VertexID)
{
	// Who needs a vertex buffer to draw a screen quad? Not us! Who needs a whole 2 triangles to
	// draw a screen quad? Again, not us! All we really need is geometry that covers the screen,
	// so a single triangle will suffice as long as it's big enough. And the vertices of the geometry
	// can be easily generated based on the vertex ID. The triangle needs to be double the size of
	// of the viewport in order to cover the entire view. This would mean the UVs would also need
	// to be doubled in size. Since we're generating output directly for normalized device coords,
	// this would put the triangle position coords in the range of [-1, 3] and the UVs would be
	// in the range of [0, 2].

	// Use the vertex ID to determine the UV coords.
	const float u = (float)(vertexId & 2);
	const float v = (float)(vertexId & 1) * 2.0f;

    VsOutputViewFill vsOut;

	// We can use the calculated UVs to figure out the position of the vertex.
	vsOut.position.x = (u * 2.0f) - 1.0f;
	vsOut.position.y = (v * 2.0f) - 1.0f;
	vsOut.position.zw = float2(0.0f, 1.0f);
	vsOut.texCoord.x = u;
	vsOut.texCoord.y = 1.0f - v;

	return vsOut;
}

//---------------------------------------------------------------------------------------------------------------------