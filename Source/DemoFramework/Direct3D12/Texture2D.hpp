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
#include "DescriptorAllocator.hpp"

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
	~Texture2D();

	Texture2D& operator =(const Texture2D&) = delete;
	Texture2D& operator =(Texture2D&&) = delete;

	static Ptr Load(
		const Device::Ptr& device,
		const GraphicsCommandList::Ptr& uploadCmdList,
		const DescriptorAllocator::Ptr& srvAlloc,
		DataType dataType,
		Channel channel,
		const char* filePath,
		uint32_t mipCount = D3D12_REQ_MIP_LEVELS
	);

	const DescriptorAllocator::Ptr& GetSrvAllocator() const;
	const Descriptor& GetDescriptor() const;

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetMipCount() const;
	DXGI_FORMAT GetFormat() const;


private:

	Resource::Ptr m_resource;
	Resource::Ptr m_staging;

	DescriptorAllocator::Ptr m_alloc;

	Descriptor m_descriptor;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_mipCount;

	DXGI_FORMAT m_format;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::Texture2D>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Texture2D::Texture2D()
	: m_resource()
	, m_staging()
	, m_alloc()
	, m_descriptor()
	, m_width(0)
	, m_height(0)
	, m_mipCount(0)
	, m_format(DXGI_FORMAT_UNKNOWN)
{
}

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Texture2D::~Texture2D()
{
	if(m_alloc)
	{
		m_alloc->Free(m_descriptor);
	}
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::DescriptorAllocator::Ptr& DemoFramework::D3D12::Texture2D::GetSrvAllocator() const
{
	return m_alloc;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::Descriptor& DemoFramework::D3D12::Texture2D::GetDescriptor() const
{
	return m_descriptor;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::Texture2D::GetWidth() const
{
	return m_width;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::Texture2D::GetHeight() const
{
	return m_height;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::Texture2D::GetMipCount() const
{
	return m_mipCount;
}

//---------------------------------------------------------------------------------------------------------------------

inline DXGI_FORMAT DemoFramework::D3D12::Texture2D::GetFormat() const
{
	return m_format;
}

//---------------------------------------------------------------------------------------------------------------------
