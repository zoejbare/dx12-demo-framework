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
#include "Sync.hpp"

#include <memory>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class RenderBase;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::RenderBase
{
public:

	RenderBase();
	RenderBase(const RenderBase&) = delete;
	RenderBase(RenderBase&& other) = delete;
	~RenderBase();

	RenderBase& operator =(const RenderBase&) = delete;
	RenderBase& operator =(RenderBase&&) = delete;

	bool Initialize(
		HWND hWnd,
		uint32_t backBufferWidth,
		uint32_t backBufferHeight,
		uint32_t backBufferCount,
		DXGI_FORMAT backBufferFormat,
		DXGI_FORMAT depthFormat
	);

	void BeginFrame();
	void EndFrame(bool vsync);

	bool ResizeSwapChain();
	void SetBackBufferAsRenderTarget();

	const DevicePtr& GetDevice() const;
	const CommandQueuePtr& GetCmdQueue() const;
	const CommandAllocatorPtr& GetCmdAlloc() const;
	const GraphicsCommandListPtr& GetCmdList() const;
	const SwapChainPtr& GetSwapChain() const;

	uint32_t GetBufferIndex() const;
	uint32_t GetBufferCount() const;

	D3D_ROOT_SIGNATURE_VERSION GetRootSignatureVersion() const;


private:

	DevicePtr m_pDevice;
	CommandQueuePtr m_pCmdQueue;
	CommandAllocatorPtr m_pCmdAlloc[DF_SWAP_CHAIN_BUFFER_MAX_COUNT];
	GraphicsCommandListPtr m_pCmdList[DF_SWAP_CHAIN_BUFFER_MAX_COUNT];
	SwapChainPtr m_pSwapChain;
	DescriptorHeapPtr m_pDepthDescHeap;
	ResourcePtr m_pDepthBuffer;

	BackBuffer m_backBuffer;
	Sync m_sync[DF_SWAP_CHAIN_BUFFER_MAX_COUNT];

	uint32_t m_bufferCount;
	uint32_t m_bufferIndex;

	DWORD m_swapChainFlags;
	DWORD m_presentTearingFlag;

	DXGI_FORMAT m_depthFormat;
	D3D_ROOT_SIGNATURE_VERSION m_rootSigVersion;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::RenderBase::RenderBase()
	: m_pDevice()
	, m_pCmdQueue()
	, m_pCmdAlloc()
	, m_pCmdList()
	, m_pSwapChain()
	, m_pDepthDescHeap()
	, m_pDepthBuffer()
	, m_backBuffer()
	, m_sync()
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
	for(size_t i = 0; i < m_bufferCount; ++i)
	{
		m_sync[i].Wait();

		m_pCmdAlloc[i].Reset();
		m_pCmdList[i].Reset();
	}

	m_pSwapChain.Reset();
	m_pCmdQueue.Reset();
	m_pDevice.Reset();
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::DevicePtr& DemoFramework::D3D12::RenderBase::GetDevice() const
{
	return m_pDevice;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::CommandQueuePtr& DemoFramework::D3D12::RenderBase::GetCmdQueue() const
{
	return m_pCmdQueue;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::CommandAllocatorPtr& DemoFramework::D3D12::RenderBase::GetCmdAlloc() const
{
	return m_pCmdAlloc[m_bufferIndex];
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::GraphicsCommandListPtr& DemoFramework::D3D12::RenderBase::GetCmdList() const
{
	return m_pCmdList[m_bufferIndex];
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::SwapChainPtr& DemoFramework::D3D12::RenderBase::GetSwapChain() const
{
	return m_pSwapChain;
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
