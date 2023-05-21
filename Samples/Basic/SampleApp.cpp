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

#include "SampleApp.hpp"

#include <DemoFramework/Application/Log.hpp>
#include <DemoFramework/Application/Window.hpp>

#include <DemoFramework/Direct3D12/CommandAllocator.hpp>
#include <DemoFramework/Direct3D12/DescriptorHeap.hpp>
#include <DemoFramework/Direct3D12/GraphicsCommandList.hpp>
#include <DemoFramework/Direct3D12/PipelineState.hpp>
#include <DemoFramework/Direct3D12/Resource.hpp>
#include <DemoFramework/Direct3D12/RootSignature.hpp>

#include <DemoFramework/Direct3D12/Struct/Sync.hpp>

#include "../Common/Shader.hpp"

#include <math.h>

//---------------------------------------------------------------------------------------------------------------------

#define APP_NAME "Basic Sample"
#define LOG_FILE "basic-sample.log"

#define APP_BACK_BUFFER_COUNT   2
#define APP_BACK_BUFFER_FORMAT  DXGI_FORMAT_R8G8B8A8_UNORM
#define APP_DEPTH_BUFFER_FORMAT DXGI_FORMAT_D32_FLOAT

#define M_PI  3.1415926535897932384626433832795f
#define M_TAU (M_PI * 2.0f)

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

constexpr D3D12_HEAP_PROPERTIES uploadHeapProps =
{
	D3D12_HEAP_TYPE_UPLOAD,          // D3D12_HEAP_TYPE Type
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
	D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
	0,                               // UINT CreationNodeMask
	0,                               // UINT VisibleNodeMask
};

constexpr D3D12_HEAP_PROPERTIES defaultHeapProps =
{
	D3D12_HEAP_TYPE_DEFAULT,         // D3D12_HEAP_TYPE Type
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
	D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
	0,                               // UINT CreationNodeMask
	0,                               // UINT VisibleNodeMask
};

struct VertexPosition
{
	float32_t x, y, z;
};

struct VertexColor
{
	float32_t r, g, b, a;
};

