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
	class RenderTarget;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::RenderTarget
{
public:

	typedef std::shared_ptr<RenderTarget> Ptr;

	RenderTarget();
	RenderTarget(const RenderTarget&) = delete;
	RenderTarget(RenderTarget&&) = delete;

	RenderTarget& operator =(const RenderTarget&) = delete;
	RenderTarget& operator =(RenderTarget&&) = delete;

	static Ptr Create(const Device::Ptr& device, uint32_t width, uint32_t height, DXGI_FORMAT format);

	void TransitionTo(const GraphicsCommandList::Ptr& cmdList, D3D12_RESOURCE_STATES states);

	const Resource::Ptr& GetResource() const;
	const DescriptorHeap::Ptr& GetRtvHeap() const;
	const DescriptorHeap::Ptr& GetSrvHeap() const;


private:

	Resource::Ptr m_resource;

	DescriptorHeap::Ptr m_rtvHeap;
	DescriptorHeap::Ptr m_srvHeap;

	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;

	D3D12_RESOURCE_STATES m_currentStates;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::RenderTarget>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::RenderTarget::RenderTarget()
	: m_resource()
	, m_rtvHeap()
	, m_srvHeap()
	, m_rtvHandle({0})
	, m_srvHandle({0})
	, m_currentStates(D3D12_RESOURCE_STATE_COMMON)
	, m_initialized(false)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::Resource::Ptr& DemoFramework::D3D12::RenderTarget::GetResource() const
{
	return m_resource;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::DescriptorHeap::Ptr& DemoFramework::D3D12::RenderTarget::GetRtvHeap() const
{
	return m_rtvHeap;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::DescriptorHeap::Ptr& DemoFramework::D3D12::RenderTarget::GetSrvHeap() const
{
	return m_srvHeap;
}

//---------------------------------------------------------------------------------------------------------------------
