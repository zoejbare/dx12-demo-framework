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

#include "Mesh.hpp"

#include "../../Utility/Array.hpp"

#include <vector>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class StaticMesh;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::StaticMesh
	: public DemoFramework::D3D12::IMesh
{
public:

	struct Geometry
	{
		struct Vertex
		{
			struct Position { float32_t x, y, z; };
			struct TexCoord { float32_t u, v; };
			struct Normal { float32_t x, y, z; };
			struct Tangent { float32_t x, y, z; };
			struct Binormal { float32_t x, y, z; };

			Position pos;
			TexCoord tex;
			Normal norm;
			Tangent tan;
			Binormal bin;
		};

		typedef uint32_t Index;

		typedef Utility::Array<Vertex> VertexArray;
		typedef Utility::Array<Index>  IndexArray;

		VertexArray vertexBuffer;
		IndexArray indexBuffer;
	};

	typedef std::shared_ptr<StaticMesh> Ptr;
	typedef Utility::Array<Ptr>         PtrArray;

	StaticMesh();
	virtual ~StaticMesh() {}

	static StaticMesh::Ptr Create(
		const Device::Ptr& device,
		const GraphicsCommandList::Ptr& cmdList,
		const char* name,
		const Geometry& geometry);

	virtual void Draw(
		const GraphicsCommandList::Ptr& cmdList,
		uint32_t instanceCount,
		uint32_t baseInstanceId) const override;

	virtual const char* GetName() const override;


private:

	char m_name[DF_MESH_NAME_MAX_SIZE];

	Resource::Ptr m_vertexResource;
	Resource::Ptr m_indexResource;

	Resource::Ptr m_stagingVertexResource;
	Resource::Ptr m_stagingIndexResource;

	uint32_t m_vertexCount;
	uint32_t m_indexCount;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API DemoFramework::D3D12::StaticMesh::Ptr;
template class DF_API DemoFramework::D3D12::StaticMesh::PtrArray;
template class DF_API DemoFramework::D3D12::StaticMesh::Geometry::VertexArray;
template class DF_API DemoFramework::D3D12::StaticMesh::Geometry::IndexArray;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::StaticMesh::StaticMesh()
	: m_name()
	, m_vertexResource()
	, m_indexResource()
	, m_stagingVertexResource()
	, m_stagingIndexResource()
	, m_vertexCount(0)
	, m_indexCount(0)
{
}

//---------------------------------------------------------------------------------------------------------------------
