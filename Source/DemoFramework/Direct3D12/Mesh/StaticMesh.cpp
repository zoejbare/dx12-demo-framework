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

#include "StaticMesh.hpp"

#include "../LowLevel/Resource.hpp"

#include "../../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::StaticMesh::Ptr DemoFramework::D3D12::StaticMesh::Create(
	const Device::Ptr& device,
	const GraphicsCommandList::Ptr& cmdList,
	const char* const name,
	const Geometry& geometry)
{
	if(!device
		|| !cmdList
		|| !name
		|| name[0] == '\0'
		|| geometry.vertexBuffer.GetCount() == 0
		|| geometry.indexBuffer.GetCount() == 0)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	StaticMesh::Ptr output = std::make_shared<StaticMesh>();

	snprintf(output->m_name, DF_MESH_NAME_MAX_SIZE, "%s", name);

	output->m_vertexCount = uint32_t(geometry.vertexBuffer.GetCount());
	output->m_indexCount = uint32_t(geometry.indexBuffer.GetCount());

	constexpr DXGI_SAMPLE_DESC defaultSampleDesc =
	{
		1, // UINT Count
		0, // UINT Quality
	};

	constexpr D3D12_HEAP_PROPERTIES heapProps =
	{
		D3D12_HEAP_TYPE_CUSTOM,         // D3D12_HEAP_TYPE Type
		D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
		D3D12_MEMORY_POOL_L0,       // D3D12_MEMORY_POOL MemoryPoolPreference
		0,                               // UINT CreationNodeMask
		0,                               // UINT VisibleNodeMask
	};

	const D3D12_RESOURCE_DESC vertexDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER,                            // D3D12_RESOURCE_DIMENSION Dimension
		0,                                                          // UINT64 Alignment
		sizeof(Geometry::Vertex) * uint64_t(output->m_vertexCount), // UINT64 Width
		1,                                                          // UINT Height
		1,                                                          // UINT16 DepthOrArraySize
		1,                                                          // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,                                        // DXGI_FORMAT Format
		defaultSampleDesc,                                          // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,                             // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_NONE,                                   // D3D12_RESOURCE_FLAGS Flags
	};

	const D3D12_RESOURCE_DESC indexDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER,                          // D3D12_RESOURCE_DIMENSION Dimension
		0,                                                        // UINT64 Alignment
		sizeof(Geometry::Index) * uint64_t(output->m_indexCount), // UINT64 Width
		1,                                                        // UINT Height
		1,                                                        // UINT16 DepthOrArraySize
		1,                                                        // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,                                      // DXGI_FORMAT Format
		defaultSampleDesc,                                        // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,                           // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_NONE,                                 // D3D12_RESOURCE_FLAGS Flags
	};

	// Create the vertex buffer resource.
	output->m_vertexResource = CreateCommittedResource(
		device,
		vertexDesc,
		heapProps,
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	if(!output->m_vertexResource)
	{
		LOG_ERROR("Failed to create model vertex buffer: name=\"%s\"", name);
		return Ptr();
	}

	output->m_indexResource = CreateCommittedResource(
		device,
		indexDesc,
		heapProps,
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	if(!output->m_indexResource)
	{
		LOG_ERROR("Failed to create model index buffer: name=\"%s\"", name);
		return Ptr();
	}

	constexpr D3D12_RANGE dummyReadRange =
	{
		0, // SIZE_T Begin
		0, // SIZE_T End
	};

	Geometry::Vertex* pVertexBuffer = nullptr;
	Geometry::Index* pIndexBuffer = nullptr;

	// Map the vertex buffer to CPU-accessible memory.
	const HRESULT mapVertexBufferResult = output->m_vertexResource->Map(0, &dummyReadRange, reinterpret_cast<void**>(&pVertexBuffer));
	if(FAILED(mapVertexBufferResult))
	{
		LOG_ERROR("Failed to map static mesh vertex buffer; name=\"%s\", result='0x%08" PRIX32 "'", name, mapVertexBufferResult);
		return Ptr();
	}

	// Copy the vertex data to the GPU resource.
	memcpy(pVertexBuffer, geometry.vertexBuffer.GetData(), sizeof(Geometry::Vertex) * geometry.vertexBuffer.GetCount());
	output->m_vertexResource->Unmap(0, nullptr);

	// Map the index buffer to CPU-accessible memory.
	const HRESULT mapIndexBufferResult = output->m_indexResource->Map(0, &dummyReadRange, reinterpret_cast<void**>(&pIndexBuffer));
	if(FAILED(mapVertexBufferResult))
	{
		LOG_ERROR("Failed to map static mesh index buffer; name=\"%s\", result='0x%08" PRIX32 "'", name, mapIndexBufferResult);
		return Ptr();
	}

	// Copy the index data to the GPU resource.
	memcpy(pIndexBuffer, geometry.indexBuffer.GetData(), sizeof(Geometry::Index) * geometry.indexBuffer.GetCount());
	output->m_indexResource->Unmap(0, nullptr);

	D3D12_RESOURCE_BARRIER barrier[2];
	barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier[0].Transition.pResource = output->m_vertexResource.Get();
	barrier[0].Transition.Subresource = 0;
	barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	barrier[1] = barrier[0];
	barrier[1].Transition.pResource = output->m_indexResource.Get();
	barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

	// Transition the mesh resources so they can be used by the input assembler.
	cmdList->ResourceBarrier(_countof(barrier), barrier);

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::StaticMesh::Draw(
	const GraphicsCommandList::Ptr& cmdList,
	const uint32_t instanceCount,
	const uint32_t baseInstanceId) const
{
	const DXGI_FORMAT indexFormat = (sizeof(Geometry::Index) == 2)
		? DXGI_FORMAT_R16_UINT
		: DXGI_FORMAT_R32_UINT;

	const D3D12_VERTEX_BUFFER_VIEW vertexBufferView =
	{
		m_vertexResource->GetGPUVirtualAddress(), // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		sizeof(Geometry::Vertex) * m_vertexCount, // UINT SizeInBytes
		sizeof(Geometry::Vertex),                 // UINT StrideInBytes
	};

	const D3D12_INDEX_BUFFER_VIEW indexBufferView =
	{
		m_indexResource->GetGPUVirtualAddress(), // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		sizeof(Geometry::Index) * m_indexCount,  // UINT SizeInBytes
		indexFormat,                             // DXGI_FORMAT Format
	};

	cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
	cmdList->IASetIndexBuffer(&indexBufferView);
	cmdList->DrawIndexedInstanced(m_indexCount, instanceCount, 0, 0, baseInstanceId);
}

//---------------------------------------------------------------------------------------------------------------------

const char* DemoFramework::D3D12::StaticMesh::GetName() const
{
	return m_name;
}

//---------------------------------------------------------------------------------------------------------------------
