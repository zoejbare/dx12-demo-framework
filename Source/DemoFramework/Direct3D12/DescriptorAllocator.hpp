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

#include "LowLevel/Types.hpp"

#include <memory>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class DescriptorAllocator;

	struct DF_API Descriptor
	{
		static const Descriptor Invalid;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

		uint32_t index;

		bool temp;
	};
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::DescriptorAllocator
{
public:

	typedef std::shared_ptr<DescriptorAllocator> Ptr;

	DescriptorAllocator();
	DescriptorAllocator(const DescriptorAllocator&) = delete;
	DescriptorAllocator(DescriptorAllocator&&) = delete;
	~DescriptorAllocator();

	DescriptorAllocator& operator =(const DescriptorAllocator&) = delete;
	DescriptorAllocator& operator =(DescriptorAllocator&&) = delete;

	static Ptr Create(const Device::Ptr& device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

	Descriptor Allocate();
	Descriptor AllocateTemp();

	void Free(const Descriptor& descriptor);
	void Free(Descriptor& descriptor);
	void FreeTempList();

	const DescriptorHeap::Ptr& GetHeap() const;

	uint32_t GetTotalLength() const;
	uint32_t GetCurrentLength() const;


private:

	void _internalFree(uint32_t);

	struct FreeList;
	struct TempList;

	DescriptorHeap::Ptr m_heap;

	FreeList* m_pFreeList;
	TempList* m_pTempList;

	size_t m_incrSize;

	uint32_t m_totalLength;
	uint32_t m_currentLength;

	uint32_t m_lastIndex;
	uint32_t m_tailIndex;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::DescriptorAllocator>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::DescriptorAllocator::DescriptorAllocator()
	: m_heap()
	, m_pFreeList(nullptr)
	, m_pTempList(nullptr)
	, m_incrSize(0)
	, m_totalLength(0)
	, m_currentLength(0)
	, m_lastIndex(0)
	, m_tailIndex(0)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline void DemoFramework::D3D12::DescriptorAllocator::Free(const Descriptor& descriptor)
{
	assert(!descriptor.temp);
	_internalFree(descriptor.index);
}

//---------------------------------------------------------------------------------------------------------------------

inline void DemoFramework::D3D12::DescriptorAllocator::Free(Descriptor& descriptor)
{
	assert(!descriptor.temp);
	_internalFree(descriptor.index);

	descriptor = Descriptor::Invalid;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::DescriptorHeap::Ptr& DemoFramework::D3D12::DescriptorAllocator::GetHeap() const
{
	return m_heap;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::DescriptorAllocator::GetTotalLength() const
{
	return m_totalLength;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::DescriptorAllocator::GetCurrentLength() const
{
	return m_currentLength;
}

//---------------------------------------------------------------------------------------------------------------------
