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

#include "BackBuffer.hpp"

#include "LowLevel/DescriptorHeap.hpp"

#include "../Application/Log.hpp"

#include <functional>

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::BackBuffer::Initialize(
	const DevicePtr& pDevice,
	const SwapChainPtr& pSwapChain,
	const D3D12_DESCRIPTOR_HEAP_FLAGS flags,
	const uint32_t nodeMask)
{
	if(!pDevice || !pSwapChain)
	{
		LOG_ERROR("Invalid parameter");
		return false;
	}
	else if(m_initialized)
	{
		LOG_ERROR("Back buffer already initialized");
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	pSwapChain->GetDesc1(&swapChainDesc);

	m_bufferCount = swapChainDesc.BufferCount;

	const D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, // D3D12_DESCRIPTOR_HEAP_TYPE Type
		m_bufferCount,                  // UINT NumDescriptors
		flags,                          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		nodeMask,                       // UINT NodeMask
	};

	// Create a descriptor heap associated with this set of back buffers.
	m_pDescHeap = CreateDescriptorHeap(pDevice, descHeapDesc);
	if(!m_pDescHeap)
	{
		return false;
	}

	// Get the start of the descriptor heap memory.
	D3D12_CPU_DESCRIPTOR_HANDLE tempHandle = m_pDescHeap->GetCPUDescriptorHandleForHeapStart();

	// Cache the original descriptor handle value before we modify it creating the RTVs.
	m_cpuHandle = tempHandle;
	m_incrSize = pDevice->GetDescriptorHandleIncrementSize(descHeapDesc.Type);

	// Create RTVs for each back buffer in the swap chain.
	for(uint32_t bufferIndex = 0; bufferIndex < swapChainDesc.BufferCount; ++bufferIndex)
	{
		ResourcePtr& pBuffer = m_pRtv[bufferIndex];

		// Get the back buffer resource for the current buffer index.
		if(!SUCCEEDED(pSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&pBuffer))))
		{
			LOG_ERROR("Failed to retrieve swap chain buffer index '%" PRIu32 "'", bufferIndex);
			return false;
		}

		// Create the RTV for the current back buffer resource.
		pDevice->CreateRenderTargetView(pBuffer.Get(), nullptr, tempHandle);

		// Update the handle pointer to reference the next buffer in the descriptor.
		tempHandle.ptr += size_t(m_incrSize);
	}

	m_initialized = true;

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::BackBuffer::Destroy()
{
	for(size_t i = 0; i < DF_SWAP_CHAIN_BUFFER_MAX_COUNT; ++i)
	{
		m_pRtv[i].Reset();
	}

	m_pDescHeap.Reset();

	m_cpuHandle = {0};
	m_bufferCount = 0;
	m_incrSize = 0;
	m_initialized = false;
}

//---------------------------------------------------------------------------------------------------------------------
