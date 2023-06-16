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

#include "../../BuildSetup.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

//---------------------------------------------------------------------------------------------------------------------

#define DF_SWAP_CHAIN_BUFFER_MAX_COUNT 3

#define DF_DECL_WRL_COM_TYPE(ifaceName, typeName) \
	class DF_API typeName \
	{ \
		public: \
			class DF_API Ptr : public Microsoft::WRL::ComPtr<ifaceName> \
			{ \
				public: \
					using Microsoft::WRL::ComPtr<ifaceName>::ComPtr; \
			}; \
	}

//---------------------------------------------------------------------------------------------------------------------

EXTERN_C const IID IID_D3D12_ID3DEvent;

MIDL_INTERFACE("BFBD1EDC-60DB-4322-8F28-40581ACFCCA2")
ID3DEvent
	: public IUnknown
{
	virtual HANDLE GetHandle() const = 0;
};

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	DF_DECL_WRL_COM_TYPE(ID3DBlob, Blob);
	DF_DECL_WRL_COM_TYPE(ID3DEvent, Event);

	DF_DECL_WRL_COM_TYPE(ID3D12CommandQueue, CommandQueue);
	DF_DECL_WRL_COM_TYPE(ID3D12CommandAllocator, CommandAllocator);
	DF_DECL_WRL_COM_TYPE(ID3D12Debug, DebugPtr);
	DF_DECL_WRL_COM_TYPE(ID3D12DescriptorHeap, DescriptorHeap);
	DF_DECL_WRL_COM_TYPE(ID3D12Device2, Device);
	DF_DECL_WRL_COM_TYPE(ID3D12InfoQueue, InfoQueue);
	DF_DECL_WRL_COM_TYPE(ID3D12Fence, Fence);
	DF_DECL_WRL_COM_TYPE(ID3D12GraphicsCommandList, GraphicsCommandList);
	DF_DECL_WRL_COM_TYPE(ID3D12PipelineState, PipelineState);
	DF_DECL_WRL_COM_TYPE(ID3D12Resource, Resource);
	DF_DECL_WRL_COM_TYPE(ID3D12RootSignature, RootSignature);

	DF_DECL_WRL_COM_TYPE(IDXGIAdapter4, Adapter);
	DF_DECL_WRL_COM_TYPE(IDXGIFactory4, Factory);
	DF_DECL_WRL_COM_TYPE(IDXGISwapChain4, SwapChain);
}}

//---------------------------------------------------------------------------------------------------------------------
