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

DemoFramework::D3D12::BackBuffer::Ptr DemoFramework::D3D12::BackBuffer::Create(
	const Device::Ptr& device,
	const SwapChain::Ptr& swapChain,
	const DescriptorAllocator::Ptr& rtvAlloc)
{
	if(!device || !swapChain || !rtvAlloc)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	Ptr output = std::make_shared<BackBuffer>();

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChain->GetDesc1(&swapChainDesc);

	// Create RTVs for each back buffer in the swap chain.
	for(uint32_t bufferIndex = 0; bufferIndex < swapChainDesc.BufferCount; ++bufferIndex)
	{
		Resource::Ptr& pBuffer = output->m_rtv[bufferIndex];

		// Get the back buffer resource for the current buffer index.
		if(!SUCCEEDED(swapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&pBuffer))))
		{
			LOG_ERROR("Failed to retrieve swap chain buffer index '%" PRIu32 "'", bufferIndex);
			return Ptr();
		}

		// Allocate a new descriptor in the heap for the current buffer.
		const Descriptor descriptor = rtvAlloc->Allocate();
		assert(descriptor.index != Descriptor::Invalid.index);

		// Create the RTV for the current back buffer resource.
		device->CreateRenderTargetView(pBuffer.Get(), nullptr, descriptor.cpuHandle);

		output->m_descAlloc = rtvAlloc;
		output->m_descriptor[bufferIndex] = descriptor;
		output->m_bufferCount = swapChainDesc.BufferCount;
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
