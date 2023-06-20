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

#include "DescriptorAllocator.hpp"

#include <memory>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class BackBuffer;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::BackBuffer
{
public:

	typedef std::shared_ptr<BackBuffer> Ptr;

	BackBuffer();
	BackBuffer(const BackBuffer&) = delete;
	BackBuffer(BackBuffer&&) = delete;
	~BackBuffer();

	BackBuffer& operator =(const BackBuffer&) = delete;
	BackBuffer& operator =(BackBuffer&&) = delete;

	static Ptr Create(
		const Device::Ptr& device,
		const SwapChain::Ptr& swapChain,
		const DescriptorAllocator::Ptr& rtvAlloc
	);

	const Resource::Ptr& GetRtv(size_t bufferIndex) const;
	const Descriptor& GetDescriptor(size_t bufferIndex) const;


private:

	DescriptorAllocator::Ptr m_descAlloc;

	Resource::Ptr m_rtv[DF_SWAP_CHAIN_BUFFER_MAX_COUNT];
	Descriptor m_descriptor[DF_SWAP_CHAIN_BUFFER_MAX_COUNT];

	uint32_t m_bufferCount;
	uint32_t m_incrSize;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::BackBuffer>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::BackBuffer::BackBuffer()
	: m_descAlloc()
	, m_rtv()
	, m_descriptor()
	, m_bufferCount(0)
	, m_incrSize(0)
{
	for(size_t i = 0; i < DF_SWAP_CHAIN_BUFFER_MAX_COUNT; ++i)
	{
		m_descriptor[i] = Descriptor::Invalid;
	}
}

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::BackBuffer::~BackBuffer()
{
	if(m_descAlloc)
	{
		for(size_t i = 0; i < m_bufferCount; ++i)
		{
			m_descAlloc->Free(m_descriptor[i]);
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::Resource::Ptr& DemoFramework::D3D12::BackBuffer::GetRtv(const size_t bufferIndex) const
{
	assert(bufferIndex < m_bufferCount);

	return m_rtv[bufferIndex];
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::Descriptor& DemoFramework::D3D12::BackBuffer::GetDescriptor(const size_t bufferIndex) const
{
	assert(bufferIndex < m_bufferCount);

	return m_descriptor[bufferIndex];
}

//---------------------------------------------------------------------------------------------------------------------
