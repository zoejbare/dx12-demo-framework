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

#include "WavefrontObj.hpp"

#include "../Application/Log.hpp"

#include <DirectXMath.h>

//---------------------------------------------------------------------------------------------------------------------

namespace std
{
	template <>
	struct hash<tinyobj::index_t>
	{
		inline size_t operator()(const tinyobj::index_t& value) const noexcept
		{
			constexpr uint32_t fnvOffsetBasis = 0x811C9DC5ul;
			constexpr uint32_t fnvPrime = 0x01000193ul;

			const uint8_t* const pDataStream = reinterpret_cast<const uint8_t*>(&value);

			uint32_t output = fnvOffsetBasis;

			// Calculate the FNV1a hash of the input value.
			for(size_t i = 0; i < sizeof(tinyobj::index_t); ++i)
			{
				output ^= pDataStream[i];
				output *= fnvPrime;
			}

			return output;
		}
	};

	template<>
	struct equal_to<tinyobj::index_t>
	{
		inline bool operator()(const tinyobj::index_t& left, const tinyobj::index_t& right) const
		{
			return (left.vertex_index == right.vertex_index)
				&& (left.texcoord_index == right.texcoord_index)
				&& (left.normal_index == right.normal_index);
		}
	};
}

//---------------------------------------------------------------------------------------------------------------------

