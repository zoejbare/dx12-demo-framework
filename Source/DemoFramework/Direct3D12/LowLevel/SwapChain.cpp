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

DemoFramework::D3D12::SwapChainPtr DemoFramework::D3D12::CreateSwapChain(
	const FactoryPtr& pFactory,
	const CommandQueuePtr& pCmdQueue,
	const DXGI_SWAP_CHAIN_DESC1& desc,
	HWND hWnd)
{
	typedef Microsoft::WRL::ComPtr<IDXGISwapChain1> StagingSwapChain;

	if(!pFactory || !pCmdQueue)
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	if(desc.BufferCount > DF_SWAP_CHAIN_BUFFER_MAX_COUNT)
	{
		LOG_ERROR("Exceeded the maximum number of supported swap chain buffers; value=%" PRIu32 ", maximum=%" PRIu32, desc.BufferCount, DF_SWAP_CHAIN_BUFFER_MAX_COUNT);
		return nullptr;
	}

	StagingSwapChain pStaging;
	D3D12::SwapChainPtr pOutput;

	const HRESULT createResult = pFactory->CreateSwapChainForHwnd(pCmdQueue.Get(), hWnd, &desc, nullptr, nullptr, &pStaging);
	if(createResult != S_OK)
	{
		LOG_ERROR("Failed to create swap chain; result='0x%08" PRIX32 "'", createResult);
		return nullptr;
	}

	if(!SUCCEEDED(pStaging.As(&pOutput)))
	{
		LOG_ERROR("Failed to query IDXGISwapChain4 interface");
		return nullptr;
	}

	const HRESULT makeAssocResult = pFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	if(makeAssocResult != S_OK)
	{
		LOG_ERROR("Failed to make window association to swap chain; result='0x%08" PRIX32 "'", makeAssocResult);
		return nullptr;
	}

	return pOutput;
}

//---------------------------------------------------------------------------------------------------------------------
