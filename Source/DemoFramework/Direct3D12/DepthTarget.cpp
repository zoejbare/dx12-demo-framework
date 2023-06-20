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

#include "DepthTarget.hpp"

#include "LowLevel/DescriptorHeap.hpp"
#include "LowLevel/Resource.hpp"

#include "../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::DepthTarget::Ptr DemoFramework::D3D12::DepthTarget::Create(
	const Device::Ptr& device,
	const DescriptorAllocator::Ptr& dsvAlloc,
	const uint32_t width,
	const uint32_t height,
	const DXGI_FORMAT format)
{
	if(!device || !dsvAlloc || width == 0 || height == 0)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	switch(format)
	{
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			break;

		default:
			LOG_ERROR("Invalid format");
			return Ptr();
	}

	Ptr output = std::make_shared<DepthTarget>();

	output->m_alloc = dsvAlloc;

	constexpr DXGI_SAMPLE_DESC sampleDesc =
	{
		1, // UINT Count
		0, // UINT Quality
	};

	constexpr D3D12_HEAP_PROPERTIES heapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,         // D3D12_HEAP_TYPE Type
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
		D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
		0,                               // UINT CreationNodeMask
		0,                               // UINT VisibleNodeMask
	};

	constexpr D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	constexpr D3D12_RESOURCE_STATES initialStates = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	const D3D12_RESOURCE_DESC resourceDesc =
	{
		D3D12_RESOURCE_DIMENSION_TEXTURE2D, // D3D12_RESOURCE_DIMENSION Dimension
		0,                                  // UINT64 Alignment
		uint64_t(width),                    // UINT64 Width
		height,                             // UINT Height
		1,                                  // UINT16 DepthOrArraySize
		1,                                  // UINT16 MipLevels
		format,                             // DXGI_FORMAT Format
		sampleDesc,                         // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_UNKNOWN,       // D3D12_TEXTURE_LAYOUT Layout
		resourceFlags,                      // D3D12_RESOURCE_FLAGS Flags
	};

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = format;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	// Create the render target backing resource.
	output->m_resource = CreateCommittedResource(device, resourceDesc, heapProps, D3D12_HEAP_FLAG_NONE, initialStates, &clearValue);
	if(!output->m_resource)
	{
		return Ptr();
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = format;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Texture2D.MipSlice = 0;

	const Descriptor descriptor = dsvAlloc->Allocate();
	assert(descriptor.index != Descriptor::Invalid.index);

	// Create the view.
	device->CreateDepthStencilView(output->m_resource.Get(), &dsvDesc, descriptor.cpuHandle);

	output->m_alloc = dsvAlloc;
	output->m_descriptor = descriptor;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
