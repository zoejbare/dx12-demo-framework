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

#include "Adapter.hpp"

#include "../../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Adapter::Ptr DemoFramework::D3D12::QueryAdapter(const Factory::Ptr& factory, const bool useWarpAdapter)
{
	typedef Microsoft::WRL::ComPtr<IDXGIAdapter1> StagingAdapterPtr;

	if(!factory)
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	D3D12::Adapter::Ptr output;

	if(useWarpAdapter)
	{
		StagingAdapterPtr pStagingAdapter;

		// We can get the WARP adapter directly.
		factory->EnumWarpAdapter(IID_PPV_ARGS(&pStagingAdapter));

		// Safe-cast the staging adapter to the output pointer.
		pStagingAdapter.As(&output);
		assert(output != nullptr);
	}
	else
	{
		StagingAdapterPtr pStagingAdapter;

		size_t maxVramSize = 0;

		// Select a display adapter.
		for(uint32_t adapterIndex = 0; factory->EnumAdapters1(adapterIndex, &pStagingAdapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 adapterDesc;
			pStagingAdapter->GetDesc1(&adapterDesc);

			// Do not consider software adapters.
			if((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			{
				continue;
			}

			// Use the adapter with the largest amount of dedicated VRAM.
			if(adapterDesc.DedicatedVideoMemory < maxVramSize)
			{
				continue;
			}

			// Make sure a device can be created with this adapter.
			if(!SUCCEEDED(D3D12CreateDevice(pStagingAdapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
			{
				continue;
			}

			maxVramSize = adapterDesc.DedicatedVideoMemory;

			// Safe-cast the staging adapter to the output pointer.
			pStagingAdapter.As(&output);
			assert(output != nullptr);
		}
	}

	if(!output)
	{
		LOG_ERROR("Failed to query a usable display adapter");
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
