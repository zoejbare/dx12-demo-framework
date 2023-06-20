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

#include "DescriptorAllocator.hpp"

#include "LowLevel/DescriptorHeap.hpp"

#include "../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

const DemoFramework::D3D12::Descriptor DemoFramework::D3D12::Descriptor::Invalid =
{
	{0},      // D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle
	{0},      // D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle
	UINT_MAX, // uint32_t id
};

//---------------------------------------------------------------------------------------------------------------------

// Defining the free list using PIMPL to make MSVC shut up about std::set<> needing a DLL interface.
struct DemoFramework::D3D12::DescriptorAllocator::FreeList
{
	std::set<uint32_t> list;
};

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::DescriptorAllocator::~DescriptorAllocator()
{
	if(m_pFreeList)
	{
		delete m_pFreeList;
	}
}

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::DescriptorAllocator::Ptr DemoFramework::D3D12::DescriptorAllocator::Create(
	const Device::Ptr& device,
	const D3D12_DESCRIPTOR_HEAP_DESC& desc)
{
	if(!device || desc.NumDescriptors == 0)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	Ptr output = std::make_shared<DescriptorAllocator>();

	// Create the descriptor heap object.
	output->m_heap = CreateDescriptorHeap(device, desc);
	if(!output->m_heap)
	{
		return Ptr();
	}

	output->m_pFreeList = new FreeList();
	output->m_incrSize = device->GetDescriptorHandleIncrementSize(desc.Type);
	output->m_totalLength = desc.NumDescriptors;
	output->m_lastIndex = desc.NumDescriptors - 1;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Descriptor DemoFramework::D3D12::DescriptorAllocator::Allocate()
{
	uint32_t index = Descriptor::Invalid.index;

	// Check if we need to allocate from the tail of the heap (meaning all descriptors at the moment have been
	// contiguously allocated from the start of the heap) or if we can re-use a descriptor in the free list.
	if(!m_pFreeList->list.empty())
	{
		// Get the entry at the head of the free list. Because this list is ordered,
		// this should give us the lowest available heap index, which should help us
		// keep entries as packed as possible in the heap.
		auto listHead = m_pFreeList->list.begin();

		index = *listHead;

		// Remove the current entry from the free list.
		m_pFreeList->list.erase(listHead);
	}
	else
	{
		// There is nothing available in the free list, so make sure there is available space at the end of the heap.
		if(m_tailIndex <= m_lastIndex)
		{
			index = m_tailIndex;

			++m_tailIndex;
		}
	}

	Descriptor output = Descriptor::Invalid;

	// Check if a valid heap index was found.
	if(index != Descriptor::Invalid.index)
	{
		const size_t cpuHandleStart = m_heap->GetCPUDescriptorHandleForHeapStart().ptr;
		const size_t gpuHandleStart = m_heap->GetGPUDescriptorHandleForHeapStart().ptr;
		const size_t offset = m_incrSize * size_t(index);

		output.cpuHandle.ptr = cpuHandleStart + offset;
		output.gpuHandle.ptr = gpuHandleStart + offset;
		output.index = index;

		++m_currentLength;
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::DescriptorAllocator::Free(Descriptor& descriptor)
{
	// Make sure the descriptor index is within the bounds of the heap.
	if(descriptor.index < m_totalLength)
	{
		if(descriptor.index == m_tailIndex)
		{
			// The descriptor being freed is at the end of the list, so we don't need to deal with the free list.
			--m_currentLength;
			--m_tailIndex;
		}
		else if(descriptor.index < m_currentLength && !m_pFreeList->list.count(descriptor.index))
		{
			// Insert the descriptor index into the free list when it's not already there.
			m_pFreeList->list.insert(descriptor.index);

			--m_currentLength;
		}

		descriptor = Descriptor::Invalid;
	}
}

//---------------------------------------------------------------------------------------------------------------------
