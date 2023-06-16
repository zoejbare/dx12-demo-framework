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
	class DepthTarget;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::DepthTarget
{
public:

	typedef std::shared_ptr<DepthTarget> Ptr;

	DepthTarget();
	DepthTarget(const DepthTarget&) = delete;
	DepthTarget(DepthTarget&&) = delete;

	DepthTarget& operator =(const DepthTarget&) = delete;
	DepthTarget& operator =(DepthTarget&&) = delete;

	static Ptr Create(const Device::Ptr& device, uint32_t width, uint32_t height, DXGI_FORMAT format);

	void TransitionTo(const GraphicsCommandList::Ptr& cmdList, D3D12_RESOURCE_STATES states);

	const Resource::Ptr& GetResource() const;
	const DescriptorHeap::Ptr& GetDsvHeap() const;


private:

	Resource::Ptr m_resource;
	DescriptorHeap::Ptr m_heap;

	D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::DepthTarget>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::DepthTarget::DepthTarget()
	: m_resource()
	, m_heap()
	, m_dsvHandle({0})
	, m_initialized(false)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::Resource::Ptr& DemoFramework::D3D12::DepthTarget::GetResource() const
{
	return m_resource;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::DescriptorHeap::Ptr& DemoFramework::D3D12::DepthTarget::GetDsvHeap() const
{
	return m_heap;
}

//---------------------------------------------------------------------------------------------------------------------