struct DemoFramework::D3D12::WavefrontObj::InternalData
{
	std::string name;
	tinyobj::attrib_t attrib;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
};

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::WavefrontObj::Ptr DemoFramework::D3D12::WavefrontObj::Load(
	const Device::Ptr& device,
	const GraphicsCommandList::Ptr& cmdList,
	const char* const name,
	const char* const filePath)
{
	// Check for errors with the input arguments.
	if(!device || !cmdList || !name || name[0] == '\0' || !filePath || filePath[0] == '\0')
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	std::string warnings;
	std::string errors;

	Ptr output = std::make_shared<WavefrontObj>();

	InternalData data;
	data.name = name;

	if(!tinyobj::LoadObj(&data.attrib, &data.shapes, &data.materials, &warnings, &errors, filePath, nullptr, false, false))
	{
		// Failed to load the OBJ file.
		output.reset();
	}

	if(!warnings.empty())
	{
		LOG_WRITE("(warning) [OBJ_LOAD] (%s) %s", name, warnings.c_str());
	}

	if(!errors.empty())
	{
		LOG_ERROR("[OBJ_LOAD] (%s) %s", name, errors.c_str());
		return Ptr();
	}

	if(!output->_build(data, device, cmdList))
	{
		LOG_ERROR("Failed to construct meshes from OBJ file: name=\"%s\"", name);
		return Ptr();
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::WavefrontObj::Draw(const GraphicsCommandList::Ptr& cmdList) const
{
	const StaticMesh::Ptr* const pMeshes = m_meshes.GetData();
	const size_t meshCount = m_meshes.GetCount();

	// Draw each mesh in the object.
	for(size_t i = 0; i < meshCount; ++i)
	{
		pMeshes[i]->Draw(cmdList, 1, 0);
	}
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::WavefrontObj::_build(
	const InternalData& data,
	const Device::Ptr& device,
	const GraphicsCommandList::Ptr& cmdList)
{
	using namespace DirectX;

	if(data.shapes.size() == 0)
	{
		// No shape data in the file; nothing to do.
		return true;
	}

	auto createMesh = [this, &data, &device, &cmdList](const tinyobj::shape_t& shape) -> StaticMesh::Ptr
	{
		std::unordered_map<
			tinyobj::index_t,
			uint32_t,
			std::hash<tinyobj::index_t>,
			std::equal_to<tinyobj::index_t>
		> indexLookupTable;

		std::vector<StaticMesh::Geometry::Vertex> vertexBuffer;
		std::vector<uint32_t> indexBuffer;

		auto mapIndex = [this, &data, &device, &cmdList, &indexLookupTable, &vertexBuffer, &indexBuffer](const tinyobj::index_t& index)
		{
			auto indexKv = indexLookupTable.find(index);
			if(indexKv == indexLookupTable.end())
			{
				indexKv = indexLookupTable.emplace(index, uint32_t(indexLookupTable.size())).first;

				StaticMesh::Geometry::Vertex vertex;

				vertex.pos.x = data.attrib.vertices[(3 * index.vertex_index) + 0];
				vertex.pos.y = data.attrib.vertices[(3 * index.vertex_index) + 1];
				vertex.pos.z = data.attrib.vertices[(3 * index.vertex_index) + 2];

				vertex.tex.u = data.attrib.texcoords[(2 * index.texcoord_index) + 0];
				vertex.tex.v = data.attrib.texcoords[(2 * index.texcoord_index) + 1];

				vertex.norm.x = data.attrib.normals[(3 * index.normal_index) + 0];
				vertex.norm.y = data.attrib.normals[(3 * index.normal_index) + 1];
				vertex.norm.z = data.attrib.normals[(3 * index.normal_index) + 2];

				XMVECTOR normal = XMVector3Normalize(XMVectorSet(vertex.norm.x, vertex.norm.y, vertex.norm.z, 0.0f));
				XMVECTOR tangent = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
				XMVECTOR dot = XMVector3Dot(normal, tangent);

				if(fabsf(XMVectorGetX(dot)) >= 1.0f - FLT_EPSILON)
				{
					tangent = XMVectorSet(0.0, 0.0f, 1.0f, 0.0f);
				}

				// Calculate the binormal, then use that to recalculate the tangent to create a perfect orthonormal basis.
				XMVECTOR binormal = XMVector3Normalize(XMVector3Cross(tangent, normal));
				tangent = XMVector3Cross(normal, binormal);

				vertex.tan.x = XMVectorGetX(tangent);
				vertex.tan.y = XMVectorGetY(tangent);
				vertex.tan.z = XMVectorGetZ(tangent);

				vertex.bin.x = XMVectorGetX(binormal);
				vertex.bin.y = XMVectorGetY(binormal);
				vertex.bin.z = XMVectorGetZ(binormal);

				vertexBuffer.push_back(vertex);
			}

			indexBuffer.push_back(indexKv->second);
		};

		size_t offset = 0;

		for(const uint8_t vertexCount : shape.mesh.num_face_vertices)
		{
			if(vertexCount == 3)
			{
				mapIndex(shape.mesh.indices[offset + 0]);
				mapIndex(shape.mesh.indices[offset + 1]);
				mapIndex(shape.mesh.indices[offset + 2]);
			}
			else if(vertexCount == 4)
			{
				const tinyobj::index_t& i0 = shape.mesh.indices[offset + 0];
				const tinyobj::index_t& i1 = shape.mesh.indices[offset + 1];
				const tinyobj::index_t& i2 = shape.mesh.indices[offset + 2];
				const tinyobj::index_t& i3 = shape.mesh.indices[offset + 3];

				mapIndex(i0);
				mapIndex(i1);
				mapIndex(i3);

				mapIndex(i3);
				mapIndex(i1);
				mapIndex(i2);
			}
			else
			{
				// Skip higher order faces for now.
				continue;
			}

			offset += vertexCount;
		}

		StaticMesh::Geometry geometry;
		geometry.vertexBuffer = StaticMesh::Geometry::VertexArray::Create(vertexBuffer.size());
		geometry.indexBuffer = StaticMesh::Geometry::IndexArray::Create(indexBuffer.size());

		StaticMesh::Geometry::Vertex* const pVertexBuffer = geometry.vertexBuffer.GetData();
		StaticMesh::Geometry::Index* const pIndexBuffer = geometry.indexBuffer.GetData();

		// Copy the vertex data to the final array.
		for(size_t i = 0; i < geometry.vertexBuffer.GetCount(); ++i)
		{
			pVertexBuffer[i] = vertexBuffer[i];
		}

		// Copy the index data to the final array.
		for(size_t i = 0; i < geometry.indexBuffer.GetCount(); ++i)
		{
			pIndexBuffer[i] = indexBuffer[i];
		}

		const std::string meshName = data.name + " [" + shape.name + "]";

		return StaticMesh::Create(device, cmdList, meshName.c_str(), geometry);
	};

	std::vector<StaticMesh::Ptr> meshes;
	meshes.reserve(data.shapes.size());

	for(const tinyobj::shape_t& shape : data.shapes)
	{
		// Attempt to create a mesh from the current shape.
		StaticMesh::Ptr mesh = createMesh(shape);
		if(mesh)
		{
			meshes.push_back(mesh);
		}
	}

	// Verify that some meshes were actually created.
	if(meshes.empty())
	{
		return false;
	}

	// Create the array of meshes.
	m_meshes = StaticMesh::PtrArray::Create(uint32_t(meshes.size()));
	StaticMesh::Ptr* const pMeshes = m_meshes.GetData();

	// Copy the meshes to the output array.
	for(size_t i = 0; i < meshes.size(); ++i)
	{
		pMeshes[i] = meshes[i];
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
