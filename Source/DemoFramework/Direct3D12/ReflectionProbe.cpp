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

#include "ReflectionProbe.hpp"
#include "Shader.hpp"
#include "Sync.hpp"
#include "Texture2D.hpp"

#include "LowLevel/DescriptorHeap.hpp"
#include "LowLevel/PipelineState.hpp"
#include "LowLevel/Resource.hpp"
#include "LowLevel/RootSignature.hpp"

#include "Shaders/reflection-probe/common.hlsli"

#include "../Application/Log.hpp"

#include <DirectXTex.h>
#include <stb_image.h>

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::ReflectionProbe::~ReflectionProbe()
{
	if(m_alloc)
	{
		for(uint32_t faceIndex = 0; faceIndex < DF_CUBE_FACE__COUNT; ++faceIndex)
		{
			for(uint32_t mipIndex = 0; mipIndex < m_envMipCount; ++mipIndex)
			{
				m_alloc->Free(m_envFaceUavDescriptor[(faceIndex * DF_CUBE_FACE__COUNT) + mipIndex]);
			}

			m_alloc->Free(m_irrFaceUavDescriptor[faceIndex]);
		}

		m_alloc->Free(m_envSrvDescriptor);
		m_alloc->Free(m_irrSrvDescriptor);
		m_alloc->Free(m_coeffSrvDescriptor);
		m_alloc->Free(m_coeffUavDescriptor);
		m_alloc->Free(m_weightSrvDescriptor);
		m_alloc->Free(m_weightUavDescriptor);
	}
}

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::ReflectionProbe::Ptr DemoFramework::D3D12::ReflectionProbe::Create(
	const Device::Ptr& device,
	const GraphicsCommandList::Ptr& cmdList,
	const DescriptorAllocator::Ptr& srvUavAlloc,
	const EnvMapQuality mapQuality)
{
	if(!device
		|| !cmdList
		|| !srvUavAlloc)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	uint32_t envMapEdgeLength = 0;
	uint32_t irrMapEdgeLength = 0;

	switch(mapQuality)
	{
		case EnvMapQuality::Low:
			envMapEdgeLength = 512;
			irrMapEdgeLength = 32;
			break;

		case EnvMapQuality::Mid:
			envMapEdgeLength = 1024;
			irrMapEdgeLength = 64;
			break;

		case EnvMapQuality::High:
			envMapEdgeLength = 2048;
			irrMapEdgeLength = 128;
			break;

		default:
			LOG_ERROR("Invalid parameter");
			break;
	}

	auto calculateMipCount = [&envMapEdgeLength]() -> uint32_t
	{
		uint32_t mipWidth = envMapEdgeLength;
		uint32_t mipLevels = 1;

		while(mipWidth > 1)
		{
			if(mipWidth > 1)
			{
				mipWidth >>= 1;
			}

			++mipLevels;
		}

		return mipLevels;
	};

	Ptr output = std::make_shared<ReflectionProbe>();

	output->m_alloc = srvUavAlloc;
	output->m_envMipCount = calculateMipCount();
	output->m_envEdgeLength = envMapEdgeLength;
	output->m_irrEdgeLength = irrMapEdgeLength;

	// Create the equi-to-cube pipeline.
	if(!output->_initEquiToCubePipeline(device))
	{
		return Ptr();
	}

	// Create the sh-project pipeline.
	if(!output->_initShProjectPipeline(device))
	{
		return Ptr();
	}

	// Create the sh-reduce pipeline.
	if(!output->_initShReducePipeline(device))
	{
		return Ptr();
	}

	// Create the sh-normalize pipeline.
	if(!output->_initShNormalizePipeline(device))
	{
		return Ptr();
	}

	// Create the sh-reconstruct pipeline.
	if(!output->_initShReconstructPipeline(device))
	{
		return Ptr();
	}

	// Create the environment and irradiance cube maps.
	if(!output->_initCubeMaps(device))
	{
		return Ptr();
	}

	// Create the spherical harmonics UAV resources.
	if(!output->_initUavResources(device))
	{
		return Ptr();
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::ReflectionProbe::LoadEnvironmentMap(
	const Device::Ptr& device,
	const GraphicsCommandList::Ptr& cmdList,
	const Texture2D::Ptr& envTexture)
{
	constexpr DXGI_FORMAT expectedFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

	if(!device || !cmdList || !envTexture)
	{
		LOG_ERROR("Invalid parameter");
		return false;
	}
	else if(envTexture->GetFormat() != expectedFormat)
	{
		LOG_ERROR("Environment texture does not have the correct format");
		return false;
	}
	else if(envTexture->GetWidth() != envTexture->GetHeight() * 2)
	{
		LOG_ERROR(
			"Invalid environment texture dimensions; width must be equal to double the height: width=%" PRIu32 ", height=%" PRIu32,
			envTexture->GetWidth(), envTexture->GetHeight());
		return false;
	}

	ID3D12DescriptorHeap* const pDescHeaps[] =
	{
		m_alloc->GetHeap().Get(),
	};

	cmdList->SetDescriptorHeaps(_countof(pDescHeaps), pDescHeaps);

	// Convert the equirectangular environment map to a cubemap.
	{
		EquiToCubeRootConstant constData;

		D3D12_RESOURCE_BARRIER envMapBarrier[2];
		envMapBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		envMapBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		envMapBarrier[0].Transition.pResource = m_envResource.Get();
		envMapBarrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		envMapBarrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		envMapBarrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		envMapBarrier[1] = envMapBarrier[0];
		envMapBarrier[1].Transition.StateBefore = envMapBarrier[0].Transition.StateAfter;
		envMapBarrier[1].Transition.StateAfter = envMapBarrier[0].Transition.StateBefore;

		// Transition the environment map back to a UAV.
		cmdList->ResourceBarrier(1, &envMapBarrier[0]);

		cmdList->SetComputeRootSignature(m_equiToCubeRootSig.Get());
		cmdList->SetPipelineState(m_equiToCubePipeline.Get());

		uint32_t mipSize = m_envEdgeLength;

		// Process each mip level of the cube map.
		for(uint32_t mipIndex = 0; mipIndex < m_envMipCount; ++mipIndex)
		{
			uint32_t groupCountX = mipSize / DF_REFL_THREAD_COUNT_X;
			uint32_t groupCountY = mipSize / DF_REFL_THREAD_COUNT_Y;

			if(groupCountX == 0)
			{
				groupCountX = 1;
			}

			if(groupCountY == 0)
			{
				groupCountY = 1;
			}

			// Set the constant properties that will not change for the current mip level.
			constData.mipIndex = mipIndex;
			constData.edgeLength = mipSize;
			constData.invEdgeLength = 1.0f / float32_t(mipSize);

			cmdList->SetComputeRootDescriptorTable(1, envTexture->GetDescriptor().gpuHandle);

			// Process each face of the cube map for the current mip level.
			for(uint32_t faceIndex = 0; faceIndex < DF_CUBE_FACE__COUNT; ++faceIndex)
			{
				// Set the face-specific constant properties.
				constData.faceIndex = faceIndex;

				D3D12_UNORDERED_ACCESS_VIEW_DESC faceUavDesc;
				faceUavDesc.Format = expectedFormat;
				faceUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				faceUavDesc.Texture2DArray.MipSlice = mipIndex;
				faceUavDesc.Texture2DArray.FirstArraySlice = faceIndex;
				faceUavDesc.Texture2DArray.ArraySize = 1;
				faceUavDesc.Texture2DArray.PlaneSlice = 0;

				const Descriptor faceDescriptor = m_alloc->AllocateTemp();
				assert(faceDescriptor.index != Descriptor::Invalid.index);

				device->CreateUnorderedAccessView(m_envResource.Get(), nullptr, &faceUavDesc, faceDescriptor.cpuHandle);

				cmdList->SetComputeRoot32BitConstants(0, sizeof(EquiToCubeRootConstant) / sizeof(uint32_t), &constData, 0);
				cmdList->SetComputeRootDescriptorTable(2, faceDescriptor.gpuHandle);
				cmdList->Dispatch(groupCountX, groupCountY, 1);
			}

			mipSize >>= 1;
		}

		// Transition the environment map to an SRV.
		cmdList->ResourceBarrier(1, &envMapBarrier[1]);
	}

	_generateIrradiance(cmdList);

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::ReflectionProbe::_initEquiToCubePipeline(const Device::Ptr& device)
{
	const char* const shaderFilePath = "shaders/framework/equi-to-cube.cs.sbin";

	// Load the shader from the specified file path.
	Blob::Ptr shader = LoadShaderFromFile(shaderFilePath);
	if(!shader)
	{
		LOG_ERROR("Failed to load shader: %s", shaderFilePath);
		return false;
	}

	const D3D12_SHADER_BYTECODE shaderBytecode =
	{
		shader->GetBufferPointer(), // const void *pShaderBytecode
		shader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	constexpr D3D12_DESCRIPTOR_RANGE srvDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	constexpr D3D12_DESCRIPTOR_RANGE uavDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	// Root constants
	D3D12_ROOT_PARAMETER rootConstParam;
	rootConstParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootConstParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootConstParam.Constants.ShaderRegister = 0;
	rootConstParam.Constants.RegisterSpace = 0;
	rootConstParam.Constants.Num32BitValues = sizeof(EquiToCubeRootConstant) / sizeof(uint32_t);

	// SRV table
	D3D12_ROOT_PARAMETER srvTableParam;
	srvTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	srvTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	srvTableParam.DescriptorTable.NumDescriptorRanges = 1;
	srvTableParam.DescriptorTable.pDescriptorRanges = &srvDescRange;

	// UAV table
	D3D12_ROOT_PARAMETER uavTableParam;
	uavTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	uavTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	uavTableParam.DescriptorTable.NumDescriptorRanges = 1;
	uavTableParam.DescriptorTable.pDescriptorRanges = &uavDescRange;

	const D3D12_ROOT_PARAMETER rootParams[] =
	{
		rootConstParam,
		srvTableParam,
		uavTableParam,
	};

	const D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = _getStaticSamplerDesc();

	constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	const D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
	{
		_countof(rootParams), // UINT NumParameters
		rootParams,           // const D3D12_ROOT_PARAMETER *pParameters
		1,                    // UINT NumStaticSamplers
		&staticSamplerDesc,   // const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers
		rootSigFlags,         // D3D12_ROOT_SIGNATURE_FLAGS Flags
	};

	// Create the shader root signature.
	m_equiToCubeRootSig = CreateRootSignature(device, rootSigDesc);
	if(!m_equiToCubeRootSig)
	{
		LOG_ERROR("Failed to create root signature for equi-to-cube shader");
		return false;
	}

	const D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc =
	{
		m_equiToCubeRootSig.Get(),      // ID3D12RootSignature *pRootSignature
		shaderBytecode,                 // D3D12_SHADER_BYTECODE CS
		0,                              // UINT NodeMask
		{},                             // D3D12_CACHED_PIPELINE_STATE CachedPSO
		D3D12_PIPELINE_STATE_FLAG_NONE, // D3D12_PIPELINE_STATE_FLAGS Flags
	};

	// Create the shader pipeline.
	m_equiToCubePipeline = CreatePipelineState(device, pipelineDesc);
	if(!m_equiToCubePipeline)
	{
		LOG_ERROR("Failed to create pipeline for equi-to-cube shader");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::ReflectionProbe::_initShProjectPipeline(const Device::Ptr& device)
{
	const char* const shaderFilePath = "shaders/framework/sh-project.cs.sbin";

	// Load the shader from the specified file path.
	Blob::Ptr shader = LoadShaderFromFile(shaderFilePath);
	if(!shader)
	{
		LOG_ERROR("Failed to load shader: %s", shaderFilePath);
		return false;
	}

	const D3D12_SHADER_BYTECODE shaderBytecode =
	{
		shader->GetBufferPointer(), // const void *pShaderBytecode
		shader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	constexpr D3D12_DESCRIPTOR_RANGE srvDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	constexpr D3D12_DESCRIPTOR_RANGE uavCoeffDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	constexpr D3D12_DESCRIPTOR_RANGE uavWeightDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		1,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	// Root constants
	D3D12_ROOT_PARAMETER rootConstParam;
	rootConstParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootConstParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootConstParam.Constants.ShaderRegister = 0;
	rootConstParam.Constants.RegisterSpace = 0;
	rootConstParam.Constants.Num32BitValues = sizeof(ShProjectRootConstant) / sizeof(uint32_t);

	// SRV table
	D3D12_ROOT_PARAMETER srvTableParam;
	srvTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	srvTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	srvTableParam.DescriptorTable.NumDescriptorRanges = 1;
	srvTableParam.DescriptorTable.pDescriptorRanges = &srvDescRange;

	// UAV table (SH coefficients)
	D3D12_ROOT_PARAMETER uavCoeffTableParam;
	uavCoeffTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	uavCoeffTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	uavCoeffTableParam.DescriptorTable.NumDescriptorRanges = 1;
	uavCoeffTableParam.DescriptorTable.pDescriptorRanges = &uavCoeffDescRange;

	// UAV table (SH weights)
	D3D12_ROOT_PARAMETER uavWeightTableParam;
	uavWeightTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	uavWeightTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	uavWeightTableParam.DescriptorTable.NumDescriptorRanges = 1;
	uavWeightTableParam.DescriptorTable.pDescriptorRanges = &uavWeightDescRange;

	// The SH coefficients and weights are split into two separate tables because
	// we can't guarantee their descriptors will be contiguous in memory with our
	// descriptor allocator.
	const D3D12_ROOT_PARAMETER rootParams[] =
	{
		rootConstParam,
		srvTableParam,
		uavCoeffTableParam,
		uavWeightTableParam,
	};

	const D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = _getStaticSamplerDesc();

	constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	const D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
	{
		_countof(rootParams), // UINT NumParameters
		rootParams,           // const D3D12_ROOT_PARAMETER *pParameters
		1,                    // UINT NumStaticSamplers
		&staticSamplerDesc,   // const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers
		rootSigFlags,         // D3D12_ROOT_SIGNATURE_FLAGS Flags
	};

	// Create the shader root signature.
	m_shProjectRootSig = CreateRootSignature(device, rootSigDesc);
	if(!m_shProjectRootSig)
	{
		LOG_ERROR("Failed to create root signature for sh-project shader");
		return false;
	}

	const D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc =
	{
		m_shProjectRootSig.Get(),       // ID3D12RootSignature *pRootSignature
		shaderBytecode,                 // D3D12_SHADER_BYTECODE CS
		0,                              // UINT NodeMask
		{},                             // D3D12_CACHED_PIPELINE_STATE CachedPSO
		D3D12_PIPELINE_STATE_FLAG_NONE, // D3D12_PIPELINE_STATE_FLAGS Flags
	};

	// Create the shader pipeline.
	m_shProjectPipeline = CreatePipelineState(device, pipelineDesc);
	if(!m_shProjectPipeline)
	{
		LOG_ERROR("Failed to create pipeline for sh-project shader");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::ReflectionProbe::_initShReducePipeline(const Device::Ptr& device)
{
	const char* const shaderFilePath = "shaders/framework/sh-reduce.cs.sbin";

	// Load the shader from the specified file path.
	Blob::Ptr shader = LoadShaderFromFile(shaderFilePath);
	if(!shader)
	{
		LOG_ERROR("Failed to load shader: %s", shaderFilePath);
		return false;
	}

	const D3D12_SHADER_BYTECODE shaderBytecode =
	{
		shader->GetBufferPointer(), // const void *pShaderBytecode
		shader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	constexpr D3D12_DESCRIPTOR_RANGE uavCoeffDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	constexpr D3D12_DESCRIPTOR_RANGE uavWeightDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		1,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	// Root constants
	D3D12_ROOT_PARAMETER rootConstParam;
	rootConstParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootConstParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootConstParam.Constants.ShaderRegister = 0;
	rootConstParam.Constants.RegisterSpace = 0;
	rootConstParam.Constants.Num32BitValues = sizeof(ShReduceRootConstant) / sizeof(uint32_t);

	// UAV table (SH coefficients)
	D3D12_ROOT_PARAMETER uavCoeffTableParam;
	uavCoeffTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	uavCoeffTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	uavCoeffTableParam.DescriptorTable.NumDescriptorRanges = 1;
	uavCoeffTableParam.DescriptorTable.pDescriptorRanges = &uavCoeffDescRange;

	// UAV table (SH weights)
	D3D12_ROOT_PARAMETER uavWeightTableParam;
	uavWeightTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	uavWeightTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	uavWeightTableParam.DescriptorTable.NumDescriptorRanges = 1;
	uavWeightTableParam.DescriptorTable.pDescriptorRanges = &uavWeightDescRange;

	// The SH coefficients and weights are split into two separate tables because
	// we can't guarantee their descriptors will be contiguous in memory with our
	// descriptor allocator.
	const D3D12_ROOT_PARAMETER rootParams[] =
	{
		rootConstParam,
		uavCoeffTableParam,
		uavWeightTableParam,
	};

	constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	const D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
	{
		_countof(rootParams), // UINT NumParameters
		rootParams,           // const D3D12_ROOT_PARAMETER *pParameters
		0,                    // UINT NumStaticSamplers
		nullptr,              // const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers
		rootSigFlags,         // D3D12_ROOT_SIGNATURE_FLAGS Flags
	};

	// Create the shader root signature.
	m_shReduceRootSig = CreateRootSignature(device, rootSigDesc);
	if(!m_shReduceRootSig)
	{
		LOG_ERROR("Failed to create root signature for sh-reduce shader");
		return false;
	}

	const D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc =
	{
		m_shReduceRootSig.Get(),        // ID3D12RootSignature *pRootSignature
		shaderBytecode,                 // D3D12_SHADER_BYTECODE CS
		0,                              // UINT NodeMask
		{},                             // D3D12_CACHED_PIPELINE_STATE CachedPSO
		D3D12_PIPELINE_STATE_FLAG_NONE, // D3D12_PIPELINE_STATE_FLAGS Flags
	};

	// Create the shader pipeline.
	m_shReducePipeline = CreatePipelineState(device, pipelineDesc);
	if(!m_shReducePipeline)
	{
		LOG_ERROR("Failed to create pipeline for sh-reduce shader");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::ReflectionProbe::_initShNormalizePipeline(const Device::Ptr& device)
{
	const char* const shaderFilePath = "shaders/framework/sh-normalize.cs.sbin";

	// Load the shader from the specified file path.
	Blob::Ptr shader = LoadShaderFromFile(shaderFilePath);
	if(!shader)
	{
		LOG_ERROR("Failed to load shader: %s", shaderFilePath);
		return false;
	}

	const D3D12_SHADER_BYTECODE shaderBytecode =
	{
		shader->GetBufferPointer(), // const void *pShaderBytecode
		shader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	constexpr D3D12_DESCRIPTOR_RANGE srvDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	constexpr D3D12_DESCRIPTOR_RANGE uavDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	// Root constants
	D3D12_ROOT_PARAMETER rootConstParam;
	rootConstParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootConstParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootConstParam.Constants.ShaderRegister = 0;
	rootConstParam.Constants.RegisterSpace = 0;
	rootConstParam.Constants.Num32BitValues = sizeof(ShNormalizeRootConstant) / sizeof(uint32_t);

	// SRV table
	D3D12_ROOT_PARAMETER srvTableParam;
	srvTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	srvTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	srvTableParam.DescriptorTable.NumDescriptorRanges = 1;
	srvTableParam.DescriptorTable.pDescriptorRanges = &srvDescRange;

	// UAV table
	D3D12_ROOT_PARAMETER uavTableParam;
	uavTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	uavTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	uavTableParam.DescriptorTable.NumDescriptorRanges = 1;
	uavTableParam.DescriptorTable.pDescriptorRanges = &uavDescRange;

	const D3D12_ROOT_PARAMETER rootParams[] =
	{
		rootConstParam,
		srvTableParam,
		uavTableParam,
	};

	constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	const D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
	{
		_countof(rootParams), // UINT NumParameters
		rootParams,           // const D3D12_ROOT_PARAMETER *pParameters
		0,                    // UINT NumStaticSamplers
		nullptr,              // const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers
		rootSigFlags,         // D3D12_ROOT_SIGNATURE_FLAGS Flags
	};

	// Create the shader root signature.
	m_shNormalizeRootSig = CreateRootSignature(device, rootSigDesc);
	if(!m_shNormalizeRootSig)
	{
		LOG_ERROR("Failed to create root signature for sh-normalize shader");
		return false;
	}

	const D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc =
	{
		m_shNormalizeRootSig.Get(),     // ID3D12RootSignature *pRootSignature
		shaderBytecode,                 // D3D12_SHADER_BYTECODE CS
		0,                              // UINT NodeMask
		{},                             // D3D12_CACHED_PIPELINE_STATE CachedPSO
		D3D12_PIPELINE_STATE_FLAG_NONE, // D3D12_PIPELINE_STATE_FLAGS Flags
	};

	// Create the shader pipeline.
	m_shNormalizePipeline = CreatePipelineState(device, pipelineDesc);
	if(!m_shNormalizePipeline)
	{
		LOG_ERROR("Failed to create pipeline for sh-normalize shader");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::ReflectionProbe::_initShReconstructPipeline(const Device::Ptr& device)
{
	const char* const shaderFilePath = "shaders/framework/sh-reconstruct.cs.sbin";

	// Load the shader from the specified file path.
	Blob::Ptr shader = LoadShaderFromFile(shaderFilePath);
	if(!shader)
	{
		LOG_ERROR("Failed to load shader: %s", shaderFilePath);
		return false;
	}

	const D3D12_SHADER_BYTECODE shaderBytecode =
	{
		shader->GetBufferPointer(), // const void *pShaderBytecode
		shader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	constexpr D3D12_DESCRIPTOR_RANGE srvDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	constexpr D3D12_DESCRIPTOR_RANGE uavDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	// Root constants
	D3D12_ROOT_PARAMETER rootConstParam;
	rootConstParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootConstParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootConstParam.Constants.ShaderRegister = 0;
	rootConstParam.Constants.RegisterSpace = 0;
	rootConstParam.Constants.Num32BitValues = sizeof(ShReconstructRootConstant) / sizeof(uint32_t);

	// SRV table
	D3D12_ROOT_PARAMETER srvTableParam;
	srvTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	srvTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	srvTableParam.DescriptorTable.NumDescriptorRanges = 1;
	srvTableParam.DescriptorTable.pDescriptorRanges = &srvDescRange;

	// UAV table
	D3D12_ROOT_PARAMETER uavTableParam;
	uavTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	uavTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	uavTableParam.DescriptorTable.NumDescriptorRanges = 1;
	uavTableParam.DescriptorTable.pDescriptorRanges = &uavDescRange;

	const D3D12_ROOT_PARAMETER rootParams[] =
	{
		rootConstParam,
		srvTableParam,
		uavTableParam,
	};

	constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	const D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
	{
		_countof(rootParams), // UINT NumParameters
		rootParams,           // const D3D12_ROOT_PARAMETER *pParameters
		0,                    // UINT NumStaticSamplers
		nullptr,              // const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers
		rootSigFlags,         // D3D12_ROOT_SIGNATURE_FLAGS Flags
	};

	// Create the shader root signature.
	m_shReconstructRootSig = CreateRootSignature(device, rootSigDesc);
	if(!m_shReconstructRootSig)
	{
		LOG_ERROR("Failed to create root signature for sh-reconstruct shader");
		return false;
	}

	const D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc =
	{
		m_shReconstructRootSig.Get(),   // ID3D12RootSignature *pRootSignature
		shaderBytecode,                 // D3D12_SHADER_BYTECODE CS
		0,                              // UINT NodeMask
		{},                             // D3D12_CACHED_PIPELINE_STATE CachedPSO
		D3D12_PIPELINE_STATE_FLAG_NONE, // D3D12_PIPELINE_STATE_FLAGS Flags
	};

	// Create the shader pipeline.
	m_shReconstructPipeline = CreatePipelineState(device, pipelineDesc);
	if(!m_shReconstructPipeline)
	{
		LOG_ERROR("Failed to create pipeline for sh-reconstruct shader");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::ReflectionProbe::_initCubeMaps(const Device::Ptr& device)
{
	constexpr DXGI_SAMPLE_DESC defaultSampleDesc =
	{
		1, // UINT Count
		0, // UINT Quality
	};

	constexpr D3D12_HEAP_PROPERTIES defaultHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,         // D3D12_HEAP_TYPE Type
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
		D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
		0,                               // UINT CreationNodeMask
		0,                               // UINT VisibleNodeMask
	};

	const D3D12_RESOURCE_DESC envResDesc =
	{
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,         // D3D12_RESOURCE_DIMENSION Dimension
		0,                                          // UINT64 Alignment
		uint64_t(m_envEdgeLength),                  // UINT64 Width
		m_envEdgeLength,                            // UINT Height
		DF_CUBE_FACE__COUNT,                        // UINT16 DepthOrArraySize
		uint16_t(m_envMipCount),                    // UINT16 MipLevels
		DXGI_FORMAT_R32G32B32A32_FLOAT,             // DXGI_FORMAT Format
		defaultSampleDesc,                          // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_UNKNOWN,               // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, // D3D12_RESOURCE_FLAGS Flags
	};

	const D3D12_RESOURCE_DESC irrResDesc =
	{
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,         // D3D12_RESOURCE_DIMENSION Dimension
		0,                                          // UINT64 Alignment
		uint64_t(m_irrEdgeLength),                  // UINT64 Width
		m_irrEdgeLength,                            // UINT Height
		DF_CUBE_FACE__COUNT,                        // UINT16 DepthOrArraySize
		1,                                          // UINT16 MipLevels
		DXGI_FORMAT_R32G32B32A32_FLOAT,             // DXGI_FORMAT Format
		defaultSampleDesc,                          // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_UNKNOWN,               // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, // D3D12_RESOURCE_FLAGS Flags
	};

	// Create the resource for the environment cube map.
	m_envResource = CreateCommittedResource(
		device,
		envResDesc,
		defaultHeapProps,
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	if(!m_envResource)
	{
		return false;
	}

	// Create the resource for the irradiance cube map.
	m_irrResource = CreateCommittedResource(
		device,
		irrResDesc,
		defaultHeapProps,
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	if(!m_irrResource)
	{
		return false;
	}

	// Create the SRV and UAVs for the environment map.
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = envResDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = m_envMipCount;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		m_envSrvDescriptor = m_alloc->Allocate();
		assert(m_envSrvDescriptor.index != Descriptor::Invalid.index);

		device->CreateShaderResourceView(m_envResource.Get(), &srvDesc, m_envSrvDescriptor.cpuHandle);

		for(uint32_t faceIndex = 0; faceIndex < DF_CUBE_FACE__COUNT; ++faceIndex)
		{
			for(uint32_t mipIndex = 0; mipIndex < m_envMipCount; ++mipIndex)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC faceUavDesc;
				faceUavDesc.Format = envResDesc.Format;
				faceUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				faceUavDesc.Texture2DArray.MipSlice = mipIndex;
				faceUavDesc.Texture2DArray.FirstArraySlice = faceIndex;
				faceUavDesc.Texture2DArray.ArraySize = 1;
				faceUavDesc.Texture2DArray.PlaneSlice = 0;

				const uint32_t uavDescIndex = (faceIndex * DF_CUBE_FACE__COUNT) + mipIndex;

				m_envFaceUavDescriptor[uavDescIndex] = m_alloc->Allocate();
				assert(m_envFaceUavDescriptor[uavDescIndex].index != Descriptor::Invalid.index);

				device->CreateUnorderedAccessView(m_envResource.Get(), nullptr, &faceUavDesc, m_envFaceUavDescriptor[uavDescIndex].cpuHandle);
			}
		}
	}

	// Create the SRV and UAVs for the irradiance map.
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = irrResDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		m_irrSrvDescriptor = m_alloc->Allocate();
		assert(m_irrSrvDescriptor.index != Descriptor::Invalid.index);

		device->CreateShaderResourceView(m_irrResource.Get(), &srvDesc, m_irrSrvDescriptor.cpuHandle);

		// Create a UAV per cube map face.
		D3D12_UNORDERED_ACCESS_VIEW_DESC faceUavDesc[DF_CUBE_FACE__COUNT];
		for(uint32_t i = 0; i < DF_CUBE_FACE__COUNT; ++i)
		{
			faceUavDesc[i].Format = irrResDesc.Format;
			faceUavDesc[i].ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			faceUavDesc[i].Texture2DArray.MipSlice = 0;
			faceUavDesc[i].Texture2DArray.FirstArraySlice = i;
			faceUavDesc[i].Texture2DArray.ArraySize = 1;
			faceUavDesc[i].Texture2DArray.PlaneSlice = 0;

			m_irrFaceUavDescriptor[i] = m_alloc->Allocate();
			assert(m_irrFaceUavDescriptor[i].index != Descriptor::Invalid.index);

			device->CreateUnorderedAccessView(m_irrResource.Get(), nullptr, &faceUavDesc[i], m_irrFaceUavDescriptor[i].cpuHandle);
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::D3D12::ReflectionProbe::_initUavResources(const Device::Ptr& device)
{
	constexpr DXGI_SAMPLE_DESC defaultSampleDesc =
	{
		1, // UINT Count
		0, // UINT Quality
	};

	constexpr D3D12_HEAP_PROPERTIES gpuHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,         // D3D12_HEAP_TYPE Type
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
		D3D12_MEMORY_POOL_UNKNOWN,       // D3D12_MEMORY_POOL MemoryPoolPreference
		0,                               // UINT CreationNodeMask
		0,                               // UINT VisibleNodeMask
	};

	const uint32_t coeffDataSize = sizeof(ShColorCoefficients);
	const uint32_t weightDataSize = sizeof(float32_t);
	const uint32_t arrayBaseLength = m_envEdgeLength * m_envEdgeLength;

	uint32_t uavMipLength = arrayBaseLength;
	uint32_t arrayLength = 0;

	// Using the same principle behind texture mipmaps to figure out how many elements total should be in the
	// UAV resource. The reasoning is that we need to sum all the elements in the array for SH reconstruction,
	// so to do that in a shader, we do multiple reduction passes with each "mip" level being processed in a
	// single pass.
	while(uavMipLength > 0)
	{
		arrayLength += uavMipLength;
		uavMipLength /= DF_SH_REDUCE_SEGMENT_SIZE;
	}

	m_uavArrayLength = arrayLength;
	m_uavArrayBaseLength = arrayBaseLength;

	const D3D12_RESOURCE_DESC shCoeffResDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER,            // D3D12_RESOURCE_DIMENSION Dimension
		0,                                          // UINT64 Alignment
		uint64_t(coeffDataSize * m_uavArrayLength), // UINT64 Width
		1,                                          // UINT Height
		1,                                          // UINT16 DepthOrArraySize
		1,                                          // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,                        // DXGI_FORMAT Format
		defaultSampleDesc,                          // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,             // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, // D3D12_RESOURCE_FLAGS Flags
	};

	const D3D12_RESOURCE_DESC shWeightResDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER,             // D3D12_RESOURCE_DIMENSION Dimension
		0,                                           // UINT64 Alignment
		uint64_t(weightDataSize * m_uavArrayLength), // UINT64 Width
		1,                                           // UINT Height
		1,                                           // UINT16 DepthOrArraySize
		1,                                           // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,                         // DXGI_FORMAT Format
		defaultSampleDesc,                           // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,              // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,  // D3D12_RESOURCE_FLAGS Flags
	};

	// Create the GPU-resident resource for the SH coefficients UAV.
	m_shCoeffResource = CreateCommittedResource(
		device,
		shCoeffResDesc,
		gpuHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	if(!m_shCoeffResource)
	{
		LOG_ERROR("Failed to create SH coefficients UAV resource");
		return false;
	}

	// Create the GPU-resident resource for the SH weights UAV.
	m_shWeightResource = CreateCommittedResource(
		device,
		shWeightResDesc,
		gpuHeapProps,
		D3D12_HEAP_FLAG_NONE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	if(!m_shWeightResource)
	{
		LOG_ERROR("Failed to create SH coefficients UAV resource");
		return false;
	}

	// Create the UAV for the SH coefficients resource.
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = m_uavArrayLength;
		srvDesc.Buffer.StructureByteStride = coeffDataSize;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = m_uavArrayLength;
		uavDesc.Buffer.StructureByteStride = uint32_t(coeffDataSize);
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		m_coeffSrvDescriptor = m_alloc->Allocate();
		assert(m_coeffSrvDescriptor.index != Descriptor::Invalid.index);

		m_coeffUavDescriptor = m_alloc->Allocate();
		assert(m_coeffUavDescriptor.index != Descriptor::Invalid.index);

		device->CreateShaderResourceView(m_shCoeffResource.Get(), &srvDesc, m_coeffSrvDescriptor.cpuHandle);
		device->CreateUnorderedAccessView(m_shCoeffResource.Get(), nullptr, &uavDesc, m_coeffUavDescriptor.cpuHandle);
	}

	// Create the UAV for the SH weights resource.
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = m_uavArrayLength;
		srvDesc.Buffer.StructureByteStride = weightDataSize;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = m_uavArrayLength;
		uavDesc.Buffer.StructureByteStride = weightDataSize;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		m_weightSrvDescriptor = m_alloc->Allocate();
		assert(m_weightSrvDescriptor.index != Descriptor::Invalid.index);

		m_weightUavDescriptor = m_alloc->Allocate();
		assert(m_weightUavDescriptor.index != Descriptor::Invalid.index);

		device->CreateShaderResourceView(m_shWeightResource.Get(), &srvDesc, m_weightSrvDescriptor.cpuHandle);
		device->CreateUnorderedAccessView(m_shWeightResource.Get(), nullptr, &uavDesc, m_weightUavDescriptor.cpuHandle);
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::ReflectionProbe::_generateIrradiance(const GraphicsCommandList::Ptr& cmdList)
{
	D3D12_RESOURCE_BARRIER uavBarrier[2];
	uavBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	uavBarrier[0].UAV.pResource = m_shCoeffResource.Get();

	uavBarrier[1] = uavBarrier[0];
	uavBarrier[1].UAV.pResource = m_shWeightResource.Get();

	// Project the environment cube map to a series of spherical harmonic coefficients and weights per sample.
	{
		const uint32_t groupCountX = m_envEdgeLength / DF_REFL_THREAD_COUNT_X;
		const uint32_t groupCountY = m_envEdgeLength / DF_REFL_THREAD_COUNT_Y;

		ShProjectRootConstant constData;
		constData.edgeLength = m_envEdgeLength;
		constData.invEdgeLength = 1.0f / float32_t(m_envEdgeLength);

		// Bind the shader pipeline.
		cmdList->SetComputeRootSignature(m_shProjectRootSig.Get());
		cmdList->SetPipelineState(m_shProjectPipeline.Get());

		// Set the shader inputs.
		cmdList->SetComputeRoot32BitConstants(0, sizeof(ShProjectRootConstant) / sizeof(uint32_t), &constData, 0);
		cmdList->SetComputeRootDescriptorTable(1, m_envSrvDescriptor.gpuHandle);
		cmdList->SetComputeRootDescriptorTable(2, m_coeffUavDescriptor.gpuHandle);
		cmdList->SetComputeRootDescriptorTable(3, m_weightUavDescriptor.gpuHandle);

		// Dispatch the SH projection job.
		cmdList->Dispatch(groupCountX, groupCountY, 1);

		// Place a barrier on the UAVs to wait for all the current work on them to complete before moving ahead.
		cmdList->ResourceBarrier(2, uavBarrier);
	}

	// Run the reduction passes on the SH coefficients and weights to calculate their total sums.
	{
		uint32_t mipLength = m_uavArrayBaseLength;

		ShReduceRootConstant constData;
		constData.headIndex = 0;
		constData.tailIndex = mipLength;

		// Bind the shader pipeline.
		cmdList->SetComputeRootSignature(m_shReduceRootSig.Get());
		cmdList->SetPipelineState(m_shReducePipeline.Get());

		// Set the shader inputs that won't change for all the reduction passes.
		cmdList->SetComputeRootDescriptorTable(1, m_coeffUavDescriptor.gpuHandle);
		cmdList->SetComputeRootDescriptorTable(2, m_weightUavDescriptor.gpuHandle);

		while(mipLength > 1)
		{
			// The group count is going to be based on the number of segments per array mip level.
			const uint32_t groupCount = (constData.tailIndex - constData.headIndex) / DF_SH_REDUCE_SEGMENT_SIZE / DF_REFL_SH_LINEAR_THREAD_COUNT;

			// Set the shader root constants which are updated per pass.
			cmdList->SetComputeRoot32BitConstants(0, sizeof(ShReduceRootConstant) / sizeof(uint32_t), &constData, 0);

			// Dispatch the job for the current reduction pass.
			cmdList->Dispatch((groupCount > 0) ? groupCount : 1, 1, 1);

			// Place a barrier on the UAVs between reduction passes since the work in
			// the previous pass needs to be completed before moving on to the next pass.
			cmdList->ResourceBarrier(2, uavBarrier);

			// Adjust the total array size that needs to be processed for the next group.
			mipLength /= DF_SH_REDUCE_SEGMENT_SIZE;

			// Point to the next group of coefficients in the UAVs.
			constData.headIndex = constData.tailIndex;
			constData.tailIndex = constData.headIndex + mipLength;
		}
	}

	// Normalize the SH coefficient total sum to finish the SH computation.
	{
		ShNormalizeRootConstant constData;
		constData.index = m_uavArrayLength - 1; // The final coefficients will always be in the last element of the array.

		// Bind the shader pipeline.
		cmdList->SetComputeRootSignature(m_shNormalizeRootSig.Get());
		cmdList->SetPipelineState(m_shNormalizePipeline.Get());

		// Set the shader inputs.
		cmdList->SetComputeRoot32BitConstants(0, sizeof(ShNormalizeRootConstant) / sizeof(uint32_t), &constData, 0);
		cmdList->SetComputeRootDescriptorTable(1, m_weightSrvDescriptor.gpuHandle);
		cmdList->SetComputeRootDescriptorTable(2, m_coeffUavDescriptor.gpuHandle);

		// Dispatch the SH normalize job.
		cmdList->Dispatch(1, 1, 1);

		// Place a barrier on just the coefficient UAV since it's the only one being updated.
		cmdList->ResourceBarrier(1, &uavBarrier[0]);
	}

	// Generate the irradiance map by SH reconstruction.
	{
		ShReconstructRootConstant constData;

		D3D12_RESOURCE_BARRIER irrMapBarrier[2];
		irrMapBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		irrMapBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		irrMapBarrier[0].Transition.pResource = m_irrResource.Get();
		irrMapBarrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		irrMapBarrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		irrMapBarrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		irrMapBarrier[1] = irrMapBarrier[0];
		irrMapBarrier[1].Transition.StateBefore = irrMapBarrier[0].Transition.StateAfter;
		irrMapBarrier[1].Transition.StateAfter = irrMapBarrier[0].Transition.StateBefore;

		const uint32_t groupCountX = m_irrEdgeLength / DF_REFL_THREAD_COUNT_X;
		const uint32_t groupCountY = m_irrEdgeLength / DF_REFL_THREAD_COUNT_Y;

		// Set the constant properties that will not change between faces.
		constData.coeffIndex = m_uavArrayLength - 1; // The final coefficients will always be in the last element of the array.
		constData.edgeLength = m_irrEdgeLength;
		constData.invEdgeLength = 1.0f / float32_t(m_irrEdgeLength);

		// Transition the irradiance map to a UAV so we can update the face texels.
		cmdList->ResourceBarrier(1, &irrMapBarrier[0]);

		// Bind the shader pipeline.
		cmdList->SetComputeRootSignature(m_shReconstructRootSig.Get());
		cmdList->SetPipelineState(m_shReconstructPipeline.Get());

		// Set the shader inputs that won't change for all the reduction passes.
		cmdList->SetComputeRootDescriptorTable(1, m_coeffSrvDescriptor.gpuHandle);

		// Generate each face of the irradiance cube map.
		for(uint32_t faceIndex = 0; faceIndex < DF_CUBE_FACE__COUNT; ++faceIndex)
		{
			// Set the face-specific constant properties.
			constData.faceIndex = faceIndex;

			// Set the per-face shader inputs.
			cmdList->SetComputeRoot32BitConstants(0, sizeof(ShReconstructRootConstant) / sizeof(uint32_t), &constData, 0);
			cmdList->SetComputeRootDescriptorTable(2, m_irrFaceUavDescriptor[faceIndex].gpuHandle);

			// Dispatch the SH reconstruction job for the current face texture.
			cmdList->Dispatch(groupCountX, groupCountY, 1);
		}

		// Transition the irradiance map back to an SRV so it can be used as a shader cube map input again.
		cmdList->ResourceBarrier(1, &irrMapBarrier[1]);
	}
}

//---------------------------------------------------------------------------------------------------------------------
