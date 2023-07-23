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

#include <DemoFramework/Direct3D12/LowLevel/CommandAllocator.hpp>
#include <DemoFramework/Direct3D12/LowLevel/DescriptorHeap.hpp>
#include <DemoFramework/Direct3D12/LowLevel/GraphicsCommandList.hpp>
#include <DemoFramework/Direct3D12/LowLevel/PipelineState.hpp>
#include <DemoFramework/Direct3D12/LowLevel/Resource.hpp>
#include <DemoFramework/Direct3D12/LowLevel/RootSignature.hpp>

#include <DemoFramework/Direct3D12/Shader.hpp>
#include <DemoFramework/Direct3D12/Sync.hpp>

#include <imgui.h>

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

	LOG_WRITE("Initializing base render resources ...");

	D3D12::RenderConfig renderConfig = D3D12::RenderConfig::Invalid;
	renderConfig.backBufferWidth = clientWidth;
	renderConfig.backBufferHeight = clientHeight;
	renderConfig.backBufferCount = APP_BACK_BUFFER_COUNT;
	renderConfig.cbvSrvUavDescCount = 100;
	renderConfig.rtvDescCount = APP_BACK_BUFFER_COUNT;
	renderConfig.dsvDescCount = 1;
	renderConfig.backBufferFormat = APP_BACK_BUFFER_FORMAT;
	renderConfig.depthFormat = APP_DEPTH_BUFFER_FORMAT;

	// Initialize the common rendering resources.
	m_renderBase = D3D12::RenderBase::Create(hWnd, renderConfig);
	if(!m_renderBase)
	{
		return false;
	}

	LOG_WRITE("Initializing GUI resources ...");

	// Initialize the on-screen GUI.
	m_gui = D3D12::Gui::Create(m_renderBase->GetDevice(), APP_NAME, APP_BACK_BUFFER_COUNT, APP_BACK_BUFFER_FORMAT);
	if(!m_gui)
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

	// Set the size of the GUI display area.
	m_gui->SetDisplaySize(pWindow->GetClientWidth(), pWindow->GetClientHeight());

	// Initialize the frame timer at the end so the initial timestamp is not
	// influenced by the time it takes to create the application resources.
	m_frameTimer.Initialize();

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::Update()
{
	using namespace DemoFramework;
	using namespace DirectX;

	if(m_resizeSwapChain)
	{
		const bool resizeSwapChainResult = m_renderBase->ResizeSwapChain();
		assert(resizeSwapChainResult == true); (void) resizeSwapChainResult;

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

	m_gui->Update(
		m_frameTimer.GetDeltaTime(),
		[](ImGuiContext* const pGuiContext)
		{
			// Always set the ImGui context before calling any other ImGui functions.
			ImGui::SetCurrentContext(pGuiContext);

			// Do custom GUI drawing code here.
		}
	);

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Render()
{
	using namespace DemoFramework;

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();
	const uint32_t bufferIndex = m_renderBase->GetBufferIndex();

	m_renderBase->BeginFrame();
	m_renderBase->SetBackBufferAsRenderTarget();

	// The swap chain buffer index is updated during the call to BeginFrame(),
	// so we need to wait until *after* that to get the current command list.
	ID3D12GraphicsCommandList* const pCmdList = m_renderBase->GetDrawContext()->GetCmdList().Get();

	void* pConstData = nullptr;

	// Map the staging buffer to CPU-accessible memory (write-only access) and copy the constant data to it.
	const D3D12_RANGE dummyReadRange = {0, 0};
	const HRESULT result = m_stagingConstBuffer[bufferIndex]->Map(0, &dummyReadRange, &pConstData);
	if(result == S_OK)
	{
		memcpy(pConstData, &m_wvpMatrix, sizeof(m_wvpMatrix));
		m_stagingConstBuffer[bufferIndex]->Unmap(0, nullptr);
	}

	ID3D12DescriptorHeap* const pDescHeaps[] =
	{
		m_renderBase->GetCbvSrvUavAllocator()->GetHeap().Get(),
	};

	D3D12_RESOURCE_BARRIER constBufferBeginBarrier, constBufferEndBarrier;

	constBufferBeginBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	constBufferBeginBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	constBufferBeginBarrier.Transition.pResource = m_constBuffer.Get();
	constBufferBeginBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	constBufferBeginBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	constBufferBeginBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

	constBufferEndBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	constBufferEndBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	constBufferEndBarrier.Transition.pResource = m_constBuffer.Get();
	constBufferEndBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	constBufferEndBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	constBufferEndBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	// Initiate a copy of constant buffer data from staging memory.
	pCmdList->ResourceBarrier(1, &constBufferBeginBarrier);
	pCmdList->CopyResource(m_constBuffer.Get(), m_stagingConstBuffer[bufferIndex].Get());
	pCmdList->ResourceBarrier(1, &constBufferEndBarrier);

	// Set the graphics pipeline state.
	pCmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	pCmdList->SetDescriptorHeaps(_countof(pDescHeaps), pDescHeaps);
	pCmdList->SetGraphicsRootDescriptorTable(0, m_vertexUniformDescriptor.gpuHandle);
	pCmdList->SetPipelineState(m_gfxPipeline.Get());

	const D3D12_VERTEX_BUFFER_VIEW vertexBufferView =
	{
		m_quadVertexBuffer->GetGPUVirtualAddress(),    // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		uint32_t(m_quadVertexBuffer->GetDesc().Width), // UINT SizeInBytes
		sizeof(Vertex),                                 // UINT StrideInBytes
	};

	const D3D12_INDEX_BUFFER_VIEW indexBufferView =
	{
		m_quadIndexBuffer->GetGPUVirtualAddress(),    // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		uint32_t(m_quadIndexBuffer->GetDesc().Width), // UINT SizeInBytes
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

	// Draw the GUI.
	m_gui->Render(pCmdList);

	m_renderBase->EndFrame(true);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Shutdown()
{
	// Do explicit shutdown tasks here.
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowResized(DemoFramework::Window* const pWindow, const uint32_t previousWidth, const uint32_t previousHeight)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(previousWidth);
	DF_UNUSED(previousHeight);

	m_gui->SetDisplaySize(pWindow->GetClientWidth(), pWindow->GetClientHeight());

	m_resizeSwapChain = true;
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseMove(DemoFramework::Window* const pWindow, const int32_t previousX, const int32_t previousY)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(previousX);
	DF_UNUSED(previousY);

	m_gui->SetMousePosition(pWindow->GetMouseX(), pWindow->GetMouseY());
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseWheel(DemoFramework::Window* const pWindow, const float32_t wheelDelta)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(wheelDelta);

	m_gui->SetMouseWheelDelta(wheelDelta);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseButtonPressed(DemoFramework::Window* const pWindow, const DemoFramework::MouseButton button)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(button);

	using namespace DemoFramework;

	uint32_t buttonIndex = UINT_MAX;
	switch(button)
	{
		case MouseButton::Left:   buttonIndex = 0; break;
		case MouseButton::Right:  buttonIndex = 1; break;
		case MouseButton::Middle: buttonIndex = 2; break;
		case MouseButton::X1:     buttonIndex = 3; break;
		case MouseButton::X2:     buttonIndex = 4; break;

		default:
			assert(false);
			break;
	}

	m_gui->SetMouseButtonState(buttonIndex, true);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseButtonReleased(DemoFramework::Window* const pWindow, const DemoFramework::MouseButton button)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(button);

	using namespace DemoFramework;

	uint32_t buttonIndex = UINT_MAX;
	switch(button)
	{
		case MouseButton::Left:   buttonIndex = 0; break;
		case MouseButton::Right:  buttonIndex = 1; break;
		case MouseButton::Middle: buttonIndex = 2; break;
		case MouseButton::X1:     buttonIndex = 3; break;
		case MouseButton::X2:     buttonIndex = 4; break;

		default:
			assert(false);
			break;
	}

	m_gui->SetMouseButtonState(buttonIndex, false);
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
	using namespace DemoFramework;

	m_vertexShader = D3D12::LoadShaderFromFile("shaders/basic/quad.vs.sbin");
	if(!m_vertexShader)
	{
		return false;
	}

	m_pixelShader = D3D12::LoadShaderFromFile("shaders/basic/quad.ps.sbin");
	if(!m_pixelShader)
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

	const D3D12::Device::Ptr& pDevice = m_renderBase->GetDevice();

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

	constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
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
	m_rootSignature = D3D12::CreateRootSignature(pDevice, rootSigDesc);
	if(!m_rootSignature)
	{
		return false;
	}

	const D3D12_SHADER_BYTECODE vertexShaderBytecode =
	{
		m_vertexShader->GetBufferPointer(), // const void *pShaderBytecode
		m_vertexShader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	const D3D12_SHADER_BYTECODE pixelShaderBytecode =
	{
		m_pixelShader->GetBufferPointer(), // const void *pShaderBytecode
		m_pixelShader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	constexpr D3D12_RENDER_TARGET_BLEND_DESC targetBlendState =
	{
		false,                        // BOOL BlendEnable
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

	constexpr D3D12_BLEND_DESC blendState =
	{
		false,                // BOOL AlphaToCoverageEnable
		false,                // BOOL IndependentBlendEnable
		{ targetBlendState }, // D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[ 8 ]
	};

	constexpr D3D12_RASTERIZER_DESC rasterizerState =
	{
		D3D12_FILL_MODE_SOLID,                     // D3D12_FILL_MODE FillMode
		D3D12_CULL_MODE_BACK,                      // D3D12_CULL_MODE CullMode
		false,                                     // BOOL FrontCounterClockwise
		D3D12_DEFAULT_DEPTH_BIAS,                  // INT DepthBias
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // FLOAT DepthBiasClamp
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // FLOAT SlopeScaledDepthBias
		true,                                      // BOOL DepthClipEnable
		false,                                     // BOOL MultisampleEnable
		false,                                     // BOOL AntialiasedLineEnable
		0,                                         // UINT ForcedSampleCount
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster
	};

	constexpr D3D12_DEPTH_STENCIL_DESC depthStencilState =
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

	constexpr D3D12_INPUT_ELEMENT_DESC vertexPositionElement =
	{
		"POSITION",                                 // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, pos),                      // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC vertexColorElement =
	{
		"COLOR",                                    // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32A32_FLOAT,             // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(Vertex, color),                    // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC inputElements[] =
	{
		vertexPositionElement,
		vertexColorElement,
	};

	const D3D12_INPUT_LAYOUT_DESC inputLayout =
	{
		inputElements,           // const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs
		_countof(inputElements), // UINT NumElements
	};

	const D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPipelineDesc =
	{
		m_rootSignature.Get(),                       // ID3D12RootSignature *pRootSignature
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
	m_gfxPipeline = D3D12::CreatePipelineState(pDevice, gfxPipelineDesc);
	if(!m_gfxPipeline)
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_createQuadGeometry()
{
	using namespace DemoFramework;

	const D3D12::Device::Ptr& device = m_renderBase->GetDevice();

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
	m_quadVertexBuffer = D3D12::CreateCommittedResource(
		device,
		vertexBufferDesc,
		defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	if(!m_quadVertexBuffer)
	{
		return false;
	}

	// Create the staging vertex buffer.
	D3D12::Resource::Ptr stagingVertexBuffer = D3D12::CreateCommittedResource(
		device,
		vertexBufferDesc,
		uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	if(!stagingVertexBuffer)
	{
		return false;
	}

	void* pStagingVertexData = nullptr;

	// Map the staging vertex buffer with CPU read access disabled.
	const HRESULT mapVertexBufferResult = stagingVertexBuffer->Map(0, &disabledCpuReadRange, &pStagingVertexData);
	if(mapVertexBufferResult != S_OK)
	{
		LOG_ERROR("Failed to map staging vertex buffer; result='0x%08" PRIX32 "'", mapVertexBufferResult);
		return false;
	}

	// Copy the vertex data into the staging buffer, then unmap it.
	memcpy(pStagingVertexData, triangleVertices, sizeof(triangleVertices));
	stagingVertexBuffer->Unmap(0, nullptr);

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
	m_quadIndexBuffer = D3D12::CreateCommittedResource(
		device,
		indexBufferDesc,
		defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	if(!m_quadIndexBuffer)
	{
		return false;
	}

	// Create the staging index buffer.
	D3D12::Resource::Ptr stagingIndexBuffer = D3D12::CreateCommittedResource(
		device,
		indexBufferDesc,
		uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	if(!stagingIndexBuffer)
	{
		return false;
	}

	void* pStagingIndexData = nullptr;

	// Map the staging index buffer with CPU read access disabled.
	const HRESULT mapIndexBufferResult = stagingIndexBuffer->Map(0, &disabledCpuReadRange, &pStagingIndexData);
	if(mapIndexBufferResult != S_OK)
	{
		LOG_ERROR("Failed to map staging index buffer; result='0x%08" PRIX32 "'", mapIndexBufferResult);
		return false;
	}

	// Copy the index data into the staging buffer, then unmap it.
	memcpy(pStagingIndexData, triangleIndices, sizeof(triangleIndices));
	stagingIndexBuffer->Unmap(0, nullptr);

	D3D12_RESOURCE_BARRIER vertexBufferBarrier, indexBufferBarrier;

	vertexBufferBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	vertexBufferBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	vertexBufferBarrier.Transition.pResource = m_quadVertexBuffer.Get();
	vertexBufferBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	vertexBufferBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	vertexBufferBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	indexBufferBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	indexBufferBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	indexBufferBarrier.Transition.pResource = m_quadIndexBuffer.Get();
	indexBufferBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	indexBufferBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	indexBufferBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

	const D3D12_RESOURCE_BARRIER bufferBarriers[] =
	{
		vertexBufferBarrier,
		indexBufferBarrier,
	};

	// Create the staging command synchronization primitive so we can wait for
	// all staging resource copies to complete at the end of initialization.
	D3D12::Sync::Ptr stagingCmdSync = D3D12::Sync::Create(device, D3D12_FENCE_FLAG_NONE);
	if(!stagingCmdSync)
	{
		return false;
	}

	const D3D12::GraphicsCommandContext::Ptr& uploadContext = m_renderBase->GetUploadContext();
	ID3D12GraphicsCommandList* const pUploadCmdList = uploadContext->GetCmdList().Get();

	// Before rendering begins, use a temporary command list to copy all static buffer data.
	pUploadCmdList->CopyResource(m_quadVertexBuffer.Get(), stagingVertexBuffer.Get());
	pUploadCmdList->CopyResource(m_quadIndexBuffer.Get(), stagingIndexBuffer.Get());
	pUploadCmdList->ResourceBarrier(_countof(bufferBarriers), bufferBarriers);

	// Stop recording commands in the staging command list and begin executing it.
	uploadContext->Submit(m_renderBase->GetCmdQueue());

	// Wait for the staging command list to finish executing.
	stagingCmdSync->Signal(m_renderBase->GetCmdQueue());
	stagingCmdSync->Wait();

	// Reset the command list so it can be used again.
	uploadContext->Reset();

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_createConstBuffer()
{
	using namespace DemoFramework;

	LOG_WRITE("Creating constant buffer resources ...");

	const D3D12::Device::Ptr& pDevice = m_renderBase->GetDevice();

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
	m_constBuffer = D3D12::CreateCommittedResource(
		pDevice,
		constBufferDesc,
		defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	if(!m_constBuffer)
	{
		return false;
	}

	// Create each staging constant buffer.
	for(size_t i = 0; i < APP_BACK_BUFFER_COUNT; ++i)
	{
		m_stagingConstBuffer[i] = D3D12::CreateCommittedResource(
			pDevice,
			constBufferDesc,
			uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ);
		if(!m_stagingConstBuffer[i])
		{
			return false;
		}
	}

	m_vertexUniformDescriptor = m_renderBase->GetCbvSrvUavAllocator()->Allocate();

	const D3D12_CONSTANT_BUFFER_VIEW_DESC constBufferViewDesc =
	{
		m_constBuffer->GetGPUVirtualAddress(),    // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		uint32_t(m_constBuffer->GetDesc().Width), // UINT SizeInBytes
	};

	// Create the constant buffer view for the vertex shader.
	pDevice->CreateConstantBufferView(&constBufferViewDesc, m_vertexUniformDescriptor.cpuHandle);

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