struct Vertex
{
	VertexPosition pos;
	VertexColor color;
};

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::Initialize(DemoFramework::Window* const pWindow)
{
	using namespace DemoFramework;

	m_pWindow = pWindow;

	HWND hWnd = m_pWindow->GetWindowHandle();

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();

	// Initialize the common rendering resources.
	if(!RenderBase::Initialize(m_renderBase, hWnd, clientWidth, clientHeight, APP_BACK_BUFFER_COUNT, APP_BACK_BUFFER_FORMAT, APP_DEPTH_BUFFER_FORMAT))
	{
		return false;
	}

	// Load the shaders required by the application.
	if(!prv_loadShaders())
	{
		return false;
	}

	// Initialize the graphics pipeline resources.
	if(!prv_createGfxPipeline())
	{
		return false;
	}

	// Initialize the geometry resources.
	if(!prv_createQuadGeometry())
	{
		return false;
	}

	// Initialize the constant buffer resources.
	if(!prv_createConstBuffer())
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::Update()
{
	using namespace DemoFramework;
	using namespace DirectX;

	if(m_resizeSwapChain)
	{
		RenderBase::ResizeSwapChain(m_renderBase);

		m_resizeSwapChain = false;
	}

	m_frameTimer.Update();

	static float32_t rotation = 0.0f;

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();

	XMVECTOR cameraPosition = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	XMVECTOR cameraForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR cameraUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_worldMatrix = XMMatrixRotationAxis(cameraForward, rotation);
	m_viewMatrix = XMMatrixLookToLH(cameraPosition, cameraForward, cameraUp);
	m_projMatrix = XMMatrixPerspectiveFovLH(M_PI * 0.5f, float32_t(clientWidth) / float32_t(clientHeight), 0.1f, 1000.0f);
	m_wvpMatrix = XMMatrixMultiply(XMMatrixMultiply(m_worldMatrix, m_viewMatrix), m_projMatrix);

	rotation += M_TAU * float32_t(m_frameTimer.GetDeltaTime()) * 0.1f;
	if(rotation >= M_TAU)
	{
		rotation -= M_TAU;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Render()
{
	using namespace DemoFramework;

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();

	RenderBase::BeginFrame(m_renderBase);
	RenderBase::SetDefaultRenderTarget(m_renderBase);

	void* pConstData = nullptr;

	// Map the staging buffer to CPU-accessible memory (write-only access) and copy the constant data to it.
	const D3D12_RANGE dummyReadRange = {0, 0};
	const HRESULT result = m_pStagingConstBuffer[m_renderBase.bufferIndex]->Map(0, &dummyReadRange, &pConstData);
	if(result == S_OK)
	{
		memcpy(pConstData, &m_wvpMatrix, sizeof(m_wvpMatrix));
		m_pStagingConstBuffer[m_renderBase.bufferIndex]->Unmap(0, nullptr);
	}

	ID3D12GraphicsCommandList* const pCmdList = m_renderBase.pCmdList[m_renderBase.bufferIndex].Get();

	ID3D12DescriptorHeap* const pDescHeaps[] =
	{
		m_pVertexShaderDescHeap.Get(),
	};

	D3D12_RESOURCE_BARRIER constBufferBeginBarrier, constBufferEndBarrier;

	constBufferBeginBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	constBufferBeginBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	constBufferBeginBarrier.Transition.pResource = m_pConstBuffer.Get();
	constBufferBeginBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	constBufferBeginBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	constBufferBeginBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

	constBufferEndBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	constBufferEndBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	constBufferEndBarrier.Transition.pResource = m_pConstBuffer.Get();
	constBufferEndBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	constBufferEndBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	constBufferEndBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	// Initiate a copy of constant buffer data from staging memory.
	pCmdList->ResourceBarrier(1, &constBufferBeginBarrier);
	pCmdList->CopyResource(m_pConstBuffer.Get(), m_pStagingConstBuffer[m_renderBase.bufferIndex].Get());
	pCmdList->ResourceBarrier(1, &constBufferEndBarrier);

	// Set the graphics pipeline state.
	pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	pCmdList->SetDescriptorHeaps(DF_ARRAY_LENGTH(pDescHeaps), pDescHeaps);
	pCmdList->SetGraphicsRootDescriptorTable(0, m_pVertexShaderDescHeap->GetGPUDescriptorHandleForHeapStart());
	pCmdList->SetPipelineState(m_pGfxPipeline.Get());

	const D3D12_VERTEX_BUFFER_VIEW vertexBufferView =
	{
		m_pQuadVertexBuffer->GetGPUVirtualAddress(),    // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		uint32_t(m_pQuadVertexBuffer->GetDesc().Width), // UINT SizeInBytes
		sizeof(Vertex),                                 // UINT StrideInBytes
	};

	const D3D12_INDEX_BUFFER_VIEW indexBufferView =
	{
		m_pQuadIndexBuffer->GetGPUVirtualAddress(),    // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		uint32_t(m_pQuadIndexBuffer->GetDesc().Width), // UINT SizeInBytes
		DXGI_FORMAT_R16_UINT,                          // DXGI_FORMAT Format
	};

	// Bind the geometry that we want to draw.
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
	pCmdList->IASetIndexBuffer(&indexBufferView);

	// Set the screen viewport.
	const D3D12_VIEWPORT viewport =
	{
		0.0f,                    // FLOAT TopLeftX
		0.0f,                    // FLOAT TopLeftY
		float32_t(clientWidth),  // FLOAT Width
		float32_t(clientHeight), // FLOAT Height
		0.0f,                    // FLOAT MinDepth
		1.0f,                    // FLOAT MaxDepth
	};
	pCmdList->RSSetViewports(1, &viewport);

	// Set the scissor region of the viewport.
	const D3D12_RECT scissorRect =
	{
		0,                  // LONG    left
		0,                  // LONG    top
		LONG(clientWidth),  // LONG    right
		LONG(clientHeight), // LONG    bottom
	};
	pCmdList->RSSetScissorRects(1, &scissorRect);

	// Draw the bound geometry.
	pCmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	RenderBase::EndFrame(m_renderBase, true);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Shutdown()
{
	// Clean up the application render resources.
	m_pRootSignature.Reset();
	m_pGfxPipeline.Reset();
	m_pQuadVertexBuffer.Reset();
	m_pQuadIndexBuffer.Reset();
	m_pConstBuffer.Reset();
	m_pVertexShaderDescHeap.Reset();

	// Clean up the common render resources last.
	RenderBase::Destroy(m_renderBase);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowResized(DemoFramework::Window* const pWindow, const uint32_t previousWidth, const uint32_t previousHeight)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(previousWidth);
	DF_UNUSED(previousHeight);

	m_resizeSwapChain = true;
}

//---------------------------------------------------------------------------------------------------------------------

const char* SampleApp::GetAppName() const
{
	return APP_NAME;
}

//---------------------------------------------------------------------------------------------------------------------

const char* SampleApp::GetLogFilename() const
{
	return LOG_FILE;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_loadShaders()
{
	m_pVertexShader = LoadShaderFromFile("basic.vertex.hlsl.bin");
	if(!m_pVertexShader)
	{
		return false;
	}

	m_pPixelShader = LoadShaderFromFile("basic.pixel.hlsl.bin");
	if(!m_pPixelShader)
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_createGfxPipeline()
{
	using namespace DemoFramework;

	LOG_WRITE("Creating graphics pipeline resources ...");

	const D3D12_DESCRIPTOR_RANGE descRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	D3D12_ROOT_PARAMETER rootParam;
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParam.DescriptorTable.NumDescriptorRanges = 1;
	rootParam.DescriptorTable.pDescriptorRanges = &descRange;

	const D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	const D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
	{
		1,            // UINT NumParameters
		&rootParam,   // const D3D12_ROOT_PARAMETER *pParameters
		0,            // UINT NumStaticSamplers
		nullptr,      // const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers
		rootSigFlags, // D3D12_ROOT_SIGNATURE_FLAGS Flags
	};

	// Create the pipeline root signature.
	m_pRootSignature = D3D12::CreateRootSignature(m_renderBase.pDevice, rootSigDesc);
	if(!m_pRootSignature)
	{
		return false;
	}

	const D3D12_SHADER_BYTECODE vertexShaderBytecode =
	{
		m_pVertexShader->GetBufferPointer(), // const void *pShaderBytecode
		m_pVertexShader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	const D3D12_SHADER_BYTECODE pixelShaderBytecode =
	{
		m_pPixelShader->GetBufferPointer(), // const void *pShaderBytecode
		m_pPixelShader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	const D3D12_RENDER_TARGET_BLEND_DESC targetBlendState =
	{
		false,                         // BOOL BlendEnable
		false,                        // BOOL LogicOpEnable
		D3D12_BLEND_SRC_ALPHA,        // D3D12_BLEND SrcBlend
		D3D12_BLEND_INV_SRC_ALPHA,    // D3D12_BLEND DestBlend
		D3D12_BLEND_OP_ADD,           // D3D12_BLEND_OP BlendOp
		D3D12_BLEND_ONE,              // D3D12_BLEND SrcBlendAlpha
		D3D12_BLEND_ONE,              // D3D12_BLEND DestBlendAlpha
		D3D12_BLEND_OP_ADD,           // D3D12_BLEND_OP BlendOpAlpha
		D3D12_LOGIC_OP_NOOP,          // D3D12_LOGIC_OP LogicOp
		D3D12_COLOR_WRITE_ENABLE_ALL, // UINT8 RenderTargetWriteMask
	};

	const D3D12_BLEND_DESC blendState =
	{
		false,                // BOOL AlphaToCoverageEnable
		false,                // BOOL IndependentBlendEnable
		{ targetBlendState }, // D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[ 8 ]
	};

	const D3D12_RASTERIZER_DESC rasterizerState =
	{
		D3D12_FILL_MODE_SOLID,                     // D3D12_FILL_MODE FillMode
		D3D12_CULL_MODE_BACK,                      // D3D12_CULL_MODE CullMode
		false,                                     // BOOL FrontCounterClockwise
		0,                                         // INT DepthBias
		0.0f,                                      // FLOAT DepthBiasClamp
		0.0f,                                      // FLOAT SlopeScaledDepthBias
		true,                                      // BOOL DepthClipEnable
		false,                                     // BOOL MultisampleEnable
		false,                                     // BOOL AntialiasedLineEnable
		0,                                         // UINT ForcedSampleCount
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster
	};

	const D3D12_DEPTH_STENCIL_DESC depthStencilState =
	{
		true,                             // BOOL DepthEnable
		D3D12_DEPTH_WRITE_MASK_ALL,       // D3D12_DEPTH_WRITE_MASK DepthWriteMask
		D3D12_COMPARISON_FUNC_LESS_EQUAL, // D3D12_COMPARISON_FUNC DepthFunc
		false,                            // BOOL StencilEnable
		0,                                // UINT8 StencilReadMask
		0,                                // UINT8 StencilWriteMask
		{},                               // D3D12_DEPTH_STENCILOP_DESC FrontFace
		{},                               // D3D12_DEPTH_STENCILOP_DESC BackFace
	};

	const D3D12_INPUT_ELEMENT_DESC vertexPositionElement =
	{
		"POSITION",                                 // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, pos),                      // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};

	const D3D12_INPUT_ELEMENT_DESC vertexColorElement =
	{
		"COLOR",                                    // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32A32_FLOAT,             // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, color),                    // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};

	const D3D12_INPUT_ELEMENT_DESC inputElements[] =
	{
		vertexPositionElement,
		vertexColorElement,
	};

	const D3D12_INPUT_LAYOUT_DESC inputLayout =
	{
		inputElements,                  // const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs
		DF_ARRAY_LENGTH(inputElements), // UINT NumElements
	};

	const D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPipelineDesc =
	{
		m_pRootSignature.Get(),                      // ID3D12RootSignature *pRootSignature
		vertexShaderBytecode,                        // D3D12_SHADER_BYTECODE VS
		pixelShaderBytecode,                         // D3D12_SHADER_BYTECODE PS
		{},                                          // D3D12_SHADER_BYTECODE DS
		{},                                          // D3D12_SHADER_BYTECODE HS
		{},                                          // D3D12_SHADER_BYTECODE GS
		{},                                          // D3D12_STREAM_OUTPUT_DESC StreamOutput
		blendState,                                  // D3D12_BLEND_DESC BlendState
		UINT_MAX,                                    // UINT SampleMask
		rasterizerState,                             // D3D12_RASTERIZER_DESC RasterizerState
		depthStencilState,                           // D3D12_DEPTH_STENCIL_DESC DepthStencilState
		inputLayout,                                 // D3D12_INPUT_LAYOUT_DESC InputLayout
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED, // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,      // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType
		1,                                           // UINT NumRenderTargets
		{ APP_BACK_BUFFER_FORMAT },                  // DXGI_FORMAT RTVFormats[ 8 ]
		APP_DEPTH_BUFFER_FORMAT,                     // DXGI_FORMAT DSVFormat
		defaultSampleDesc,                           // DXGI_SAMPLE_DESC SampleDesc
		0,                                           // UINT NodeMask
		{},                                          // D3D12_CACHED_PIPELINE_STATE CachedPSO
		D3D12_PIPELINE_STATE_FLAG_NONE,              // D3D12_PIPELINE_STATE_FLAGS Flags
	};

	// Create the graphics pipeline state.
	m_pGfxPipeline = D3D12::CreatePipelineState(m_renderBase.pDevice, gfxPipelineDesc);
	if(!m_pGfxPipeline)
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_createQuadGeometry()
{
	using namespace DemoFramework;

	// Construct the geometry for a quad
	const Vertex triangleVertices[] =
	{	//      x      y     z         r     g     b     a
		{ { -0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
	};
	const uint16_t triangleIndices[] = { 0, 1, 2, 2, 1, 3 };

	LOG_WRITE("Creating geometry resources ...");

	const D3D12_RESOURCE_DESC vertexBufferDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER,            // D3D12_RESOURCE_DIMENSION Dimension
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // UINT64 Alignment
		sizeof(triangleVertices),                   // UINT64 Width
		1,                                          // UINT Height
		1,                                          // UINT16 DepthOrArraySize
		1,                                          // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,                        // DXGI_FORMAT Format
		defaultSampleDesc,                          // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,             // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_NONE,                   // D3D12_RESOURCE_FLAGS Flags
	};

	// Create the vertex buffer.
	m_pQuadVertexBuffer = D3D12::CreateCommittedResource(
		m_renderBase.pDevice,
		vertexBufferDesc,
		defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	if(!m_pQuadVertexBuffer)
	{
		return false;
	}

	// Create the staging vertex buffer.
	D3D12::ResourcePtr pStagingVertexBuffer = D3D12::CreateCommittedResource(
		m_renderBase.pDevice,
		vertexBufferDesc,
		uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	if(!pStagingVertexBuffer)
	{
		return false;
	}

	void* pStagingVertexData = nullptr;

	// Map the staging vertex buffer with CPU read access disabled.
	const HRESULT mapVertexBufferResult = pStagingVertexBuffer->Map(0, &disabledCpuReadRange, &pStagingVertexData);
	if(mapVertexBufferResult != S_OK)
	{
		LOG_ERROR("Failed to map staging vertex buffer; result='0x%08" PRIX32 "'", mapVertexBufferResult);
		return false;
	}

	// Copy the vertex data into the staging buffer, then unmap it.
	memcpy(pStagingVertexData, triangleVertices, sizeof(triangleVertices));
	pStagingVertexBuffer->Unmap(0, nullptr);

	const D3D12_RESOURCE_DESC indexBufferDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER,            // D3D12_RESOURCE_DIMENSION Dimension
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // UINT64 Alignment
		sizeof(triangleIndices),                    // UINT64 Width
		1,                                          // UINT Height
		1,                                          // UINT16 DepthOrArraySize
		1,                                          // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,                        // DXGI_FORMAT Format
		defaultSampleDesc,                          // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,             // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_NONE,                   // D3D12_RESOURCE_FLAGS Flags
	};

	// Create the index buffer.
	m_pQuadIndexBuffer = D3D12::CreateCommittedResource(
		m_renderBase.pDevice,
		indexBufferDesc,
		defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	if(!m_pQuadIndexBuffer)
	{
		return false;
	}

	// Create the staging index buffer.
	D3D12::ResourcePtr pStagingIndexBuffer = D3D12::CreateCommittedResource(
		m_renderBase.pDevice,
		indexBufferDesc,
		uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	if(!pStagingIndexBuffer)
	{
		return false;
	}

	void* pStagingIndexData = nullptr;

	// Map the staging index buffer with CPU read access disabled.
	const HRESULT mapIndexBufferResult = pStagingIndexBuffer->Map(0, &disabledCpuReadRange, &pStagingIndexData);
	if(mapIndexBufferResult != S_OK)
	{
		LOG_ERROR("Failed to map staging index buffer; result='0x%08" PRIX32 "'", mapIndexBufferResult);
		return false;
	}

	// Copy the index data into the staging buffer, then unmap it.
	memcpy(pStagingIndexData, triangleIndices, sizeof(triangleIndices));
	pStagingIndexBuffer->Unmap(0, nullptr);

	// Create the staging command synchronization primitive so we don't kill resources while they're currently in use.
	D3D12::Sync tempCmdSync;
	if(!D3D12::Sync::Create(tempCmdSync, m_renderBase.pDevice, D3D12_FENCE_FLAG_NONE))
	{
		return false;
	}

	// Create the staging command allocator.
	D3D12::CommandAllocatorPtr pTempCmdAlloc = D3D12::CreateCommandAllocator(m_renderBase.pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	if(!pTempCmdAlloc)
	{
		return false;
	}

	// Create the staging command list.
	D3D12::GraphicsCommandListPtr pTempCmdList = D3D12::CreateGraphicsCommandList(m_renderBase.pDevice, pTempCmdAlloc, D3D12_COMMAND_LIST_TYPE_DIRECT, 0);
	if(!pTempCmdList)
	{
		return false;
	}

	D3D12_RESOURCE_BARRIER vertexBufferBarrier, indexBufferBarrier;

	vertexBufferBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	vertexBufferBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	vertexBufferBarrier.Transition.pResource = m_pQuadVertexBuffer.Get();
	vertexBufferBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	vertexBufferBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	vertexBufferBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	indexBufferBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	indexBufferBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	indexBufferBarrier.Transition.pResource = m_pQuadIndexBuffer.Get();
	indexBufferBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	indexBufferBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	indexBufferBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

	const D3D12_RESOURCE_BARRIER bufferBarriers[] =
	{
		vertexBufferBarrier,
		indexBufferBarrier,
	};

	// Before rendering begins, use a temporary command list to copy all static buffer data.
	pTempCmdList->CopyResource(m_pQuadVertexBuffer.Get(), pStagingVertexBuffer.Get());
	pTempCmdList->CopyResource(m_pQuadIndexBuffer.Get(), pStagingIndexBuffer.Get());
	pTempCmdList->ResourceBarrier(DF_ARRAY_LENGTH(bufferBarriers), bufferBarriers);
	pTempCmdList->Close();

	ID3D12CommandList* pCmdLists[] =
	{
		pTempCmdList.Get(),
	};

	// Begin executing the staging command list.
	m_renderBase.pCmdQueue->ExecuteCommandLists(1, pCmdLists);

	// Add a synchronization signal after the staging command list, then wait for it.
	D3D12::Sync::Signal(tempCmdSync, m_renderBase.pCmdQueue);
	D3D12::Sync::Wait(tempCmdSync);

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_createConstBuffer()
{
	using namespace DemoFramework;

	LOG_WRITE("Creating constant buffer resources ...");

	// Constant buffers are required to have a size that is 256-byte aligned.
	const D3D12_RESOURCE_DESC constBufferDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER,            // D3D12_RESOURCE_DIMENSION Dimension
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // UINT64 Alignment
		(sizeof(DirectX::XMMATRIX) + 255) & ~255,   // UINT64 Width
		1,                                          // UINT Height
		1,                                          // UINT16 DepthOrArraySize
		1,                                          // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,                        // DXGI_FORMAT Format
		defaultSampleDesc,                          // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,             // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_NONE,                   // D3D12_RESOURCE_FLAGS Flags
	};

	// Create the constant buffer.
	m_pConstBuffer = D3D12::CreateCommittedResource(
		m_renderBase.pDevice,
		constBufferDesc,
		defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	if(!m_pConstBuffer)
	{
		return false;
	}

	// Create each staging constant buffer.
	for(size_t i = 0; i < APP_BACK_BUFFER_COUNT; ++i)
	{
		m_pStagingConstBuffer[i] = D3D12::CreateCommittedResource(
			m_renderBase.pDevice,
			constBufferDesc,
			uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ);
		if(!m_pStagingConstBuffer[i])
		{
			return false;
		}
	}

	const D3D12_DESCRIPTOR_HEAP_DESC vertexShaderDescHeapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    // D3D12_DESCRIPTOR_HEAP_TYPE Type
		1,                                         // UINT NumDescriptors
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		0,                                         // UINT NodeMask
	};

	// Create the descriptor heap for the vertex shader resource inputs.
	m_pVertexShaderDescHeap = D3D12::CreateDescriptorHeap(m_renderBase.pDevice, vertexShaderDescHeapDesc);
	if(!m_pVertexShaderDescHeap)
	{
		return false;
	}

	const D3D12_CONSTANT_BUFFER_VIEW_DESC constBufferViewDesc =
	{
		m_pConstBuffer->GetGPUVirtualAddress(),    // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		uint32_t(m_pConstBuffer->GetDesc().Width), // UINT SizeInBytes
	};

	// Create the constant buffer view for the vertex shader.
	m_renderBase.pDevice->CreateConstantBufferView(&constBufferViewDesc, m_pVertexShaderDescHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
