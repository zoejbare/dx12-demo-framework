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

#include "CommandContext.hpp"

#include <memory>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class Texture2D;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::Texture2D
{
public:

	enum class DataType
	{
		Unorm,
		Float,
	};

	enum class Channel
	{
		L,    // 1-channel: luminance
		LA,   // 2-channel: luminance, alpha
		RGBA, // 4-channel: red, green, blue, alpha
	};

	typedef std::shared_ptr<Texture2D> Ptr;

	Texture2D();
	Texture2D(const Texture2D&) = delete;
	Texture2D(Texture2D&&) = delete;

	Texture2D& operator =(const Texture2D&) = delete;
	Texture2D& operator =(Texture2D&&) = delete;

	static Ptr Load(
		const DevicePtr& device,
		const CommandQueuePtr& cmdQueue,
		const GraphicsCommandContext::Ptr& uploadCmdCtx,
		DataType dataType,
		Channel channel,
		const char* filePath,
		uint32_t mipCount = D3D12_REQ_MIP_LEVELS
	);

	const DescriptorHeapPtr& GetHeap() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;


private:

	ResourcePtr m_resource;
	DescriptorHeapPtr m_heap;

	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;

	bool m_initialized;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::Texture2D>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Texture2D::Texture2D()
	: m_resource()
	, m_heap()
	, m_cpuHandle({0})
	, m_gpuHandle({0})
	, m_initialized(false)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::DescriptorHeapPtr& DemoFramework::D3D12::Texture2D::GetHeap() const
{
	return m_heap;
}

//---------------------------------------------------------------------------------------------------------------------

inline D3D12_GPU_DESCRIPTOR_HANDLE DemoFramework::D3D12::Texture2D::GetGpuHandle() const
{
	return m_gpuHandle;
}

//---------------------------------------------------------------------------------------------------------------------
