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

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class BackBuffer;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::BackBuffer
{
public:

	BackBuffer();
	BackBuffer(const BackBuffer&) = delete;
	BackBuffer(BackBuffer&&) = delete;

	BackBuffer& operator =(const BackBuffer&) = delete;
	BackBuffer& operator =(BackBuffer&&) = delete;

	bool Initialize(
		const DevicePtr& pDevice,
		const SwapChainPtr& pSwapChain,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags,
		uint32_t nodeMask
	);
	void Destroy();

	const ResourcePtr& GetRtv(size_t bufferIndex) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescHandle(size_t bufferIndex) const;


private:

	ResourcePtr m_pRtv[DF_SWAP_CHAIN_BUFFER_MAX_COUNT];
	DescriptorHeapPtr m_pDescHeap;

	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;

	uint32_t m_bufferCount;
	uint32_t m_incrSize;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::BackBuffer::BackBuffer()
	: m_pRtv()
	, m_pDescHeap()
	, m_cpuHandle({0})
	, m_bufferCount(0)
	, m_incrSize(0)
	, m_initialized(false)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::ResourcePtr& DemoFramework::D3D12::BackBuffer::GetRtv(const size_t bufferIndex) const
{
	assert(bufferIndex < m_bufferCount);

	return m_pRtv[bufferIndex];
}

//---------------------------------------------------------------------------------------------------------------------

inline D3D12_CPU_DESCRIPTOR_HANDLE DemoFramework::D3D12::BackBuffer::GetCpuDescHandle(const size_t bufferIndex) const
{
	assert(bufferIndex < m_bufferCount);

	D3D12_CPU_DESCRIPTOR_HANDLE output = m_cpuHandle;
	output.ptr += size_t(m_incrSize) * bufferIndex;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
