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
#include "Texture2D.hpp"

#include "Shaders/global-common.hlsli"

#include <memory>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class ReflectionProbe;

	enum class EnvMapQuality
	{
		Low,
		Mid,
		High,
	};
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::ReflectionProbe
{
public:

	typedef std::shared_ptr<ReflectionProbe> Ptr;

	ReflectionProbe();
	ReflectionProbe(const ReflectionProbe&) = delete;
	ReflectionProbe(ReflectionProbe&&) = delete;
	~ReflectionProbe();

	ReflectionProbe& operator =(const ReflectionProbe&) = delete;
	ReflectionProbe& operator =(ReflectionProbe&&) = delete;

	static Ptr Create(
		const Device::Ptr& device,
		const GraphicsCommandList::Ptr& cmdList,
		const DescriptorAllocator::Ptr& srvUavAlloc,
		EnvMapQuality mapQuality);

	bool LoadEnvironmentMap(
		const Device::Ptr& device,
		const GraphicsCommandList::Ptr& cmdList,
		const Texture2D::Ptr& envTexture);

	const DescriptorAllocator::Ptr& GetSrvAllocator() const;
	const Descriptor& GetEnvMapDescriptor() const;
	const Descriptor& GetIrrMapDescriptor() const;

	uint32_t GetEnvMapMipLevelCount() const;
	uint32_t GetEnvMapEdgeLength() const;
	uint32_t GetIrrMapEdgeLength() const;


private:

	bool _initEquiToCubePipeline(const Device::Ptr&);
	bool _initShProjectPipeline(const Device::Ptr&);
	bool _initShReducePipeline(const Device::Ptr&);
	bool _initShNormalizePipeline(const Device::Ptr&);
	bool _initShReconstructPipeline(const Device::Ptr&);
	bool _initCubeMaps(const Device::Ptr&);
	bool _initUavResources(const Device::Ptr&);
	void _generateIrradiance(const GraphicsCommandList::Ptr&);

	D3D12_STATIC_SAMPLER_DESC _getStaticSamplerDesc() const;

	DescriptorAllocator::Ptr m_alloc;

	RootSignature::Ptr m_equiToCubeRootSig;
	PipelineState::Ptr m_equiToCubePipeline;

	RootSignature::Ptr m_shProjectRootSig;
	PipelineState::Ptr m_shProjectPipeline;

	RootSignature::Ptr m_shReduceRootSig;
	PipelineState::Ptr m_shReducePipeline;

	RootSignature::Ptr m_shNormalizeRootSig;
	PipelineState::Ptr m_shNormalizePipeline;

	RootSignature::Ptr m_shReconstructRootSig;
	PipelineState::Ptr m_shReconstructPipeline;

	Resource::Ptr m_envResource;
	Resource::Ptr m_irrResource;
	Resource::Ptr m_shCoeffResource;
	Resource::Ptr m_shWeightResource;

	Descriptor m_envSrvDescriptor;
	Descriptor m_irrSrvDescriptor;

	Descriptor m_envFaceUavDescriptor[DF_CUBE_FACE__COUNT * D3D12_REQ_MIP_LEVELS];
	Descriptor m_irrFaceUavDescriptor[DF_CUBE_FACE__COUNT];

	Descriptor m_coeffSrvDescriptor;
	Descriptor m_coeffUavDescriptor;
	Descriptor m_weightSrvDescriptor;
	Descriptor m_weightUavDescriptor;

	uint32_t m_envMipCount;
	uint32_t m_envEdgeLength;
	uint32_t m_irrEdgeLength;
	uint32_t m_uavArrayLength;
	uint32_t m_uavArrayBaseLength;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API std::shared_ptr<DemoFramework::D3D12::ReflectionProbe>;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::ReflectionProbe::ReflectionProbe()
	: m_alloc()
	, m_equiToCubeRootSig()
	, m_equiToCubePipeline()
	, m_shProjectRootSig()
	, m_shProjectPipeline()
	, m_shReduceRootSig()
	, m_shReducePipeline()
	, m_shNormalizeRootSig()
	, m_shNormalizePipeline()
	, m_shReconstructRootSig()
	, m_shReconstructPipeline()
	, m_envResource()
	, m_irrResource()
	, m_shCoeffResource()
	, m_shWeightResource()
	, m_envSrvDescriptor(Descriptor::Invalid)
	, m_irrSrvDescriptor(Descriptor::Invalid)
	, m_irrFaceUavDescriptor()
	, m_coeffSrvDescriptor(Descriptor::Invalid)
	, m_coeffUavDescriptor(Descriptor::Invalid)
	, m_weightSrvDescriptor(Descriptor::Invalid)
	, m_weightUavDescriptor(Descriptor::Invalid)
	, m_envMipCount(0)
	, m_envEdgeLength(0)
	, m_irrEdgeLength(0)
	, m_uavArrayLength(0)
	, m_uavArrayBaseLength(0)
{
	for(size_t i = 0; i < DF_CUBE_FACE__COUNT; ++i)
	{
		for(size_t j = 0; j < D3D12_REQ_MIP_LEVELS; ++j)
		{
			m_envFaceUavDescriptor[(i * DF_CUBE_FACE__COUNT) + j] = Descriptor::Invalid;
		}

		m_irrFaceUavDescriptor[i] = Descriptor::Invalid;
	}
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::DescriptorAllocator::Ptr& DemoFramework::D3D12::ReflectionProbe::GetSrvAllocator() const
{
	return m_alloc;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::Descriptor& DemoFramework::D3D12::ReflectionProbe::GetEnvMapDescriptor() const
{
	return m_envSrvDescriptor;
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::Descriptor& DemoFramework::D3D12::ReflectionProbe::GetIrrMapDescriptor() const
{
	return m_irrSrvDescriptor;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::ReflectionProbe::GetEnvMapMipLevelCount() const
{
	return m_envMipCount;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::ReflectionProbe::GetEnvMapEdgeLength() const
{
	return m_envEdgeLength;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::D3D12::ReflectionProbe::GetIrrMapEdgeLength() const
{
	return m_irrEdgeLength;
}

//---------------------------------------------------------------------------------------------------------------------

inline D3D12_STATIC_SAMPLER_DESC DemoFramework::D3D12::ReflectionProbe::_getStaticSamplerDesc() const
{
	const D3D12_STATIC_SAMPLER_DESC output =
	{
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,             // D3D12_FILTER Filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,             // D3D12_TEXTURE_ADDRESS_MODE AddressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,             // D3D12_TEXTURE_ADDRESS_MODE AddressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,             // D3D12_TEXTURE_ADDRESS_MODE AddressW
		0.0f,                                        // FLOAT MipLODBias
		1,                                           // UINT MaxAnisotropy
		D3D12_COMPARISON_FUNC_ALWAYS,                // D3D12_COMPARISON_FUNC ComparisonFunc
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, // D3D12_STATIC_BORDER_COLOR BorderColor
		0.0f,                                        // FLOAT MinLOD
		float32_t(m_envMipCount - 1),                // FLOAT MaxLOD
		0,                                           // UINT ShaderRegister
		0,                                           // UINT RegisterSpace
		D3D12_SHADER_VISIBILITY_ALL,                 // D3D12_SHADER_VISIBILITY ShaderVisibility
	};

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
