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

#include <DemoFramework/Direct3D12/Struct/BackBuffer.hpp>
#include <DemoFramework/Direct3D12/Struct/Sync.hpp>

//---------------------------------------------------------------------------------------------------------------------

struct RenderBase
{
	static bool Initialize(
		RenderBase& output,
		HWND hWnd,
		uint32_t backBufferWidth,
		uint32_t backBufferHeight,
		uint32_t backBufferCount,
		DXGI_FORMAT backBufferFormat,
		DXGI_FORMAT depthFormat
	);
	static void Destroy(RenderBase& renderBase);
	static void ResizeSwapChain(RenderBase& renderBase);

	static void BeginFrame(RenderBase& renderBase);
	static void EndFrame(RenderBase& renderBase, bool vsync);

	static void SetDefaultRenderTarget(const RenderBase& renderBase);

	DemoFramework::D3D12::DevicePtr pDevice;
	DemoFramework::D3D12::CommandQueuePtr pCmdQueue;
	DemoFramework::D3D12::CommandAllocatorPtr pCmdAlloc[BACK_BUFFER_MAX_COUNT];
	DemoFramework::D3D12::GraphicsCommandListPtr pCmdList[BACK_BUFFER_MAX_COUNT];
	DemoFramework::D3D12::FencePtr pFence;
	DemoFramework::D3D12::SwapChainPtr pSwapChain;
	DemoFramework::D3D12::DescriptorHeapPtr pDepthDescHeap;
	DemoFramework::D3D12::ResourcePtr pDepthBuffer;

	DemoFramework::D3D12::BackBuffer backBuffer;
	DemoFramework::D3D12::Sync sync[BACK_BUFFER_MAX_COUNT];

	uint32_t bufferCount;
	uint32_t bufferIndex;

	DWORD swapChainFlags;
	DWORD presentTearingFlag;

	D3D_ROOT_SIGNATURE_VERSION rootSigVersion;
};

//---------------------------------------------------------------------------------------------------------------------
