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

#include "SwapChain.hpp"

#include "../../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::SwapChain::Ptr DemoFramework::D3D12::CreateSwapChain(
	const Factory::Ptr& factory,
	const CommandQueue::Ptr& cmdQueue,
	const DXGI_SWAP_CHAIN_DESC1& desc,
	HWND hWnd)
{
	typedef Microsoft::WRL::ComPtr<IDXGISwapChain1> StagingSwapChainPtr;

	if(!factory || !cmdQueue)
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	if(desc.BufferCount > DF_SWAP_CHAIN_BUFFER_MAX_COUNT)
	{
		LOG_ERROR("Exceeded the maximum number of supported swap chain buffers; value=%" PRIu32 ", maximum=%" PRIu32, desc.BufferCount, DF_SWAP_CHAIN_BUFFER_MAX_COUNT);
		return nullptr;
	}

	StagingSwapChainPtr staging;
	D3D12::SwapChain::Ptr output;

	const HRESULT createResult = factory->CreateSwapChainForHwnd(cmdQueue.Get(), hWnd, &desc, nullptr, nullptr, &staging);
	if(createResult != S_OK)
	{
		LOG_ERROR("Failed to create swap chain; result='0x%08" PRIX32 "'", createResult);
		return nullptr;
	}

	if(!SUCCEEDED(staging.As(&output)))
	{
		LOG_ERROR("Failed to query IDXGISwapChain4 interface");
		return nullptr;
	}

	const HRESULT makeAssocResult = factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	if(makeAssocResult != S_OK)
	{
		LOG_ERROR("Failed to make window association with swap chain; result='0x%08" PRIX32 "'", makeAssocResult);
		return nullptr;
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
