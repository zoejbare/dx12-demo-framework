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

#include "CommandContext.hpp"

#include <memory>

//---------------------------------------------------------------------------------------------------------------------

#define DF_MESH_NAME_MAX_LENGTH 64

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class Model;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::Model
{
public:

	typedef std::shared_ptr<Model> Ptr;

	struct Vertex
	{
		struct Vec3
		{
			float32_t x, y, z;
		};

		struct Tex2
		{
			float32_t u, v;
		};

		Vec3 pos;     // position
		Vec3 nrm;     // normal
		Vec3 tan;     // tangent
		Vec3 bin;     // binormal
		Tex2 tex;     // texcoord
		uint32_t col; // color
	};

	struct Mesh
	{
		Mesh();
		~Mesh();

		ResourcePtr vertexBuffer;
		ResourcePtr indexBuffer;

		Vertex* pVertices;
		void* pIndices;

		uint32_t vertexCount;
		uint32_t indexCount;
		uint32_t indexStride;

		DXGI_FORMAT indexFormat;

		char name[DF_MESH_NAME_MAX_LENGTH];
	};

	Model();
	Model(const Model&) = delete;
	Model(Model&&) = delete;
	~Model();

	Model& operator =(const Model&) = delete;
	Model& operator =(Model&&) = delete;

	static D3D12_INPUT_LAYOUT_DESC GetInputLayout();

	static Ptr CreateFromObj(
		const DevicePtr& device,
		const CommandQueuePtr& cmdQueue,
		const GraphicsCommandContext::Ptr& uploadContext,
		const char* filePath);

	void Render(const GraphicsCommandListPtr& cmdList, uint32_t instanceCount, D3D12_PRIMITIVE_TOPOLOGY topology);


private:

	Mesh** m_ppMeshes;

	size_t m_meshCount;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::Model>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Model::Model()
	: m_ppMeshes(nullptr)
	, m_meshCount(0)
	, m_initialized(false)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Model::~Model()
{
	if(m_ppMeshes)
	{
		for(size_t i = 0; i < m_meshCount; ++i)
		{
			delete m_ppMeshes[i];
		}

		delete[] m_ppMeshes;
	}
}

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Model::Mesh::Mesh()
	: vertexBuffer()
	, indexBuffer()
	, pVertices(nullptr)
	, pIndices(nullptr)
	, vertexCount(0)
	, indexCount(0)
	, indexStride(0)
	, indexFormat(DXGI_FORMAT_UNKNOWN)
	, name()
{
	name[0] = '\0';
}

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Model::Mesh::~Mesh()
{
	if(pVertices)
	{
		delete[] pVertices;
	}

	if(pIndices)
	{
		delete[] pIndices;
	}
}

//---------------------------------------------------------------------------------------------------------------------

inline D3D12_INPUT_LAYOUT_DESC DemoFramework::D3D12::Model::GetInputLayout()
{
	constexpr D3D12_INPUT_ELEMENT_DESC positionElement =
	{
		"POSITION",                                 // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, pos),                      // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};
	constexpr D3D12_INPUT_ELEMENT_DESC normalElement =
	{
		"NORMAL",                                   // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, nrm),                      // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};
	constexpr D3D12_INPUT_ELEMENT_DESC tangentElement =
	{
		"TANGENT",                                  // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, tan),                      // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};
	constexpr D3D12_INPUT_ELEMENT_DESC binormalElement =
	{
		"BINORMAL",                                 // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, bin),                      // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};
	constexpr D3D12_INPUT_ELEMENT_DESC texCoordElement =
	{
		"TEXCOORD",                                 // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32_FLOAT,                   // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, tex),                      // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};
	constexpr D3D12_INPUT_ELEMENT_DESC colorElement =
	{
		"COLOR",                                    // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R8G8B8A8_UNORM,                 // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, col),                      // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC elements[] =
	{
		positionElement,
		normalElement,
		tangentElement,
		binormalElement,
		texCoordElement,
		colorElement,
	};
	const D3D12_INPUT_LAYOUT_DESC layoutDesc =
	{
		elements,
		_countof(elements),
	};

	return layoutDesc;
}

//---------------------------------------------------------------------------------------------------------------------
