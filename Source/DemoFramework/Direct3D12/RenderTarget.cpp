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

#include "RenderTarget.hpp"

#include "LowLevel/DescriptorHeap.hpp"
#include "LowLevel/Resource.hpp"

#include "../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::RenderTarget::Ptr DemoFramework::D3D12::RenderTarget::Create(
	const Device::Ptr& device,
	const uint32_t width,
	const uint32_t height,
	const DXGI_FORMAT format)
{
	if(!device || width == 0 || height == 0)
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
			LOG_ERROR("Invalid format");
			return Ptr();

		default:
			break;
	}

	Ptr output = std::make_shared<RenderTarget>();

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

	constexpr D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,  // D3D12_DESCRIPTOR_HEAP_TYPE Type
		1,                               // UINT NumDescriptors
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE, // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		0,                               // UINT NodeMask
	};

	constexpr D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    // D3D12_DESCRIPTOR_HEAP_TYPE Type
		1,                                         // UINT NumDescriptors
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		0,                                         // UINT NodeMask
	};

	constexpr D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	constexpr D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
	constexpr D3D12_RESOURCE_STATES initialStates = D3D12_RESOURCE_STATE_RENDER_TARGET;

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
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	// Create the render target backing resource.
	output->m_resource = CreateCommittedResource(device, resourceDesc, heapProps, heapFlags, initialStates, &clearValue);
	if(!output->m_resource)
	{
		return Ptr();
	}

	// Create the RTV descriptor heap.
	output->m_rtvHeap = CreateDescriptorHeap(device, rtvHeapDesc);
	if(!output->m_rtvHeap)
	{
		return Ptr();
	}

	// Create the SRV descriptor heap.
	output->m_srvHeap = CreateDescriptorHeap(device, srvHeapDesc);
	if(!output->m_srvHeap)
	{
		return Ptr();
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// Get the RTV & SRV handles for their respective heaps.
	output->m_rtvHandle = output->m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	output->m_srvHandle = output->m_srvHeap->GetCPUDescriptorHandleForHeapStart();

	// Create the views.
	device->CreateRenderTargetView(output->m_resource.Get(), &rtvDesc, output->m_rtvHandle);
	device->CreateShaderResourceView(output->m_resource.Get(), &srvDesc, output->m_srvHandle);

	output->m_currentStates = initialStates;
	output->m_initialized = true;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::RenderTarget::TransitionTo(const GraphicsCommandList::Ptr& cmdList, const D3D12_RESOURCE_STATES states)
{
	if(m_initialized && states != m_currentStates)
	{
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = m_resource.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = m_currentStates;
		barrier.Transition.StateAfter = states;

		cmdList->ResourceBarrier(1, &barrier);

		m_currentStates = states;
	}
}

//---------------------------------------------------------------------------------------------------------------------
