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

#include "Model.hpp"
#include "Sync.hpp"

#include "LowLevel/Resource.hpp"

#include "../Application/Log.hpp"

#include <tiny_obj_loader.h>

#include <DirectXMath.h>

#include <string>
#include <unordered_map>
#include <vector>

//---------------------------------------------------------------------------------------------------------------------

namespace std
{
	template <>
	struct hash<tinyobj::index_t>
	{
		inline size_t operator()(const tinyobj::index_t& value) const noexcept
		{
			const size_t vertexIndexHash = (size_t(value.vertex_index) << 7) | (size_t(value.vertex_index) >> 25);
			const size_t normalIndexHash = (size_t(value.normal_index) << 29) | (size_t(value.normal_index) >> 3);
			const size_t texCoordIndexHash = (size_t(value.texcoord_index) << 14) | (size_t(value.texcoord_index) >> 18);

			return vertexIndexHash ^ normalIndexHash ^ texCoordIndexHash;
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

constexpr DXGI_SAMPLE_DESC defaultSampleDesc =
{
	1, // UINT Count
	0, // UINT Quality
};

constexpr D3D12_RANGE disabledCpuReadRange =
{
	0, // SIZE_T Begin
	0, // SIZE_T End
};

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Model::Ptr DemoFramework::D3D12::Model::CreateFromObj(
	const Device::Ptr& device,
	const CommandQueue::Ptr& cmdQueue,
	const GraphicsCommandContext::Ptr& uploadContext,
	const char* const filePath)
{
	using namespace DirectX;

	if(!device || !filePath || filePath[0] == '\0')
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	Ptr output = std::make_shared<Model>();

	tinyobj::attrib_t attrib;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	const bool loadResult = tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, filePath, nullptr, false, true);

	if(!loadResult)
	{
		LOG_ERROR("Failed to load Wavefront OBJ file: %s", filePath);
		return Ptr();
	}

	if(shapes.size() > 0)
	{
		auto createMesh = [&device, &cmdQueue, &uploadContext, &attrib](const tinyobj::shape_t& shape) -> Mesh*
		{
			std::unordered_map<
				tinyobj::index_t,
				uint32_t,
				std::hash<tinyobj::index_t>,
				std::equal_to<tinyobj::index_t>
			> indexLookupTable;

			std::vector<Vertex> resolvedVertices;
			std::vector<uint32_t> resolvedIndicies;

			uint32_t largestVertexIndex = 0;

			auto mapIndex = [&attrib, &indexLookupTable, &resolvedVertices, &resolvedIndicies, &largestVertexIndex](const tinyobj::index_t& index)
			{
				auto indexKv = indexLookupTable.find(index);
				if(indexKv == indexLookupTable.end())
				{
					indexKv = indexLookupTable.emplace(index, uint32_t(indexLookupTable.size())).first;

					Vertex vertex;

					vertex.pos.x = attrib.vertices[(3 * index.vertex_index) + 0];
					vertex.pos.y = attrib.vertices[(3 * index.vertex_index) + 1];
					vertex.pos.z = attrib.vertices[(3 * index.vertex_index) + 2];

					vertex.nrm.x = attrib.normals[(3 * index.normal_index) + 0];
					vertex.nrm.y = attrib.normals[(3 * index.normal_index) + 1];
					vertex.nrm.z = attrib.normals[(3 * index.normal_index) + 2];

					vertex.tex.u = attrib.texcoords[(2 * index.texcoord_index) + 0];
					vertex.tex.v = attrib.texcoords[(2 * index.texcoord_index) + 1];

					const float32_t r = attrib.colors[(3 * index.vertex_index) + 0];
					const float32_t g = attrib.colors[(3 * index.vertex_index) + 1];
					const float32_t b = attrib.colors[(3 * index.vertex_index) + 2];

					vertex.col = 0xFF000000 | (uint32_t(b * 255.0f) << 16) | (uint32_t(g * 255.0f) << 8) | uint32_t(r * 255.0f);

					XMVECTORF32 normal;
					normal.f[0] = vertex.nrm.x;
					normal.f[1] = vertex.nrm.y;
					normal.f[2] = vertex.nrm.z;

					normal.v = XMVector3NormalizeEst(normal.v);

					// It's not *entirely* correct to guess at the tangent initial direction aligned to a world axis,
					// but it's close enough for most testing purposes. We can replace this with something else later.
					XMVECTORF32 tangent;
					tangent.f[0] = 1.0f;
					tangent.f[1] = 0.0f;
					tangent.f[2] = 0.0f;

					XMVECTORF32 nDotT;
					nDotT.v = XMVector3Dot(normal, tangent);

					// The normal and tangent overlap or are VERY close to overlapping.
					// In this case, we change the initial direction of the tangent.
					if(nDotT.f[0] > -FLT_EPSILON && nDotT.f[0] < FLT_EPSILON)
					{
						tangent.f[0] = 0.0f;
						tangent.f[1] = 0.0f;
						tangent.f[2] = 1.0f;
					}

					// Calculate the binormal, then use that to recalculate the tangent to create a perfect orthonormal basis.
					XMVECTORF32 binormal;
					binormal.v = XMVector3Cross(tangent.v, normal.v);
					tangent.v = XMVector3Cross(normal.v, binormal.v);

					vertex.tan.x = tangent.f[0];
					vertex.tan.y = tangent.f[1];
					vertex.tan.z = tangent.f[2];

					vertex.bin.x = binormal.f[0];
					vertex.bin.y = binormal.f[1];
					vertex.bin.z = binormal.f[2];

					// Add the resolved vertex to the end of the vertex array.
					resolvedVertices.push_back(vertex);
				}

				// Add the final index value to the end of the index array.
				resolvedIndicies.push_back(indexKv->second);

				if(indexKv->second > largestVertexIndex)
				{
					// Track the largest vertex index so we know what format to use with the index buffer.
					largestVertexIndex = indexKv->second;
				}
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

			const size_t vertexCount = resolvedVertices.size();
			const size_t indexCount = resolvedIndicies.size();

			Mesh* const pMesh = new Mesh();
			snprintf(pMesh->name, DF_MESH_NAME_MAX_LENGTH, "%s", shape.name.c_str());

			pMesh->pVertices = new Vertex[vertexCount];

			// Copy the vertices into the final vertex array.
			for(const Vertex& vertex : resolvedVertices)
			{
				pMesh->pVertices[pMesh->vertexCount] = vertex;

				++pMesh->vertexCount;
			}

			// Determine what data type should be used for the final index array.
			if(largestVertexIndex > UINT16_MAX)
			{
				uint32_t* const pIndices = new uint32_t[indexCount];

				// Copy the indices into the final index array.
				for(uint32_t index : resolvedIndicies)
				{
					pIndices[pMesh->indexCount] = index;

					++pMesh->indexCount;
				}

				pMesh->pIndices = pIndices;
				pMesh->indexFormat = DXGI_FORMAT_R32_UINT;
				pMesh->indexStride = sizeof(uint32_t);
			}
			else
			{
				uint16_t* const pIndices = new uint16_t[indexCount];

				// Copy the indices into the final index array.
				for(uint32_t index : resolvedIndicies)
				{
					pIndices[pMesh->indexCount] = uint16_t(index);

					++pMesh->indexCount;
				}

				pMesh->pIndices = pIndices;
				pMesh->indexFormat = DXGI_FORMAT_R16_UINT;
				pMesh->indexStride = sizeof(uint16_t);
			}

			const D3D12_HEAP_PROPERTIES defaultHeapProps =
			{
				D3D12_HEAP_TYPE_DEFAULT,         // D3D12_HEAP_TYPE Type
				D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
				D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
				0,                               // UINT CreationNodeMask
				0,                               // UINT VisibleNodeMask
			};

			const D3D12_HEAP_PROPERTIES uploadHeapProps =
			{
				D3D12_HEAP_TYPE_UPLOAD,          // D3D12_HEAP_TYPE Type
				D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
				D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
				0,                               // UINT CreationNodeMask
				0,                               // UINT VisibleNodeMask
			};

			const D3D12_RESOURCE_DESC vertexBufferDesc =
			{
				D3D12_RESOURCE_DIMENSION_BUFFER, // D3D12_RESOURCE_DIMENSION Dimension
				0,                               // UINT64 Alignment
				sizeof(Vertex) * vertexCount,    // UINT64 Width
				1,                               // UINT Height
				1,                               // UINT16 DepthOrArraySize
				1,                               // UINT16 MipLevels
				DXGI_FORMAT_UNKNOWN,             // DXGI_FORMAT Format
				defaultSampleDesc,               // DXGI_SAMPLE_DESC SampleDesc
				D3D12_TEXTURE_LAYOUT_ROW_MAJOR,  // D3D12_TEXTURE_LAYOUT Layout
				D3D12_RESOURCE_FLAG_NONE,        // D3D12_RESOURCE_FLAGS Flags
			};

			const D3D12_RESOURCE_DESC indexBufferDesc =
			{
				D3D12_RESOURCE_DIMENSION_BUFFER, // D3D12_RESOURCE_DIMENSION Dimension
				0,                               // UINT64 Alignment
				pMesh->indexStride * indexCount, // UINT64 Width
				1,                               // UINT Height
				1,                               // UINT16 DepthOrArraySize
				1,                               // UINT16 MipLevels
				DXGI_FORMAT_UNKNOWN,             // DXGI_FORMAT Format
				defaultSampleDesc,               // DXGI_SAMPLE_DESC SampleDesc
				D3D12_TEXTURE_LAYOUT_ROW_MAJOR,  // D3D12_TEXTURE_LAYOUT Layout
				D3D12_RESOURCE_FLAG_NONE,        // D3D12_RESOURCE_FLAGS Flags
			};

			// Create the staging vertex buffer.
			Resource::Ptr stagingVertexBuffer = CreateCommittedResource(
				device,
				vertexBufferDesc,
				uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				D3D12_RESOURCE_STATE_GENERIC_READ);
			if(!stagingVertexBuffer)
			{
				delete pMesh;
				return nullptr;
			}

			// Create the staging index buffer;
			Resource::Ptr stagingIndexBuffer = CreateCommittedResource(
				device,
				indexBufferDesc,
				uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				D3D12_RESOURCE_STATE_GENERIC_READ);
			if(!stagingIndexBuffer)
			{
				delete pMesh;
				return nullptr;
			}

			// Create the mesh vertex buffer.
			pMesh->vertexBuffer = CreateCommittedResource(
				device,
				vertexBufferDesc,
				defaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				D3D12_RESOURCE_STATE_COPY_DEST);
			if(!pMesh->vertexBuffer)
			{
				delete pMesh;
				return nullptr;
			}

			// Create the mesh index buffer.
			pMesh->indexBuffer = CreateCommittedResource(
				device,
				indexBufferDesc,
				defaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				D3D12_RESOURCE_STATE_COPY_DEST);
			if(!pMesh->indexBuffer)
			{
				delete pMesh;
				return nullptr;
			}

			// Create the staging command synchronization primitive so we can wait for
			// all staging resource copies to complete at the end of initialization.
			D3D12::Sync::Ptr stagingCmdSync = D3D12::Sync::Create(device, D3D12_FENCE_FLAG_NONE);
			if(!stagingCmdSync)
			{
				delete pMesh;
				return nullptr;
			}

			void* pStagingVertexData = nullptr;
			void* pStagingIndexData = nullptr;

			// Map the staging vertex buffer with CPU read access disabled.
			const HRESULT mapVertexBufferResult = stagingVertexBuffer->Map(0, &disabledCpuReadRange, &pStagingVertexData);
			assert(mapVertexBufferResult == S_OK); (void) mapVertexBufferResult;

			// Copy the vertex data into the staging buffer, then unmap it.
			memcpy(pStagingVertexData, pMesh->pVertices, sizeof(Vertex) * pMesh->vertexCount);
			stagingVertexBuffer->Unmap(0, nullptr);

			// Map the staging index buffer with CPU read access disabled.
			const HRESULT mapIndexBufferResult = stagingIndexBuffer->Map(0, &disabledCpuReadRange, &pStagingIndexData);
			assert(mapIndexBufferResult == S_OK); (void) mapIndexBufferResult;

			// Copy the index data into the staging buffer, then unmap it.
			memcpy(pStagingIndexData, pMesh->pIndices, pMesh->indexStride * pMesh->indexCount);
			stagingIndexBuffer->Unmap(0, nullptr);

			D3D12_RESOURCE_BARRIER vertexBufferBarrier, indexBufferBarrier;

			vertexBufferBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			vertexBufferBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			vertexBufferBarrier.Transition.pResource = pMesh->vertexBuffer.Get();
			vertexBufferBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			vertexBufferBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			vertexBufferBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

			indexBufferBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			indexBufferBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			indexBufferBarrier.Transition.pResource = pMesh->indexBuffer.Get();
			indexBufferBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			indexBufferBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			indexBufferBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

			const D3D12_RESOURCE_BARRIER bufferBarriers[] =
			{
				vertexBufferBarrier,
				indexBufferBarrier,
			};

			ID3D12GraphicsCommandList* const pUploadCmdList = uploadContext->GetCmdList().Get();

			// Before rendering begins, use a temporary command list to copy all buffer data.
			pUploadCmdList->CopyResource(pMesh->vertexBuffer.Get(), stagingVertexBuffer.Get());
			pUploadCmdList->CopyResource(pMesh->indexBuffer.Get(), stagingIndexBuffer.Get());
			pUploadCmdList->ResourceBarrier(_countof(bufferBarriers), bufferBarriers);

			// Stop recording commands in the staging command list and begin executing it.
			uploadContext->Submit(cmdQueue);

			// Wait for the staging command list to finish executing.
			stagingCmdSync->Signal(cmdQueue);
			stagingCmdSync->Wait();

			// Reset the command list so it can be used again.
			uploadContext->Reset();

			return pMesh;
		};

		std::vector<Mesh*> meshes;
		meshes.reserve(shapes.size());

		for(const tinyobj::shape_t& shape : shapes)
		{
			Mesh* const pMesh = createMesh(shape);
			if(pMesh)
			{
				meshes.push_back(pMesh);
			}
		}

		const size_t meshCount = meshes.size();

		if(meshCount > 0)
		{
			output->m_ppMeshes = new Mesh*[meshCount];

			// Copy the meshes to the final mesh array.
			for(Mesh* const pMesh : meshes)
			{
				output->m_ppMeshes[output->m_meshCount] = pMesh;

				++output->m_meshCount;
			}
		}
	}

	output->m_initialized = true;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Model::Render(
	const GraphicsCommandList::Ptr& cmdList,
	const uint32_t instanceCount,
	const D3D12_PRIMITIVE_TOPOLOGY topology)
{
	if(m_initialized && instanceCount > 0)
	{
		cmdList->IASetPrimitiveTopology(topology);

		// Draw each mesh.
		for(size_t meshIndex = 0; meshIndex < m_meshCount; ++meshIndex)
		{
			const Mesh* const pMesh = m_ppMeshes[meshIndex];

			const D3D12_VERTEX_BUFFER_VIEW vertexBufferView =
			{
				pMesh->vertexBuffer->GetGPUVirtualAddress(), // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
				pMesh->vertexCount * sizeof(Vertex),         // UINT SizeInBytes
				sizeof(Vertex),                              // UINT StrideInBytes
			};

			const D3D12_INDEX_BUFFER_VIEW indexBufferView =
			{
				pMesh->indexBuffer->GetGPUVirtualAddress(), // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
				pMesh->indexCount * pMesh->indexStride,     // UINT SizeInBytes
				pMesh->indexFormat,                         // DXGI_FORMAT Format
			};

			cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
			cmdList->IASetIndexBuffer(&indexBufferView);
			cmdList->DrawIndexedInstanced(pMesh->indexCount, instanceCount, 0, 0, 0);
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
