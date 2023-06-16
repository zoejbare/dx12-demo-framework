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

#include "BackBuffer.hpp"
#include "CommandContext.hpp"

#include <memory>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class RenderBase;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::RenderBase
{
public:

	typedef std::shared_ptr<RenderBase> Ptr;

	RenderBase();
	RenderBase(const RenderBase&) = delete;
	RenderBase(RenderBase&& other) = delete;
	~RenderBase();

	RenderBase& operator =(const RenderBase&) = delete;
	RenderBase& operator =(RenderBase&&) = delete;

	static Ptr Create(
		HWND hWnd,
		uint32_t backBufferWidth,
		uint32_t backBufferHeight,
		uint32_t backBufferCount,
		DXGI_FORMAT backBufferFormat,
		DXGI_FORMAT depthFormat);

	void BeginFrame();
	void EndFrame(bool vsync);

	bool ResizeSwapChain();
	void SetBackBufferAsRenderTarget();

	const Device::Ptr& GetDevice() const;
	const CommandQueue::Ptr& GetCmdQueue() const;
	const SwapChain::Ptr& GetSwapChain() const;

	const GraphicsCommandContext::Ptr& GetUploadContext() const;
	const GraphicsCommandContext::Ptr& GetDrawContext() const;

	uint32_t GetBufferIndex() const;
	uint32_t GetBufferCount() const;

	D3D_ROOT_SIGNATURE_VERSION GetRootSignatureVersion() const;


private:

	void prv_waitForFrame(uint32_t);

	Device::Ptr m_device;
	CommandQueue::Ptr m_cmdQueue;
	SwapChain::Ptr m_swapChain;
	DescriptorHeap::Ptr m_depthDescHeap;
	Resource::Ptr m_depthBuffer;

	Fence::Ptr m_drawFence;
	Event::Ptr m_drawEvent;

	GraphicsCommandContext::Ptr m_uploadContext;
	GraphicsCommandContext::Ptr m_drawContext[DF_SWAP_CHAIN_BUFFER_MAX_COUNT];

	BackBuffer::Ptr m_backBuffer;

	uint64_t m_fenceMarker[DF_SWAP_CHAIN_BUFFER_MAX_COUNT];
	uint64_t m_nextFenceMarker;

	uint32_t m_bufferCount;
	uint32_t m_bufferIndex;

	DWORD m_swapChainFlags;
	DWORD m_presentTearingFlag;

	DXGI_FORMAT m_depthFormat;
	D3D_ROOT_SIGNATURE_VERSION m_rootSigVersion;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::RenderBase>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::RenderBase::RenderBase()
	: m_device()
	, m_cmdQueue()
	, m_swapChain()
	, m_depthDescHeap()
	, m_depthBuffer()
	, m_drawFence()
	, m_drawEvent()
	, m_uploadContext()
	, m_drawContext()
	, m_backBuffer()
	, m_fenceMarker()
	, m_nextFenceMarker(0)
	, m_bufferCount(0)
	, m_bufferIndex(0)
	, m_swapChainFlags(0)
	, m_presentTearingFlag(0)
	, m_depthFormat(DXGI_FORMAT_UNKNOWN)
	, m_rootSigVersion(D3D_ROOT_SIGNATURE_VERSION_1_0)
	, m_initialized(false)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::RenderBase::~RenderBase()
{
	// Flush each command list before shutting down.
	for(uint32_t i = 0; i < m_bufferCount; ++i)
	{
		prv_waitForFrame(i);
	}
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::Device::Ptr& DemoFramework::D3D12::RenderBase::GetDevice() const
{
	return m_device;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::CommandQueue::Ptr& DemoFramework::D3D12::RenderBase::GetCmdQueue() const
{
	return m_cmdQueue;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::GraphicsCommandContext::Ptr& DemoFramework::D3D12::RenderBase::GetUploadContext() const
{
	return m_uploadContext;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::GraphicsCommandContext::Ptr& DemoFramework::D3D12::RenderBase::GetDrawContext() const
{
	return m_drawContext[m_bufferIndex];
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::SwapChain::Ptr& DemoFramework::D3D12::RenderBase::GetSwapChain() const
{
	return m_swapChain;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::RenderBase::GetBufferIndex() const
{
	return m_bufferIndex;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::RenderBase::GetBufferCount() const
{
	return m_bufferCount;
}

//---------------------------------------------------------------------------------------------------------------------

inline D3D_ROOT_SIGNATURE_VERSION DemoFramework::D3D12::RenderBase::GetRootSignatureVersion() const
{
	return m_rootSigVersion;
}

//---------------------------------------------------------------------------------------------------------------------
