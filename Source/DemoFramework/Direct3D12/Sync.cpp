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

#include "LowLevel/Event.hpp"
#include "LowLevel/Fence.hpp"

#include "../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Sync::Ptr DemoFramework::D3D12::Sync::Create(const DevicePtr& device, const D3D12_FENCE_FLAGS flags)
{
	if(!device)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	Ptr output = std::make_shared<Sync>();

	// Create a fence object.
	output->m_fence = CreateFence(device, flags, 0);
	if(!output->m_fence)
	{
		return Ptr();
	}

	// Create a native event that will be set when the fence is signaled.
	output->m_event = CreateEvent(nullptr, false, false, nullptr);
	if(!output->m_event)
	{
		return Ptr();
	}

	output->m_waitValue = 0;
	output->m_nextValue = output->m_waitValue + 1;

	output->m_initialized = true;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Sync::Signal(const CommandQueuePtr& pCmdQueue)
{
	if(m_initialized)
	{
		const HRESULT result = pCmdQueue->Signal(m_fence.Get(), m_nextValue);
		if(result != S_OK)
		{
			LOG_ERROR("Failed to enqueue fence signal command; result='0x%08" PRIX32 "'", result);
			return;
		}

		m_waitValue = m_nextValue;

		++m_nextValue;
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Sync::Wait(const uint32_t timeout)
{
	if(m_initialized && m_fence->GetCompletedValue() != m_waitValue)
	{
		const HRESULT result = m_fence->SetEventOnCompletion(m_waitValue, m_event->GetHandle());
		if(result != S_OK)
		{
			LOG_ERROR("Failed to set completion event on fence; result='0x%08" PRIX32 "'", result);
			return;
		}

		::WaitForSingleObject(m_event->GetHandle(), timeout);
	}
}

//---------------------------------------------------------------------------------------------------------------------
