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

#include <DemoFramework/Application/Log.hpp>

#include <DemoFramework/Direct3D12/Adapter.hpp>
#include <DemoFramework/Direct3D12/CommandAllocator.hpp>
#include <DemoFramework/Direct3D12/CommandQueue.hpp>
#include <DemoFramework/Direct3D12/DescriptorHeap.hpp>
#include <DemoFramework/Direct3D12/Device.hpp>
#include <DemoFramework/Direct3D12/Factory.hpp>
#include <DemoFramework/Direct3D12/GraphicsCommandList.hpp>
#include <DemoFramework/Direct3D12/Resource.hpp>
#include <DemoFramework/Direct3D12/SwapChain.hpp>

#include <DirectXMath.h>

//---------------------------------------------------------------------------------------------------------------------

bool RenderBase::Initialize(
	RenderBase& output,
	HWND hWnd,
	const uint32_t backBufferWidth,
	const uint32_t backBufferHeight,
	const uint32_t backBufferCount,
	const DXGI_FORMAT backBufferFormat,
	const DXGI_FORMAT depthFormat)
{
	using namespace DemoFramework;

	if(!hWnd || backBufferWidth == 0 || backBufferHeight == 0 || backBufferCount == 0 || backBufferFormat == DXGI_FORMAT_UNKNOWN)
	{
		LOG_ERROR("Invalid parameter");
		return false;
	}
	else if(backBufferCount > BACK_BUFFER_MAX_COUNT)
	{
		LOG_ERROR("Exceeded maximum back buffer count; value='%" PRIu32 "', maximum='%" PRIu32 "'", backBufferCount, BACK_BUFFER_MAX_COUNT);
		return false;
	}

	LOG_WRITE("Creating D3D12 base resources ...");

	output.bufferCount = backBufferCount;
	output.bufferIndex = 0;

	output.swapChainFlags = 0;
	output.presentTearingFlag = 0;

	// Verify the DirectX math library is supported on this CPU.
	if(!DirectX::XMVerifyCPUSupport())
	{
		LOG_ERROR("DirectX math library is not supported on this CPU");
		return false;
	}

	// Creat the DXGI factor.
	D3D12::FactoryPtr pFactory = D3D12::CreateFactory();
	if(!pFactory)
	{
		return false;
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
				output.swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
				output.presentTearingFlag = DXGI_PRESENT_ALLOW_TEARING;
			}
		}
	}

	// Find a usable DXGI adapter.
	D3D12::AdapterPtr pAdapter = D3D12::QueryAdapter(pFactory, false);
	if(!pAdapter)
	{
		return false;
	}

	// Create the D3D12 device.
	output.pDevice = D3D12::CreateDevice(pAdapter);
	if(!output.pDevice)
	{
		return false;
	}

	// Determine the root signature version.
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE feature;
		feature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if(FAILED(output.pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature, sizeof(feature))))
		{
			feature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		output.rootSigVersion = feature.HighestVersion;
	}

	const D3D12_COMMAND_QUEUE_DESC cmdQueueDesc =
	{
		D3D12_COMMAND_LIST_TYPE_DIRECT,      // D3D12_COMMAND_LIST_TYPE Type
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, // INT Priority
		D3D12_COMMAND_QUEUE_FLAG_NONE,       // D3D12_COMMAND_QUEUE_FLAGS Flags
		0,                                   // UINT NodeMask
	};

	// Create a command queue.
	output.pCmdQueue = D3D12::CreateCommandQueue(output.pDevice, cmdQueueDesc);
	if(!output.pCmdQueue)
	{
		return false;
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
		output.swapChainFlags,         // UINT Flags
	};

	// Create a swap chain for the window.
	output.pSwapChain = D3D12::CreateSwapChain(pFactory, output.pCmdQueue, swapChainDesc, hWnd);
	if(!output.pSwapChain)
	{
		return false;
	}

	const D3D12_DESCRIPTOR_HEAP_DESC depthDescHeapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV,  // D3D12_DESCRIPTOR_HEAP_TYPE Type
		1,                               // UINT NumDescriptors
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE, // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		0,                               // UINT NodeMask
	};

	// Create the descriptor heap for the depth buffer.
	output.pDepthDescHeap = D3D12::CreateDescriptorHeap(output.pDevice, depthDescHeapDesc);
	if(!output.pDepthDescHeap)
	{
		return false;
	}

	const DXGI_SAMPLE_DESC depthSampleDesc =
	{
		1, // UINT Count
		0, // UINT Quality
	};

	const D3D12_RESOURCE_DESC depthBufferDesc =
	{
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,         // D3D12_RESOURCE_DIMENSION Dimension
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // UINT64 Alignment
		backBufferWidth,                            // UINT64 Width
		backBufferHeight,                           // UINT Height
		1,                                          // UINT16 DepthOrArraySize
		1,                                          // UINT16 MipLevels
		depthFormat,                                // DXGI_FORMAT Format
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
	depthClearValue.Format = depthFormat;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;

	// Create the depth buffer resource.
	output.pDepthBuffer = D3D12::CreateCommittedResource(
		output.pDevice,
		depthBufferDesc,
		depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue);
	if(!output.pDepthBuffer)
	{
		return false;
	}


	D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {};
	depthViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	// Create the depth/stencil view.
	output.pDevice->CreateDepthStencilView(output.pDepthBuffer.Get(), &depthViewDesc, output.pDepthDescHeap->GetCPUDescriptorHandleForHeapStart());

	// Get render target views for each back buffer in the swap chain.
	if(!D3D12::BackBuffer::Create(output.backBuffer, output.pDevice, output.pSwapChain, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0))
	{
		return false;
	}

	// Create per-frame resources.
	for(size_t bufferIndex = 0; bufferIndex < backBufferCount; ++bufferIndex)
	{
		// Create a command allocator.
		output.pCmdAlloc[bufferIndex] = D3D12::CreateCommandAllocator(output.pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		if(!output.pCmdAlloc[bufferIndex])
		{
			return false;
		}

		// Create a command list.
		output.pCmdList[bufferIndex] = D3D12::CreateGraphicsCommandList(output.pDevice, output.pCmdAlloc[bufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT, 0);
		if(!output.pCmdList[bufferIndex])
		{
			return false;
		}

		// Command lists start in the recording state, so for consistency, we'll close them after they're created.
		output.pCmdList[bufferIndex]->Close();

		// Create a synchronization primitive for signaling the end of command list processing.
		if(!D3D12::Sync::Create(output.sync[bufferIndex], output.pDevice, D3D12_FENCE_FLAG_NONE))
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void RenderBase::Destroy(RenderBase& renderBase)
{
	using namespace DemoFramework;

	// Flush each command list before shutting down.
	for(size_t i = 0; i < renderBase.bufferCount; ++i)
	{
		D3D12::Sync::Wait(renderBase.sync[i]);

		renderBase.pCmdAlloc[i].Reset();
		renderBase.pCmdList[i].Reset();

		renderBase.sync[i] = D3D12::Sync();
	}

	renderBase.backBuffer = D3D12::BackBuffer();

	renderBase.pSwapChain.Reset();
	renderBase.pCmdQueue.Reset();
	renderBase.pDevice.Reset();
}

//---------------------------------------------------------------------------------------------------------------------

void RenderBase::ResizeSwapChain(RenderBase& renderBase)
{
	using namespace DemoFramework;

	// Wait for each pending present to complete since no back buffer
	// resources can be in use when resizing the swap chain.
	for(uint32_t i = 0; i < renderBase.bufferCount; ++i)
	{
		D3D12::Sync::Wait(renderBase.sync[i]);
	}

	// Clear the back buffer resources.
	renderBase.backBuffer = D3D12::BackBuffer();

	// Resize the swap chain, preserving the existing buffer count and format,
	// and using the window's width and height for the new buffers
	renderBase.pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, renderBase.swapChainFlags);

	// Create new back buffer resources from the resized swap chain.
	const bool createBackBuffersResult = D3D12::BackBuffer::Create(
		renderBase.backBuffer,
		renderBase.pDevice,
		renderBase.pSwapChain,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0);
	assert(createBackBuffersResult == true); (void) createBackBuffersResult;
}

//---------------------------------------------------------------------------------------------------------------------

void RenderBase::BeginFrame(RenderBase& renderBase)
{
	using namespace DemoFramework;

	renderBase.bufferIndex = renderBase.pSwapChain->GetCurrentBackBufferIndex();

	ID3D12CommandAllocator* const pCmdAlloc = renderBase.pCmdAlloc[renderBase.bufferIndex].Get();
	ID3D12GraphicsCommandList* const pCmdList = renderBase.pCmdList[renderBase.bufferIndex].Get();
	ID3D12Resource* const pBackBufferRtv = renderBase.backBuffer.pRtv[renderBase.bufferIndex].Get();

	const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = D3D12::BackBuffer::GetCpuDescHandle(renderBase.backBuffer, renderBase.bufferIndex);

	D3D12::Sync& sync = renderBase.sync[renderBase.bufferIndex];

	// Wait for the command queue to finish processing the current back buffer.
	D3D12::Sync::Wait(sync);

	// Reset the command list and its allocator.
	pCmdAlloc->Reset();
	pCmdList->Reset(pCmdAlloc, nullptr);

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
	pCmdList->ClearDepthStencilView(renderBase.pDepthDescHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------

void RenderBase::EndFrame(RenderBase& renderBase, const bool vsync)
{
	using namespace DemoFramework;

	const uint32_t presentInterval = vsync ? 1 : 0;
	const uint32_t presentFlags = !vsync ? renderBase.presentTearingFlag : 0;

	ID3D12GraphicsCommandList* const pCmdList = renderBase.pCmdList[renderBase.bufferIndex].Get();
	ID3D12Resource* const pBackBufferRtv = renderBase.backBuffer.pRtv[renderBase.bufferIndex].Get();

	D3D12::Sync& sync = renderBase.sync[renderBase.bufferIndex];

	D3D12_RESOURCE_BARRIER endBarrier;
	endBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	endBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	endBarrier.Transition.pResource = pBackBufferRtv;
	endBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	endBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	endBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	// Transition the back buffer from a render target to a surface that can be presented to the swap chain.
	pCmdList->ResourceBarrier(1, &endBarrier);

	// Stop recording commands.
	pCmdList->Close();

	ID3D12CommandList* const pCmdLists[] =
	{
		pCmdList,
	};

	// Execute the specified command lists.
	renderBase.pCmdQueue->ExecuteCommandLists(1, pCmdLists);

	// Present the current back buffer to the screen and flip to the next buffer.
	renderBase.pSwapChain->Present(presentInterval, presentFlags);

	// Add a signal to the command queue so we know when back buffer is no longer in use.
	D3D12::Sync::Signal(sync, renderBase.pCmdQueue);
}

//---------------------------------------------------------------------------------------------------------------------

void RenderBase::SetDefaultRenderTarget(const RenderBase& renderBase)
{
	using namespace DemoFramework;

	const D3D12_CPU_DESCRIPTOR_HANDLE depthTargetHandle = renderBase.pDepthDescHeap->GetCPUDescriptorHandleForHeapStart();
	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandles[] =
	{
		D3D12::BackBuffer::GetCpuDescHandle(renderBase.backBuffer, renderBase.bufferIndex),
	};

	ID3D12GraphicsCommandList* const pCmdList = renderBase.pCmdList[renderBase.bufferIndex].Get();

	// Set the back buffer as the render target along with the base depth target.
	pCmdList->OMSetRenderTargets(1, renderTargetHandles, false, &depthTargetHandle);
}

//---------------------------------------------------------------------------------------------------------------------
