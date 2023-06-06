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

#define APP_NAME "PBR Sample"
#define LOG_FILE "pbr-sample.log"

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

	// Initialize the common rendering resources.
	m_renderBase = D3D12::RenderBase::Create(
		hWnd,
		clientWidth,
		clientHeight,
		APP_BACK_BUFFER_COUNT,
		APP_BACK_BUFFER_FORMAT,
		APP_DEPTH_BUFFER_FORMAT);
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

	// Draw the GUI.
	m_gui->Render(pCmdList);

	m_renderBase->EndFrame(true);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Shutdown()
{
	// Clean up the application render resources.
	m_vertexShaderDescHeap.Reset();
	m_constBuffer.Reset();

	for(size_t i = 0; i < DF_SWAP_CHAIN_BUFFER_MAX_COUNT; ++i)
	{
		m_stagingConstBuffer[i].Reset();
	}

	// Clean up the common render resources last.
	m_gui.reset();
	m_renderBase.reset();
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

bool SampleApp::prv_createConstBuffer()
{
	using namespace DemoFramework;

	LOG_WRITE("Creating constant buffer resources ...");

	const D3D12::DevicePtr& pDevice = m_renderBase->GetDevice();

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

	const D3D12_DESCRIPTOR_HEAP_DESC vertexShaderDescHeapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    // D3D12_DESCRIPTOR_HEAP_TYPE Type
		1,                                         // UINT NumDescriptors
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		0,                                         // UINT NodeMask
	};

	// Create the descriptor heap for the vertex shader resource inputs.
	m_vertexShaderDescHeap = D3D12::CreateDescriptorHeap(pDevice, vertexShaderDescHeapDesc);
	if(!m_vertexShaderDescHeap)
	{
		return false;
	}

	const D3D12_CONSTANT_BUFFER_VIEW_DESC constBufferViewDesc =
	{
		m_constBuffer->GetGPUVirtualAddress(),    // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
		uint32_t(m_constBuffer->GetDesc().Width), // UINT SizeInBytes
	};

	// Create the constant buffer view for the vertex shader.
	pDevice->CreateConstantBufferView(&constBufferViewDesc, m_vertexShaderDescHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
