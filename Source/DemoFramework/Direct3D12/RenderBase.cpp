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

#include "RenderBase.hpp"

#include "../Application/Log.hpp"

#include "LowLevel/Adapter.hpp"
#include "LowLevel/CommandAllocator.hpp"
#include "LowLevel/CommandQueue.hpp"
#include "LowLevel/DescriptorHeap.hpp"
#include "LowLevel/Device.hpp"
#include "LowLevel/Event.hpp"
#include "LowLevel/Factory.hpp"
#include "LowLevel/Fence.hpp"
#include "LowLevel/GraphicsCommandList.hpp"
#include "LowLevel/Resource.hpp"
#include "LowLevel/SwapChain.hpp"

#include <DirectXMath.h>

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::RenderBase::Ptr DemoFramework::D3D12::RenderBase::Create(
	HWND hWnd,
	const uint32_t backBufferWidth,
	const uint32_t backBufferHeight,
	const uint32_t backBufferCount,
	const DXGI_FORMAT backBufferFormat,
	const DXGI_FORMAT depthFormat)
{
	if(!hWnd || backBufferWidth == 0 || backBufferHeight == 0 || backBufferCount == 0 || backBufferFormat == DXGI_FORMAT_UNKNOWN)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}
	else if(backBufferCount > DF_SWAP_CHAIN_BUFFER_MAX_COUNT)
	{
		LOG_ERROR("Exceeded maximum swap chain buffer count; value='%" PRIu32 "', maximum='%" PRIu32 "'", backBufferCount, DF_SWAP_CHAIN_BUFFER_MAX_COUNT);
		return Ptr();
	}

	Ptr output = std::make_shared<RenderBase>();

	LOG_WRITE("Creating D3D12 base resources ...");

	output->m_bufferCount = backBufferCount;
	output->m_bufferIndex = 0;

	output->m_swapChainFlags = 0;
	output->m_presentTearingFlag = 0;

	output->m_depthFormat = depthFormat;

	// Verify the DirectX math library is supported on this CPU.
	if(!DirectX::XMVerifyCPUSupport())
	{
		LOG_ERROR("DirectX math library is not supported on this CPU");
		return Ptr();
	}

	// Creat the DXGI factor.
	D3D12::FactoryPtr pFactory = D3D12::CreateFactory();
	if(!pFactory)
	{
		return Ptr();
	}

	// Check for variable refresh rate support when vsync is off (nvidia G-Sync, AMD Freesync)
	{
		Microsoft::WRL::ComPtr<IDXGIFactory5> pStagingFactory;
		if(SUCCEEDED(pFactory.As(&pStagingFactory)))
		{
			BOOL allowTearing = false;

			const HRESULT checkResult = pStagingFactory->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing,
				sizeof(allowTearing)
			);

			if(SUCCEEDED(checkResult) && allowTearing)
			{
				output->m_swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
				output->m_presentTearingFlag = DXGI_PRESENT_ALLOW_TEARING;
			}
		}
	}

	// Find a usable DXGI adapter.
	D3D12::AdapterPtr pAdapter = D3D12::QueryAdapter(pFactory, false);
	if(!pAdapter)
	{
		return Ptr();
	}

	// Create the D3D12 device.
	output->m_device = D3D12::CreateDevice(pAdapter);
	if(!output->m_device)
	{
		return Ptr();
	}

	// Determine the root signature version.
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE feature;
		feature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if(FAILED(output->m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature, sizeof(feature))))
		{
			feature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		output->m_rootSigVersion = feature.HighestVersion;
	}

	const D3D12_COMMAND_QUEUE_DESC cmdQueueDesc =
	{
		D3D12_COMMAND_LIST_TYPE_DIRECT,      // D3D12_COMMAND_LIST_TYPE Type
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, // INT Priority
		D3D12_COMMAND_QUEUE_FLAG_NONE,       // D3D12_COMMAND_QUEUE_FLAGS Flags
		0,                                   // UINT NodeMask
	};

	// Create a command queue.
	output->m_cmdQueue = D3D12::CreateCommandQueue(output->m_device, cmdQueueDesc);
	if(!output->m_cmdQueue)
	{
		return Ptr();
	}

	const D3D12_DESCRIPTOR_HEAP_DESC depthDescHeapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV,  // D3D12_DESCRIPTOR_HEAP_TYPE Type
		1,                               // UINT NumDescriptors
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE, // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		0,                               // UINT NodeMask
	};

	// Create the descriptor heap for the depth buffer.
	output->m_depthDescHeap = D3D12::CreateDescriptorHeap(output->m_device, depthDescHeapDesc);
	if(!output->m_depthDescHeap)
	{
		return Ptr();
	}

	const DXGI_SAMPLE_DESC sampleDesc =
	{
		1, // UINT Count
		0, // UINT Quality
	};

	const DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
	{
		backBufferWidth,               // UINT Width
		backBufferHeight,              // UINT Height
		backBufferFormat,              // DXGI_FORMAT Format
		false,                         // BOOL Stereo
		sampleDesc,                    // DXGI_SAMPLE_DESC SampleDesc
		DXGI_USAGE_BACK_BUFFER,        // DXGI_USAGE BufferUsage
		backBufferCount,               // UINT BufferCount
		DXGI_SCALING_STRETCH,          // DXGI_SCALING Scaling
		DXGI_SWAP_EFFECT_FLIP_DISCARD, // DXGI_SWAP_EFFECT SwapEffect
		DXGI_ALPHA_MODE_IGNORE,        // DXGI_ALPHA_MODE AlphaMode
		output->m_swapChainFlags,      // UINT Flags
	};

	// Create a swap chain for the window.
	output->m_swapChain = D3D12::CreateSwapChain(pFactory, output->m_cmdQueue, swapChainDesc, hWnd);
	if(!output->m_swapChain)
	{
		return Ptr();
	}

	// Create the frame draw fence.
	output->m_drawFence = D3D12::CreateFence(output->m_device, D3D12_FENCE_FLAG_NONE, 0);
	if(!output->m_drawFence)
	{
		return Ptr();
	}

	// Create the frame draw event.
	output->m_drawEvent = D3D12::CreateEvent(nullptr, false, false, nullptr);
	if(!output->m_drawEvent)
	{
		return Ptr();
	}

	// Get render target views for each back buffer in the swap chain.
	output->m_backBuffer = BackBuffer::Create(output->m_device, output->m_swapChain, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0);
	if(!output->m_backBuffer)
	{
		return Ptr();
	}

	// Initialize the upload context.
	output->m_uploadContext = GraphicsCommandContext::Create(output->m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	if(!output->m_uploadContext)
	{
		return Ptr();
	}

	// Create per-frame resources.
	for(size_t bufferIndex = 0; bufferIndex < backBufferCount; ++bufferIndex)
	{
		// Initialize the draw context for the current buffer.
		output->m_drawContext[bufferIndex] = GraphicsCommandContext::Create(output->m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		if(!output->m_drawContext[bufferIndex])
		{
			return Ptr();
		}

		output->m_fenceMarker[0] = 0;
	}

	// Reset the upload context so it's immediately ready to use.
	output->m_uploadContext->Reset();

	output->m_nextFenceMarker = output->m_fenceMarker[0] + 1;
	output->m_initialized = true;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::RenderBase::BeginFrame()
{
	if(!m_depthBuffer)
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
		m_swapChain->GetDesc1(&swapChainDesc);

		const DXGI_SAMPLE_DESC depthSampleDesc =
		{
			1, // UINT Count
			0, // UINT Quality
		};

		const D3D12_RESOURCE_DESC depthBufferDesc =
		{
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,         // D3D12_RESOURCE_DIMENSION Dimension
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // UINT64 Alignment
			swapChainDesc.Width,                        // UINT64 Width
			swapChainDesc.Height,                       // UINT Height
			1,                                          // UINT16 DepthOrArraySize
			1,                                          // UINT16 MipLevels
			m_depthFormat,                              // DXGI_FORMAT Format
			depthSampleDesc,                            // DXGI_SAMPLE_DESC SampleDesc
			D3D12_TEXTURE_LAYOUT_UNKNOWN,               // D3D12_TEXTURE_LAYOUT Layout
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,    // D3D12_RESOURCE_FLAGS Flags
		};

		const D3D12_HEAP_PROPERTIES depthHeapProps =
		{
			D3D12_HEAP_TYPE_DEFAULT,         // D3D12_HEAP_TYPE Type
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
			D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
			0,                               // UINT CreationNodeMask
			0,                               // UINT VisibleNodeMask
		};

		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = m_depthFormat;
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;

		// Create the depth buffer resource.
		m_depthBuffer = D3D12::CreateCommittedResource(
			m_device,
			depthBufferDesc,
			depthHeapProps,
			D3D12_HEAP_FLAG_NONE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClearValue
		);
		assert(m_depthBuffer != nullptr);

		D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {};
		depthViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthViewDesc.Flags = D3D12_DSV_FLAG_NONE;

		// Create the depth/stencil view.
		m_device->CreateDepthStencilView(
			m_depthBuffer.Get(),
			&depthViewDesc,
			m_depthDescHeap->GetCPUDescriptorHandleForHeapStart()
		);
	}

	m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	const GraphicsCommandContext::Ptr& drawContext = m_drawContext[m_bufferIndex];

	ID3D12GraphicsCommandList* const pCmdList = drawContext->GetCmdList().Get();
	ID3D12Resource* const pBackBufferRtv = m_backBuffer->GetRtv(m_bufferIndex).Get();

	const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_backBuffer->GetCpuDescHandle(m_bufferIndex);

	// Wait for the command queue to finish processing the current back buffer.
	prv_waitForFrame(m_bufferIndex);

	// Reset the command context.
	drawContext->Reset();

	D3D12_RESOURCE_BARRIER beginBarrier;
	beginBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	beginBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	beginBarrier.Transition.pResource = pBackBufferRtv;
	beginBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	beginBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	beginBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// Transition the back buffer to a render target so we can draw to it.
	pCmdList->ResourceBarrier(1, &beginBarrier);

	const float32_t clearColor[4] = { 0.0f, 0.1f, 0.175f, 1.0f };

	// Clear the back buffer and depth buffer.
	pCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	pCmdList->ClearDepthStencilView(m_depthDescHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::RenderBase::EndFrame(const bool vsync)
{
	const uint32_t presentInterval = vsync ? 1 : 0;
	const uint32_t presentFlags = !vsync ? m_presentTearingFlag : 0;

	const GraphicsCommandContext::Ptr& drawContext = m_drawContext[m_bufferIndex];

	ID3D12GraphicsCommandList* const pCmdList = drawContext->GetCmdList().Get();
	ID3D12Resource* const pBackBufferRtv = m_backBuffer->GetRtv(m_bufferIndex).Get();

	D3D12_RESOURCE_BARRIER endBarrier;
	endBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	endBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	endBarrier.Transition.pResource = pBackBufferRtv;
	endBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	endBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	endBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	// Transition the back buffer from a render target to a surface that can be presented to the swap chain.
	pCmdList->ResourceBarrier(1, &endBarrier);

	drawContext->Submit(m_cmdQueue);

	// Present the current back buffer to the screen and flip to the next buffer.
	m_swapChain->Present(presentInterval, presentFlags);

	// Add a signal to the command queue so we know when the current back buffer is no longer in use.
	m_cmdQueue->Signal(m_drawFence.Get(), m_nextFenceMarker);

	m_fenceMarker[m_bufferIndex] = m_nextFenceMarker;
	++m_nextFenceMarker;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::RenderBase::ResizeSwapChain()
{
	// Wait for each pending present to complete since no back buffer
	// resources can be in use when resizing the swap chain.
	m_cmdQueue->Signal(m_drawFence.Get(), m_nextFenceMarker);
	m_drawFence->SetEventOnCompletion(m_nextFenceMarker, m_drawEvent->GetHandle());
	::WaitForSingleObject(m_drawEvent->GetHandle(), INFINITE);

	++m_nextFenceMarker;

	// Clear the existing back buffer resources.
	m_backBuffer.reset();

	// Resize the swap chain, preserving the existing buffer count and format,
	// and using the window's width and height for the new buffers
	m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, m_swapChainFlags);

	// Create new back buffer resources from the resized swap chain.
	m_backBuffer = BackBuffer::Create(m_device, m_swapChain, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0);

	// Clear the depth buffer so it's created again at the beginning of the frame.
	m_depthBuffer.Reset();

	return (m_backBuffer != nullptr);
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::RenderBase::SetBackBufferAsRenderTarget()
{
	const D3D12_CPU_DESCRIPTOR_HANDLE depthTargetHandle = m_depthDescHeap->GetCPUDescriptorHandleForHeapStart();
	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandles[] =
	{
		m_backBuffer->GetCpuDescHandle(m_bufferIndex),
	};

	const GraphicsCommandContext::Ptr& drawContext = m_drawContext[m_bufferIndex];
	ID3D12GraphicsCommandList* const pCmdList = drawContext->GetCmdList().Get();

	// Set the back buffer as the render target along with the base depth target.
	pCmdList->OMSetRenderTargets(1, renderTargetHandles, false, &depthTargetHandle);
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::RenderBase::prv_waitForFrame(const uint32_t bufferIndex)
{
	const uint64_t currentMarker = m_fenceMarker[bufferIndex];

	m_fenceMarker[bufferIndex] = 0;

	if(m_drawFence->GetCompletedValue() < currentMarker)
	{
		// Bind the event to the current buffer's fence value.
		const HRESULT setEventResult = m_drawFence->SetEventOnCompletion(currentMarker, m_drawEvent->GetHandle());
		assert(setEventResult == S_OK); (void) setEventResult;

		// Wait for the event to be triggered by the fence.
		::WaitForSingleObject(m_drawEvent->GetHandle(), INFINITE);
	}
}

//---------------------------------------------------------------------------------------------------------------------
