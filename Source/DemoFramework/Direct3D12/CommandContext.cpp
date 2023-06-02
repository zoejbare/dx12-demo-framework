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

#include "CommandContext.hpp"

#include "LowLevel/CommandAllocator.hpp"
#include "LowLevel/GraphicsCommandList.hpp"

#include "../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::GraphicsCommandContext::Initialize(const DevicePtr& device, const D3D12_COMMAND_LIST_TYPE type)
{
	if(!device)
	{
		LOG_ERROR("Invalid parameter");
		return false;
	}
	else if(m_initialized)
	{
		LOG_ERROR("Already initialized");
		return false;
	}

	// Create a command allocator.
	m_cmdAlloc = CreateCommandAllocator(device, type);
	if(!m_cmdAlloc)
	{
		return false;
	}

	// Create a command list.
	m_cmdList = D3D12::CreateGraphicsCommandList(device, m_cmdAlloc, type, 0);
	if(!m_cmdList)
	{
		return false;
	}

	// Command lists start in the recording state, so for consistency, we'll close them after they're created.
	m_cmdList->Close();

	m_initialized = true;

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::GraphicsCommandContext::Reset()
{
	if(m_initialized)
	{
		m_cmdAlloc->Reset();
		m_cmdList->Reset(m_cmdAlloc.Get(), nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::GraphicsCommandContext::Submit(const CommandQueuePtr& cmdQueue)
{
	if(m_initialized && cmdQueue)
	{
		ID3D12CommandList* const pCmdLists[] =
		{
			m_cmdList.Get(),
		};

		// Stop recording commands.
		m_cmdList->Close();

		// Immediately begin executing the command list.
		cmdQueue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);
	}
}

//---------------------------------------------------------------------------------------------------------------------
