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

#include "Texture2D.hpp"

#include "Sync.hpp"

#include "LowLevel/DescriptorHeap.hpp"
#include "LowLevel/Resource.hpp"

#include "../Application/Log.hpp"

#include <DirectXTex.h>
#include <stb_image.h>

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Texture2D::Ptr DemoFramework::D3D12::Texture2D::Load(
	const Device::Ptr& device,
	const CommandQueue::Ptr& cmdQueue,
	const GraphicsCommandContext::Ptr& uploadCmdCtx,
	const DataType dataType,
	const Channel channel,
	const char* const filePath,
	const uint32_t mipCount)
{
	if(!device || !cmdQueue || !uploadCmdCtx || !filePath || filePath[0] == '\0' || mipCount == 0)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t channelCount = 0;
	uint32_t channelSize = 0;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	void* pImgData = nullptr;

	switch(channel)
	{
		case Channel::L:    channelCount = 1; break;
		case Channel::LA:   channelCount = 2; break;
		case Channel::RGBA: channelCount = 4; break;

		default:
			LOG_ERROR("Invalid parameter");
			return Ptr();
	}

	switch(dataType)
	{
		case DataType::Unorm:
		{
			pImgData = stbi_load(filePath, reinterpret_cast<int32_t*>(&width), reinterpret_cast<int32_t*>(&height), nullptr, channelCount);
			channelSize = sizeof(uint8_t);

			switch(channel)
			{
				case Channel::L:    format = DXGI_FORMAT_R8_UNORM;       break;
				case Channel::LA:   format = DXGI_FORMAT_R8G8_UNORM;     break;
				case Channel::RGBA: format = DXGI_FORMAT_R8G8B8A8_UNORM; break;

				default:
					// This should never happen.
					assert(false);
					break;
			}
			break;
		}

		case DataType::Float:
		{
			pImgData = stbi_loadf(filePath, reinterpret_cast<int32_t*>(&width), reinterpret_cast<int32_t*>(&height), nullptr, channelCount);
			channelSize = sizeof(float32_t);

			switch(channel)
			{
				case Channel::L:    format = DXGI_FORMAT_R32_FLOAT;          break;
				case Channel::LA:   format = DXGI_FORMAT_R32G32_FLOAT;       break;
				case Channel::RGBA: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;

				default:
					// This should never happen.
					assert(false);
					break;
			}
			break;
		}

		default:
			LOG_ERROR("Invalid parameter");
			return Ptr();
	}

	if(!pImgData)
	{
		LOG_ERROR("Failed to load image file: %s", filePath);
		return Ptr();
	}

	auto calculateMipLevelMaxCount = [&width, &height]() -> uint32_t
	{
		uint32_t mipWidth = width;
		uint32_t mipHeight = height;
		uint32_t mipLevels = 1;

		while(mipWidth > 1 || mipHeight > 1)
		{
			if(mipWidth > 1)
			{
				mipWidth >>= 1;
			}

			if(mipHeight > 1)
			{
				mipHeight >>= 1;
			}

			++mipLevels;
		}

		return mipLevels;
	};

	const uint32_t mipLevelMaxCount = calculateMipLevelMaxCount();
	const uint32_t mipLevelCount = (mipCount < mipLevelMaxCount) ? mipCount : mipLevelMaxCount;

	auto alignStagingDataPitch = [](const uint32_t basePitch) -> uint32_t
	{
		return (basePitch + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
	};

	auto alignStagingDataPlacement = [](const uint32_t levelOffset) -> uint32_t
	{
		return (levelOffset + (D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);
	};

	DirectX::Image baseImage;
	baseImage.width = size_t(width);
	baseImage.height = size_t(height);
	baseImage.format = format;
	baseImage.rowPitch = size_t(width * channelCount * channelSize);
	baseImage.slicePitch = baseImage.height * baseImage.rowPitch;
	baseImage.pixels = reinterpret_cast<uint8_t*>(pImgData);

	DirectX::ScratchImage mipChain;

	if(mipLevelCount > 1)
	{
		DirectX::GenerateMipMaps(baseImage, DirectX::TEX_FILTER_BOX, size_t(mipLevelCount), mipChain);
	}
	else
	{
		mipChain.InitializeFromImage(baseImage);
	}

	// Free the original image data now that we no longer need it.
	stbi_image_free(pImgData);

	auto calculateStagingDataSize = [
		&width,
		&height,
		&channelCount,
		&channelSize,
		&mipLevelCount,
		alignStagingDataPitch,
		alignStagingDataPlacement]() -> uint32_t
	{
		const uint32_t texelSize = channelSize * channelCount;

		uint32_t mipWidth = width;
		uint32_t mipHeight = height;

		uint32_t totalSize = 0;

		for(uint32_t mipIndex = 0; mipIndex < mipLevelCount; ++mipIndex)
		{
			// Adjust the total size by the required alignment of the current level offset.
			totalSize = alignStagingDataPlacement(totalSize);

			const uint32_t rowPitch = alignStagingDataPitch(mipWidth * texelSize);
			const uint32_t levelSize = mipHeight * rowPitch;

			totalSize += levelSize;

			if(mipWidth > 1)
			{
				mipWidth >>= 1;
			}

			if(mipHeight > 1)
			{
				mipHeight >>= 1;
			}
		}

		return totalSize;
	};

	const uint32_t stagingTotalSize = calculateStagingDataSize();

	constexpr D3D12_HEAP_PROPERTIES gpuHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,         // D3D12_HEAP_TYPE Type
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
		D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
		0,                               // UINT CreationNodeMask
		0,                               // UINT VisibleNodeMask
	};

	constexpr D3D12_HEAP_PROPERTIES uploadHeapProps =
	{
		D3D12_HEAP_TYPE_UPLOAD,          // D3D12_HEAP_TYPE Type
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
		D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
		0,                               // UINT CreationNodeMask
		0,                               // UINT VisibleNodeMask
	};

	constexpr D3D12_DESCRIPTOR_HEAP_DESC heapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    // D3D12_DESCRIPTOR_HEAP_TYPE Type
		1,                                         // UINT NumDescriptors
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		0,                                         // UINT NodeMask
	};

	constexpr DXGI_SAMPLE_DESC defaultSampleDesc =
	{
		1, // UINT Count
		0, // UINT Quality
	};

	const D3D12_RESOURCE_DESC gpuResDesc =
	{
		D3D12_RESOURCE_DIMENSION_TEXTURE2D, // D3D12_RESOURCE_DIMENSION Dimension
		0,                                  // UINT64 Alignment
		uint64_t(width),                    // UINT64 Width
		height,                             // UINT Height
		1,                                  // UINT16 DepthOrArraySize
		uint16_t(mipLevelCount),            // UINT16 MipLevels
		format,                             // DXGI_FORMAT Format
		defaultSampleDesc,                  // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_UNKNOWN,       // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_NONE,           // D3D12_RESOURCE_FLAGS Flags
	};

	const D3D12_RESOURCE_DESC stagingResDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER, // D3D12_RESOURCE_DIMENSION Dimension
		0,                               // UINT64 Alignment
		uint64_t(stagingTotalSize),      // UINT64 Width
		1,                               // UINT Height
		1,                               // UINT16 DepthOrArraySize
		1,                               // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,             // DXGI_FORMAT Format
		defaultSampleDesc,               // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,  // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_NONE,        // D3D12_RESOURCE_FLAGS Flags
	};

	constexpr D3D12_RANGE disableCpuReadRange =
	{
		0, // SIZE_T Begin
		0, // SIZE_T End
	};

	// Create the staging texture resource.
	Resource::Ptr stagingTexture = CreateCommittedResource(
		device,
		stagingResDesc,
		uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	if(!stagingTexture)
	{
		return Ptr();
	}

	// Create the GPU-resident texture resource.
	Resource::Ptr gpuTexture = CreateCommittedResource(
		device,
		gpuResDesc,
		gpuHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	if(!gpuTexture)
	{
		return Ptr();
	}

	// Create the texture's descriptor heap.
	DescriptorHeap::Ptr heap = CreateDescriptorHeap(device, heapDesc);
	if(!heap)
	{
		return Ptr();
	}

	D3D12_TEXTURE_COPY_LOCATION srcLoc;
	srcLoc.pResource = stagingTexture.Get();
	srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLoc.PlacedFootprint.Footprint.Format = format;

	D3D12_TEXTURE_COPY_LOCATION destLoc;
	destLoc.pResource = gpuTexture.Get();
	destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	const GraphicsCommandList::Ptr& cmdList = uploadCmdCtx->GetCmdList();

	void* pStagingData = nullptr;

	// Map the staging texture.
	const HRESULT mapResult = stagingTexture->Map(0, &disableCpuReadRange, &pStagingData);
	assert(SUCCEEDED(mapResult)); (void) mapResult;

	uint32_t outputOffset = 0;

	// Copy each mip level of the input image to the staging texture.
	for(uint32_t mipIndex = 0; mipIndex < uint32_t(gpuResDesc.MipLevels); ++mipIndex)
	{
		const DirectX::Image* const pMipImage = mipChain.GetImage(mipIndex, 0, 0);

		const uint32_t stagingPitch = alignStagingDataPitch(uint32_t(pMipImage->rowPitch));

		// Correct the current mip level's staging offset since the end of the
		// last level may not be packed against the start of the current level.
		outputOffset = alignStagingDataPlacement(outputOffset);

		srcLoc.PlacedFootprint.Offset = outputOffset;
		srcLoc.PlacedFootprint.Footprint.Width = uint32_t(pMipImage->width);
		srcLoc.PlacedFootprint.Footprint.Height = uint32_t(pMipImage->height);
		srcLoc.PlacedFootprint.Footprint.Depth = 1;
		srcLoc.PlacedFootprint.Footprint.RowPitch = stagingPitch;

		destLoc.SubresourceIndex = mipIndex;

		if(pMipImage->rowPitch != stagingPitch)
		{
			// The input image and staging texture have different row pitches, so we'll need to copy the data for one row at a time.
			for(size_t y = 0; y < size_t(pMipImage->height); ++y)
			{
				const size_t inputOffset = y * pMipImage->rowPitch;

				memcpy(
					reinterpret_cast<uint8_t*>(pStagingData) + outputOffset,
					pMipImage->pixels + inputOffset,
					pMipImage->rowPitch);

				// Update the staging data offset.
				outputOffset += stagingPitch;
			}
		}
		else
		{
			// The staging texture and image data both have the same pitch, meaning we can copy all the data in one batch.
			memcpy(reinterpret_cast<uint8_t*>(pStagingData) + outputOffset, pMipImage->pixels, pMipImage->slicePitch);

			// Update the staging data offset.
			outputOffset += uint32_t(pMipImage->slicePitch);
		}

		// Copy the staging data to the GPU-resident texture.
		cmdList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
	}

	// Unmap the staging texture.
	stagingTexture->Unmap(0, nullptr);

	// Release the mip chain now that all of its data has been copied into the staging image.
	mipChain.Release();

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = gpuTexture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	cmdList->ResourceBarrier(1, &barrier);

	// Create the staging command synchronization primitive so we can wait for
	// all staging resource copies to complete at the end of initialization.
	D3D12::Sync::Ptr cmdSync = D3D12::Sync::Create(device, D3D12_FENCE_FLAG_NONE);
	if(!cmdSync)
	{
		return Ptr();
	}

	// Stop recording commands in the staging command list and begin executing it.
	uploadCmdCtx->Submit(cmdQueue);

	// Wait for the staging command list to finish executing.
	cmdSync->Signal(cmdQueue);
	cmdSync->Wait();

	// Reset the command list so it can be used again.
	uploadCmdCtx->Reset();

	Ptr output = std::make_shared<Texture2D>();

	output->m_heap = heap;
	output->m_resource = gpuTexture;
	output->m_cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
	output->m_gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
	output->m_width = width;
	output->m_height = height;
	output->m_format = format;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = gpuResDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = uint32_t(gpuResDesc.MipLevels);
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// Create the SRV from the resource.
	device->CreateShaderResourceView(gpuTexture.Get(), &srvDesc, output->m_cpuHandle);

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
