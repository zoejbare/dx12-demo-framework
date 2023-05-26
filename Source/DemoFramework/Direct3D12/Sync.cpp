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

bool DemoFramework::D3D12::Sync::Initialize(const DevicePtr& pDevice, const D3D12_FENCE_FLAGS flags)
{
	if(!pDevice)
	{
		LOG_ERROR("Invalid parameter");
		return false;
	}
	else if(m_initialized)
	{
		LOG_ERROR("Sync object already initialized");
		return false;
	}

	// Create a fence object.
	m_pFence = CreateFence(pDevice, flags, 0);
	if(!m_pFence)
	{
		return false;
	}

	// Create a native event that will be set when the fence is signaled.
	m_pEvent = CreateEvent(nullptr, false, false, nullptr);
	if(!m_pEvent)
	{
		return false;
	}

	m_waitValue = 0;
	m_nextValue = m_waitValue + 1;

	m_initialized = true;

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Sync::Signal(const CommandQueuePtr& pCmdQueue)
{
	if(m_initialized)
	{
		const HRESULT result = pCmdQueue->Signal(m_pFence.Get(), m_nextValue);
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
	if(m_initialized && m_pFence->GetCompletedValue() != m_waitValue)
	{
		const HRESULT result = m_pFence->SetEventOnCompletion(m_waitValue, m_pEvent->GetHandle());
		if(result != S_OK)
		{
			LOG_ERROR("Failed to set completion event on fence; result='0x%08" PRIX32 "'", result);
			return;
		}

		::WaitForSingleObject(m_pEvent->GetHandle(), timeout);
	}
}

//---------------------------------------------------------------------------------------------------------------------
