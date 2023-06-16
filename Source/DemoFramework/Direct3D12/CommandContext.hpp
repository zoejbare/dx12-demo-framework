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

#include <memory>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class GraphicsCommandContext;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::GraphicsCommandContext
{
public:

	typedef std::shared_ptr<GraphicsCommandContext> Ptr;

	GraphicsCommandContext();
	GraphicsCommandContext(const GraphicsCommandContext&) = delete;
	GraphicsCommandContext(GraphicsCommandContext&&) = delete;

	GraphicsCommandContext& operator =(const GraphicsCommandContext&) = delete;
	GraphicsCommandContext& operator =(GraphicsCommandContext&&) = delete;

	static Ptr Create(const Device::Ptr& device, D3D12_COMMAND_LIST_TYPE type);

	void Reset();
	void Submit(const CommandQueue::Ptr& cmdQueue);

	const CommandAllocator::Ptr& GetCmdAlloc() const;
	const GraphicsCommandList::Ptr& GetCmdList() const;


private:

	CommandAllocator::Ptr m_cmdAlloc;
	GraphicsCommandList::Ptr m_cmdList;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::GraphicsCommandContext>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::GraphicsCommandContext::GraphicsCommandContext()
	: m_cmdAlloc()
	, m_cmdList()
	, m_initialized(false)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::CommandAllocator::Ptr& DemoFramework::D3D12::GraphicsCommandContext::GetCmdAlloc() const
{
	return m_cmdAlloc;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::GraphicsCommandList::Ptr& DemoFramework::D3D12::GraphicsCommandContext::GetCmdList() const
{
	return m_cmdList;
}

//---------------------------------------------------------------------------------------------------------------------
