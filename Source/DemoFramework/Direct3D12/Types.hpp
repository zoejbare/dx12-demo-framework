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

#include "../BuildSetup.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

//---------------------------------------------------------------------------------------------------------------------

#define BACK_BUFFER_MAX_COUNT 3

#define DECL_WRL_COM_TYPE(ifaceName, typeName) \
	class DF_API typeName : public Microsoft::WRL::ComPtr<ifaceName> { public: using Microsoft::WRL::ComPtr<ifaceName>::ComPtr; }

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	DECL_WRL_COM_TYPE(ID3DBlob, BlobPtr);

	DECL_WRL_COM_TYPE(ID3D12CommandQueue, CommandQueuePtr);
	DECL_WRL_COM_TYPE(ID3D12CommandAllocator, CommandAllocatorPtr);
	DECL_WRL_COM_TYPE(ID3D12Debug, DebugPtr);
	DECL_WRL_COM_TYPE(ID3D12DescriptorHeap, DescriptorHeapPtr);
	DECL_WRL_COM_TYPE(ID3D12Device2, DevicePtr);
	DECL_WRL_COM_TYPE(ID3D12InfoQueue, InfoQueuePtr);
	DECL_WRL_COM_TYPE(ID3D12Fence, FencePtr);
	DECL_WRL_COM_TYPE(ID3D12GraphicsCommandList, GraphicsCommandListPtr);
	DECL_WRL_COM_TYPE(ID3D12PipelineState, PipelineStatePtr);
	DECL_WRL_COM_TYPE(ID3D12Resource, ResourcePtr);
	DECL_WRL_COM_TYPE(ID3D12RootSignature, RootSignaturePtr);

	DECL_WRL_COM_TYPE(IDXGIAdapter4, AdapterPtr);
	DECL_WRL_COM_TYPE(IDXGIFactory4, FactoryPtr);
	DECL_WRL_COM_TYPE(IDXGISwapChain4, SwapChainPtr);
}}

//---------------------------------------------------------------------------------------------------------------------
