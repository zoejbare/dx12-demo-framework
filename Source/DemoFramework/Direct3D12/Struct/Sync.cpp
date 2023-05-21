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

#include "Sync.hpp"

#include "../Fence.hpp"

#include "../../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::Sync::Create(Sync& output, DevicePtr& pDevice, const D3D12_FENCE_FLAGS flags)
{
	if(!pDevice)
	{
		LOG_ERROR("Invalid parameter");
		return false;
	}

	output.pFence = CreateFence(pDevice, flags, 0);
	if(!output.pFence)
	{
		return false;
	}

	output.pEvent = CreateEvent(nullptr, false, false, nullptr);
	if(!output.pEvent)
	{
		return false;
	}

	output.waitValue = 0;
	output.nextValue = output.waitValue + 1;

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Sync::Signal(Sync& sync, CommandQueuePtr& pCmdQueue)
{
	if(sync.pFence && pCmdQueue)
	{
		const HRESULT result = pCmdQueue->Signal(sync.pFence.Get(), sync.nextValue);
		if(result != S_OK)
		{
			LOG_ERROR("Failed to enqueue fence signal command; result='0x%08" PRIX32 "'", result);
			return;
		}

		sync.waitValue = sync.nextValue;

		++sync.nextValue;
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Sync::Wait(Sync& sync, const uint32_t timeout)
{
	if(sync.pFence && sync.pFence->GetCompletedValue() != sync.waitValue)
	{
		const HRESULT result = sync.pFence->SetEventOnCompletion(sync.waitValue, sync.pEvent->GetHandle());
		if(result != S_OK)
		{
			LOG_ERROR("Failed to set completion event on fence; result='0x%08" PRIX32 "'", result);
			return;
		}

		::WaitForSingleObject(sync.pEvent->GetHandle(), timeout);
	}
}

//---------------------------------------------------------------------------------------------------------------------
