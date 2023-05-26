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

#include "LowLevel/Types.hpp"

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class Sync;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::Sync
{
public:

	Sync();
	Sync(const Sync&) = delete;
	Sync(Sync&&) = delete;

	Sync& operator =(const Sync&) = delete;
	Sync& operator =(Sync&&) = delete;

	bool Initialize(const DevicePtr& pDevice, D3D12_FENCE_FLAGS flags);

	void Signal(const CommandQueuePtr& pCmdQueue);
	void Wait(uint32_t timeout = INFINITE);


private:

	FencePtr m_pFence;
	EventPtr m_pEvent;

	uint64_t m_waitValue;
	uint64_t m_nextValue;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Sync::Sync()
	: m_pFence()
	, m_pEvent()
	, m_waitValue(0)
	, m_nextValue(0)
	, m_initialized(false)
{
}

//---------------------------------------------------------------------------------------------------------------------
