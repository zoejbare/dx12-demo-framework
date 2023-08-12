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

#include "SampleApp.hpp"

#include <DemoFramework/Application/Log.hpp>
#include <DemoFramework/Application/Window.hpp>

#include <DemoFramework/Direct3D12/LowLevel/CommandAllocator.hpp>
#include <DemoFramework/Direct3D12/LowLevel/DescriptorHeap.hpp>
#include <DemoFramework/Direct3D12/LowLevel/GraphicsCommandList.hpp>
#include <DemoFramework/Direct3D12/LowLevel/PipelineState.hpp>
#include <DemoFramework/Direct3D12/LowLevel/Resource.hpp>
#include <DemoFramework/Direct3D12/LowLevel/RootSignature.hpp>

#include <DemoFramework/Direct3D12/Shader.hpp>
#include <DemoFramework/Direct3D12/Sync.hpp>
#include <DemoFramework/Direct3D12/WavefrontObj.hpp>

#include <DemoFramework/Utility/Math.hpp>

#include <imgui.h>

#include <math.h>

//---------------------------------------------------------------------------------------------------------------------

#define APP_NAME "Diffuse Environment Mapping Sample"
#define LOG_FILE "env-diffuse-sample.log"

#define APP_BACK_BUFFER_COUNT   2
#define APP_BACK_BUFFER_FORMAT  DXGI_FORMAT_R8G8B8A8_UNORM
#define APP_DEPTH_BUFFER_FORMAT DXGI_FORMAT_D32_FLOAT

//---------------------------------------------------------------------------------------------------------------------

constexpr DXGI_SAMPLE_DESC defaultSampleDesc =
{
	1, // UINT Count
	0, // UINT Quality
};

constexpr D3D12_RANGE disabledCpuReadRange =
{
	0, // SIZE_T Begin
	0, // SIZE_T End
};

constexpr D3D12_HEAP_PROPERTIES writeCombineHeapProps =
{
	D3D12_HEAP_TYPE_CUSTOM,                // D3D12_HEAP_TYPE Type
	D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE, // D3D12_CPU_PAGE_PROPERTY CPUPageProperty
	D3D12_MEMORY_POOL_L0,                  // D3D12_MEMORY_POOL MemoryPoolPreference
	0,                                     // UINT CreationNodeMask
	0,                                     // UINT VisibleNodeMask
};

//---------------------------------------------------------------------------------------------------------------------

struct EnvMapVertex
{
	struct Position { float32_t x, y; };

	Position pos;
};

struct EnvConstData
{
	DirectX::XMMATRIX viewInverse;
	DirectX::XMMATRIX projInverse;
};

struct ObjConstData
{
	DirectX::XMMATRIX worldViewProj;
	DirectX::XMMATRIX world;
};

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::Initialize(DemoFramework::Window* const window)
{
	using namespace DemoFramework;

	m_pWindow = window;

	HWND hWnd = m_pWindow->GetWindowHandle();

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();

	LOG_WRITE("Initializing base render resources ...");

	D3D12::RenderConfig renderConfig = D3D12::RenderConfig::Invalid;
	renderConfig.backBufferWidth = clientWidth;
	renderConfig.backBufferHeight = clientHeight;
	renderConfig.backBufferCount = APP_BACK_BUFFER_COUNT;
	renderConfig.cbvSrvUavDescCount = 500;
	renderConfig.rtvDescCount = APP_BACK_BUFFER_COUNT * 2;
	renderConfig.dsvDescCount = 2;
	renderConfig.backBufferFormat = APP_BACK_BUFFER_FORMAT;
	renderConfig.depthFormat = APP_DEPTH_BUFFER_FORMAT;

	// Initialize the common rendering resources.
	m_renderBase = D3D12::RenderBase::Create(hWnd, renderConfig);
	if(!m_renderBase)
	{
		return false;
	}

	LOG_WRITE("Initializing GUI resources ...");

	// Initialize the on-screen GUI.
	m_gui = D3D12::Gui::Create(m_renderBase->GetDevice(), APP_NAME, APP_BACK_BUFFER_COUNT, APP_BACK_BUFFER_FORMAT);
	if(!m_gui)
	{
		return false;
	}

	// Load all external data.
	if(!prv_loadExternalFiles())
	{
		return false;
	}

	// Initialize the environment mapping pipeline resources.
	if(!prv_createEnvPipeline())
	{
		return false;
	}

	// Initialize the scene object rendering pipeline resources.
	if(!prv_createObjPipeline())
	{
		return false;
	}

	// Set the size of the GUI display area.
	m_gui->SetDisplaySize(clientWidth, clientHeight);

	// Initialize the frame timer at the end so the initial timestamp is not
	// influenced by the time it takes to create the application resources.
	m_frameTimer.Initialize();

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::Update()
{
	using namespace DemoFramework;
	using namespace DirectX;

	if(m_resizeSwapChain)
	{
		const bool resizeSwapChainResult = m_renderBase->ResizeSwapChain();
		assert(resizeSwapChainResult == true); (void) resizeSwapChainResult;

		m_resizeSwapChain = false;
	}

	m_frameTimer.Update();

	static float32_t rotation = 0.0f;

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();

	XMVECTOR cameraPosition = XMVectorSet(1.6f, 0.5f, -1.6f, 0.0f);//XMVectorSet(cosf(rotation) * 2.0f, 0.5f, sinf(rotation) * 2.0f, 0.0f);
	XMVECTOR cameraFocus = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR cameraUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//m_worldMatrix = XMMatrixIdentity();
	m_worldMatrix = XMMatrixRotationAxis(cameraUp, rotation);
	m_viewMatrix = XMMatrixLookAtLH(cameraPosition, cameraFocus, cameraUp);
	m_projMatrix = XMMatrixPerspectiveFovLH(M_PI * 0.45f, float32_t(clientWidth) / float32_t(clientHeight), 0.1f, 1000.0f);
	m_viewInvMatrix = XMMatrixInverse(nullptr, m_viewMatrix);
	m_projInvMatrix = XMMatrixInverse(nullptr, m_projMatrix);
	m_wvpMatrix = XMMatrixMultiply(XMMatrixMultiply(m_worldMatrix, m_viewMatrix), m_projMatrix);

	rotation += M_TAU * float32_t(m_frameTimer.GetDeltaTime()) * 0.025f;
	if(rotation >= M_TAU)
	{
		rotation -= M_TAU;
	}

	m_gui->Update(
		m_frameTimer.GetDeltaTime(),
		[](ImGuiContext* const pGuiContext)
		{
			// Always set the ImGui context before calling any other ImGui functions.
			ImGui::SetCurrentContext(pGuiContext);

			// Do custom GUI drawing code here.
		}
	);

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Render()
{
	using namespace DemoFramework;
	using namespace DirectX;

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();

	m_renderBase->BeginFrame();
	m_renderBase->SetBackBufferAsRenderTarget();

	// The swap chain buffer index is updated during the call to BeginFrame(),
	// so we need to wait until *after* that to get the current command list.
	const D3D12::GraphicsCommandList::Ptr& cmdList = m_renderBase->GetDrawContext()->GetCmdList();

	// Update the environment map const buffer.
	{
		EnvConstData* pConstData = nullptr;

		// Map the buffer to CPU-accessible memory (write-only access) and copy the constant data to it.
		const HRESULT mapEnvConstBufferResult = m_envConstBuffer[m_constBufferIndex]->Map(
			0,
			&disabledCpuReadRange,
			reinterpret_cast<void**>(&pConstData));
		assert(SUCCEEDED(mapEnvConstBufferResult)); (void) mapEnvConstBufferResult;

		memcpy(&pConstData->viewInverse, &m_viewInvMatrix, sizeof(XMMATRIX));
		memcpy(&pConstData->projInverse, &m_projInvMatrix, sizeof(XMMATRIX));

		m_envConstBuffer[m_constBufferIndex]->Unmap(0, nullptr);
	}

	// Update the scene object const buffer.
	{
		ObjConstData* pConstData = nullptr;

		// Map the buffer to CPU-accessible memory (write-only access) and copy the constant data to it.
		const HRESULT mapObjConstBufferResult = m_objConstBuffer[m_constBufferIndex]->Map(
			0,
			&disabledCpuReadRange,
			reinterpret_cast<void**>(&pConstData));
		assert(SUCCEEDED(mapObjConstBufferResult)); (void) mapObjConstBufferResult;

		memcpy(&pConstData->worldViewProj, &m_wvpMatrix, sizeof(XMMATRIX));
		memcpy(&pConstData->world, &m_worldMatrix, sizeof(XMMATRIX));

		m_objConstBuffer[m_constBufferIndex]->Unmap(0, nullptr);
	}

	ID3D12DescriptorHeap* const pDescHeaps[] =
	{
		m_renderBase->GetCbvSrvUavAllocator()->GetHeap().Get(),
	};

	// Set the descriptor heap used for all the draw calls.
	cmdList->SetDescriptorHeaps(_countof(pDescHeaps), pDescHeaps);

	// Set the screen viewport.
	const D3D12_VIEWPORT viewport =
	{
		0.0f,                    // FLOAT TopLeftX
		0.0f,                    // FLOAT TopLeftY
		float32_t(clientWidth),  // FLOAT Width
		float32_t(clientHeight), // FLOAT Height
		0.0f,                    // FLOAT MinDepth
		1.0f,                    // FLOAT MaxDepth
	};
	cmdList->RSSetViewports(1, &viewport);

	// Set the scissor region of the viewport.
	const D3D12_RECT scissorRect =
	{
		0,                  // LONG    left
		0,                  // LONG    top
		LONG(clientWidth),  // LONG    right
		LONG(clientHeight), // LONG    bottom
	};
	cmdList->RSSetScissorRects(1, &scissorRect);

	// Draw the static meshes.
	{
		// Bind the graphics pipeline.
		cmdList->SetGraphicsRootSignature(m_objRootSig.Get());
		cmdList->SetPipelineState(m_objPipeline.Get());

		// Bind the shader resources.
		cmdList->SetGraphicsRootDescriptorTable(0, m_objCbvDesc[m_constBufferIndex].gpuHandle);
		cmdList->SetGraphicsRootDescriptorTable(1, m_reflectionProbe->GetIrrMapDescriptor().gpuHandle);

		// Setup the input assembler.
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw the loaded object.
		m_object->Draw(cmdList);
	}

	// Draw the environment map.
	{
		const D3D12_VERTEX_BUFFER_VIEW vertexBufferView =
		{
			m_envVertexBuffer->GetGPUVirtualAddress(), // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
			sizeof(EnvMapVertex) * 4,                  // UINT SizeInBytes
			sizeof(EnvMapVertex),                      // UINT StrideInBytes
		};

		// Bind the graphics pipeline.
		cmdList->SetGraphicsRootSignature(m_envRootSig.Get());
		cmdList->SetPipelineState(m_envPipeline.Get());

		// Bind the shader resources.
		cmdList->SetGraphicsRootDescriptorTable(0, m_envCbvDesc[m_constBufferIndex].gpuHandle);
		cmdList->SetGraphicsRootDescriptorTable(1, m_reflectionProbe->GetEnvMapDescriptor().gpuHandle);

		// Setup the input assembler.
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);

		// Issue the draw call.
		cmdList->DrawInstanced(4, 1, 0, 0);
	}

	// Draw the GUI.
	m_gui->Render(cmdList);

	m_renderBase->EndFrame(true);

	// Move to the next set of constant buffers.
	m_constBufferIndex = (m_constBufferIndex + 1) % APP_CONST_BUFFER_COUNT;
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Shutdown()
{
	// Do explicit shutdown tasks here.
	m_object.reset();
	m_reflectionProbe.reset();
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowResized(DemoFramework::Window* const pWindow, const uint32_t previousWidth, const uint32_t previousHeight)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(previousWidth);
	DF_UNUSED(previousHeight);

	m_gui->SetDisplaySize(pWindow->GetClientWidth(), pWindow->GetClientHeight());

	m_resizeSwapChain = true;
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseMove(DemoFramework::Window* const pWindow, const int32_t previousX, const int32_t previousY)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(previousX);
	DF_UNUSED(previousY);

	m_gui->SetMousePosition(pWindow->GetMouseX(), pWindow->GetMouseY());
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseWheel(DemoFramework::Window* const pWindow, const float32_t wheelDelta)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(wheelDelta);

	m_gui->SetMouseWheelDelta(wheelDelta);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseButtonPressed(DemoFramework::Window* const pWindow, const DemoFramework::MouseButton button)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(button);

	using namespace DemoFramework;

	uint32_t buttonIndex = UINT_MAX;
	switch(button)
	{
		case MouseButton::Left:   buttonIndex = 0; break;
		case MouseButton::Right:  buttonIndex = 1; break;
		case MouseButton::Middle: buttonIndex = 2; break;
		case MouseButton::X1:     buttonIndex = 3; break;
		case MouseButton::X2:     buttonIndex = 4; break;

		default:
			assert(false);
			break;
	}

	m_gui->SetMouseButtonState(buttonIndex, true);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseButtonReleased(DemoFramework::Window* const pWindow, const DemoFramework::MouseButton button)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(button);

	using namespace DemoFramework;

	uint32_t buttonIndex = UINT_MAX;
	switch(button)
	{
		case MouseButton::Left:   buttonIndex = 0; break;
		case MouseButton::Right:  buttonIndex = 1; break;
		case MouseButton::Middle: buttonIndex = 2; break;
		case MouseButton::X1:     buttonIndex = 3; break;
		case MouseButton::X2:     buttonIndex = 4; break;

		default:
			assert(false);
			break;
	}

	m_gui->SetMouseButtonState(buttonIndex, false);
}

//---------------------------------------------------------------------------------------------------------------------

const char* SampleApp::GetAppName() const
{
	return APP_NAME;
}

//---------------------------------------------------------------------------------------------------------------------

const char* SampleApp::GetLogFilename() const
{
	return LOG_FILE;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_loadExternalFiles()
{
	using namespace DemoFramework;

	const D3D12::Device::Ptr& device = m_renderBase->GetDevice();
	const D3D12::DescriptorAllocator::Ptr& descAlloc = m_renderBase->GetCbvSrvUavAllocator();
	const D3D12::CommandQueue::Ptr& cmdQueue = m_renderBase->GetCmdQueue();

	D3D12::GraphicsCommandContext::Ptr cmdCtx = D3D12::GraphicsCommandContext::Create(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	cmdCtx->Reset();

	const D3D12::GraphicsCommandList::Ptr& cmdList = cmdCtx->GetCmdList();

	// Create a command synchronization primitive so we can wait for all
	// operations to complete for the meshes at the end of initialization.
	D3D12::Sync::Ptr cmdSync = D3D12::Sync::Create(device, D3D12_FENCE_FLAG_NONE);
	if(!cmdSync)
	{
		return false;
	}

	const char* const modelFilePath = "models/common/head.obj";

	// Load the object that will be displayed in the center of the environment.
	m_object = D3D12::WavefrontObj::Load(device, cmdList, "Object", modelFilePath);
	if(!m_object)
	{
		LOG_ERROR("Failed to load OBJ file: \"%s\"", modelFilePath);
		return false;
	}

	const char* const textureFilePath = "textures/common/pine_attic_2k.hdr";

	// Load the image file that will be used for the environment map.
	D3D12::Texture2D::Ptr envTexture = D3D12::Texture2D::Load(
		device,
		cmdList,
		descAlloc,
		D3D12::Texture2D::DataType::Float,
		D3D12::Texture2D::Channel::RGBA,
		textureFilePath,
		1);
	if(!envTexture)
	{
		LOG_ERROR("Failed to load environment map texture: \"%s\"", textureFilePath);
		return false;
	}

	// Create the environment reflection probe.
	m_reflectionProbe = D3D12::ReflectionProbe::Create(device, cmdList, descAlloc, D3D12::EnvMapQuality::Mid);
	if(!m_reflectionProbe)
	{
		LOG_ERROR("Failed to create reflection probe");
		return false;
	}

	// Attempt to generate the environment map resources from the environment texture.
	if(!m_reflectionProbe->LoadEnvironmentMap(device, cmdList, envTexture))
	{
		LOG_ERROR("Failed to load environment map into reflection probe");
		return false;
	}

	// Stop recording commands in the command list and begin executing it.
	cmdCtx->Submit(cmdQueue);

	// Wait for the command list to finish executing.
	cmdSync->Signal(cmdQueue);
	cmdSync->Wait();

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_createEnvPipeline()
{
	using namespace DemoFramework;

	LOG_WRITE("Creating environment map pipeline ...");

	const D3D12::Device::Ptr& device = m_renderBase->GetDevice();
	const D3D12::DescriptorAllocator::Ptr& descAlloc = m_renderBase->GetCbvSrvUavAllocator();

	const char* const vertexShaderFilePath = "shaders/env-diffuse/envmap.vs.sbin";
	const char* const pixelShaderFilePath = "shaders/env-diffuse/envmap.ps.sbin";

	D3D12::Blob::Ptr vertexShader = D3D12::LoadShaderFromFile(vertexShaderFilePath);
	if(!vertexShader)
	{
		LOG_ERROR("Failed to load shader: \"%s\"", vertexShaderFilePath);
		return false;
	}

	D3D12::Blob::Ptr pixelShader = D3D12::LoadShaderFromFile(pixelShaderFilePath);
	if(!pixelShader)
	{
		LOG_ERROR("Failed to load shader: \"%s\"", pixelShaderFilePath);
		return false;
	}

	constexpr D3D12_DESCRIPTOR_RANGE cbvDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	constexpr D3D12_DESCRIPTOR_RANGE srvDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	// CBV table
	D3D12_ROOT_PARAMETER cbvTableParam;
	cbvTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	cbvTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	cbvTableParam.DescriptorTable.NumDescriptorRanges = 1;
	cbvTableParam.DescriptorTable.pDescriptorRanges = &cbvDescRange;

	// SRV table
	D3D12_ROOT_PARAMETER srvTableParam;
	srvTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	srvTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	srvTableParam.DescriptorTable.NumDescriptorRanges = 1;
	srvTableParam.DescriptorTable.pDescriptorRanges = &srvDescRange;

	const D3D12_ROOT_PARAMETER rootParams[] =
	{
		cbvTableParam,
		srvTableParam,
	};

	const D3D12_STATIC_SAMPLER_DESC staticSamplerDesc =
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
		1.0f,                                        // FLOAT MaxLOD
		0,                                           // UINT ShaderRegister
		0,                                           // UINT RegisterSpace
		D3D12_SHADER_VISIBILITY_PIXEL,               // D3D12_SHADER_VISIBILITY ShaderVisibility
	};

	constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

	const D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
	{
		_countof(rootParams), // UINT NumParameters
		rootParams,           // const D3D12_ROOT_PARAMETER *pParameters
		1,                    // UINT NumStaticSamplers
		&staticSamplerDesc,   // const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers
		rootSigFlags,         // D3D12_ROOT_SIGNATURE_FLAGS Flags
	};

	// Create the pipeline root signature.
	m_envRootSig = D3D12::CreateRootSignature(device, rootSigDesc);
	if(!m_envRootSig)
	{
		LOG_ERROR("Failed to create environment map root signature");
		return false;
	}

	const D3D12_SHADER_BYTECODE vertexShaderBytecode =
	{
		vertexShader->GetBufferPointer(), // const void *pShaderBytecode
		vertexShader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	const D3D12_SHADER_BYTECODE pixelShaderBytecode =
	{
		pixelShader->GetBufferPointer(), // const void *pShaderBytecode
		pixelShader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	constexpr D3D12_RENDER_TARGET_BLEND_DESC targetBlendState =
	{
		false,                        // BOOL BlendEnable
		false,                        // BOOL LogicOpEnable
		D3D12_BLEND_SRC_ALPHA,        // D3D12_BLEND SrcBlend
		D3D12_BLEND_INV_SRC_ALPHA,    // D3D12_BLEND DestBlend
		D3D12_BLEND_OP_ADD,           // D3D12_BLEND_OP BlendOp
		D3D12_BLEND_ONE,              // D3D12_BLEND SrcBlendAlpha
		D3D12_BLEND_ONE,              // D3D12_BLEND DestBlendAlpha
		D3D12_BLEND_OP_ADD,           // D3D12_BLEND_OP BlendOpAlpha
		D3D12_LOGIC_OP_NOOP,          // D3D12_LOGIC_OP LogicOp
		D3D12_COLOR_WRITE_ENABLE_ALL, // UINT8 RenderTargetWriteMask
	};

	constexpr D3D12_BLEND_DESC blendState =
	{
		false,                // BOOL AlphaToCoverageEnable
		false,                // BOOL IndependentBlendEnable
		{ targetBlendState }, // D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[ 8 ]
	};

	constexpr D3D12_RASTERIZER_DESC rasterizerState =
	{
		D3D12_FILL_MODE_SOLID,                     // D3D12_FILL_MODE FillMode
		D3D12_CULL_MODE_BACK,                      // D3D12_CULL_MODE CullMode
		false,                                     // BOOL FrontCounterClockwise
		D3D12_DEFAULT_DEPTH_BIAS,                  // INT DepthBias
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // FLOAT DepthBiasClamp
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // FLOAT SlopeScaledDepthBias
		true,                                      // BOOL DepthClipEnable
		false,                                     // BOOL MultisampleEnable
		false,                                     // BOOL AntialiasedLineEnable
		0,                                         // UINT ForcedSampleCount
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster
	};

	// IMPORTANT:
	//  Note that depth testing is enabled here, but it won't write anything to the depth buffer.
	//  This is so the environment map can be drawn *after* the scene geometry at the back of the
	//  viewport (enforced by the explicit Z position set in the shader) and draw only to the
	//  background fragments without overwriting anything important within the scene.
	constexpr D3D12_DEPTH_STENCIL_DESC depthStencilState =
	{
		true,                             // BOOL DepthEnable
		D3D12_DEPTH_WRITE_MASK_ZERO,      // D3D12_DEPTH_WRITE_MASK DepthWriteMask
		D3D12_COMPARISON_FUNC_LESS_EQUAL, // D3D12_COMPARISON_FUNC DepthFunc
		false,                            // BOOL StencilEnable
		0,                                // UINT8 StencilReadMask
		0,                                // UINT8 StencilWriteMask
		{},                               // D3D12_DEPTH_STENCILOP_DESC FrontFace
		{},                               // D3D12_DEPTH_STENCILOP_DESC BackFace
	};

	constexpr D3D12_INPUT_ELEMENT_DESC vertexPositionElement =
	{
		"POSITION",                                 // LPCSTR SemanticName
		0,                                          // UINT SemanticIndex
		DXGI_FORMAT_R32G32_FLOAT,                   // DXGI_FORMAT Format
		0,                                          // UINT InputSlot
		offsetof(EnvMapVertex, pos),                // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                          // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC inputElements[] =
	{
		vertexPositionElement,
	};

	const D3D12_INPUT_LAYOUT_DESC inputLayout =
	{
		inputElements,           // const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs
		_countof(inputElements), // UINT NumElements
	};

	const D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc =
	{
		m_envRootSig.Get(),                          // ID3D12RootSignature *pRootSignature
		vertexShaderBytecode,                        // D3D12_SHADER_BYTECODE VS
		pixelShaderBytecode,                         // D3D12_SHADER_BYTECODE PS
		{},                                          // D3D12_SHADER_BYTECODE DS
		{},                                          // D3D12_SHADER_BYTECODE HS
		{},                                          // D3D12_SHADER_BYTECODE GS
		{},                                          // D3D12_STREAM_OUTPUT_DESC StreamOutput
		blendState,                                  // D3D12_BLEND_DESC BlendState
		UINT_MAX,                                    // UINT SampleMask
		rasterizerState,                             // D3D12_RASTERIZER_DESC RasterizerState
		depthStencilState,                           // D3D12_DEPTH_STENCIL_DESC DepthStencilState
		inputLayout,                                 // D3D12_INPUT_LAYOUT_DESC InputLayout
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED, // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,      // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType
		1,                                           // UINT NumRenderTargets
		{ APP_BACK_BUFFER_FORMAT },                  // DXGI_FORMAT RTVFormats[ 8 ]
		APP_DEPTH_BUFFER_FORMAT,                     // DXGI_FORMAT DSVFormat
		defaultSampleDesc,                           // DXGI_SAMPLE_DESC SampleDesc
		0,                                           // UINT NodeMask
		{},                                          // D3D12_CACHED_PIPELINE_STATE CachedPSO
		D3D12_PIPELINE_STATE_FLAG_NONE,              // D3D12_PIPELINE_STATE_FLAGS Flags
	};

	// Create the graphics pipeline state.
	m_envPipeline = D3D12::CreatePipelineState(device, pipelineDesc);
	if(!m_envPipeline)
	{
		LOG_ERROR("Failed to create environment map graphics pipeline");
		return false;
	}

	const D3D12_RESOURCE_DESC vertexBufferDesc =
	{
		D3D12_RESOURCE_DIMENSION_BUFFER, // D3D12_RESOURCE_DIMENSION Dimension
		0,                               // UINT64 Alignment
		sizeof(EnvMapVertex) * 4,        // UINT64 Width
		1,                               // UINT Height
		1,                               // UINT16 DepthOrArraySize
		1,                               // UINT16 MipLevels
		DXGI_FORMAT_UNKNOWN,             // DXGI_FORMAT Format
		defaultSampleDesc,               // DXGI_SAMPLE_DESC SampleDesc
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,  // D3D12_TEXTURE_LAYOUT Layout
		D3D12_RESOURCE_FLAG_NONE,        // D3D12_RESOURCE_FLAGS Flags
	};

	// Create the vertex buffer resource.
	{
		m_envVertexBuffer = D3D12::CreateCommittedResource(
			device,
			vertexBufferDesc,
			writeCombineHeapProps,
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		if(!m_envVertexBuffer)
		{
			LOG_ERROR("Failed to create environment map vertex buffer");
			return false;
		}

		volatile EnvMapVertex* pVertexData = nullptr;

		// Map the vertex buffer to CPU-accessible memory so we can write the vertex data into it.
		const HRESULT mapEnvVertexBufferResult = m_envVertexBuffer->Map(
			0,
			&disabledCpuReadRange,
			reinterpret_cast<void**>(const_cast<EnvMapVertex**>(&pVertexData)));
		if(FAILED(mapEnvVertexBufferResult))
		{
			LOG_ERROR("Failed to map environment map vertex buffer for writing; result=0x%08" PRIX32, mapEnvVertexBufferResult);
			return false;
		}

		// Upper left
		pVertexData[0].pos.x = -1.0f;
		pVertexData[0].pos.y = 1.0f;

		// Upper right
		pVertexData[1].pos.x = 1.0f;
		pVertexData[1].pos.y = 1.0f;

		// Lower left
		pVertexData[2].pos.x = -1.0f;
		pVertexData[2].pos.y = -1.0f;

		// Lower right
		pVertexData[3].pos.x = 1.0f;
		pVertexData[3].pos.y = -1.0f;

		m_envVertexBuffer->Unmap(0, nullptr);
	}

	const uint64_t alignedCbSize = Utility::Math::GetAlignedSize<uint64_t>(sizeof(EnvConstData), 256);

	// Create the constant buffers and their descriptors.
	for(size_t i = 0; i < APP_CONST_BUFFER_COUNT; ++i)
	{
		const D3D12_RESOURCE_DESC constBufferDesc =
		{
			D3D12_RESOURCE_DIMENSION_BUFFER, // D3D12_RESOURCE_DIMENSION Dimension
			0,                               // UINT64 Alignment
			alignedCbSize,                   // UINT64 Width
			1,                               // UINT Height
			1,                               // UINT16 DepthOrArraySize
			1,                               // UINT16 MipLevels
			DXGI_FORMAT_UNKNOWN,             // DXGI_FORMAT Format
			defaultSampleDesc,               // DXGI_SAMPLE_DESC SampleDesc
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR,  // D3D12_TEXTURE_LAYOUT Layout
			D3D12_RESOURCE_FLAG_NONE,        // D3D12_RESOURCE_FLAGS Flags
		};

		m_envConstBuffer[i] = D3D12::CreateCommittedResource(
			device,
			constBufferDesc,
			writeCombineHeapProps,
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		if(!m_envConstBuffer[i])
		{
			LOG_ERROR("Failed to create environment map constant buffer [%zu]", i);
			return false;
		}

		m_envCbvDesc[i] = descAlloc->Allocate();
		if(m_envCbvDesc[i].index == D3D12::Descriptor::Invalid.index)
		{
			LOG_ERROR("Failed to allocate descriptor for environment map constant buffer [%zu]", i);
			return false;
		}

		const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc =
		{
			m_envConstBuffer[i]->GetGPUVirtualAddress(), // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
			uint32_t(alignedCbSize),                     // UINT SizeInBytes
		};

		// Create the constant buffer view.
		device->CreateConstantBufferView(&cbvDesc, m_envCbvDesc[i].cpuHandle);
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::prv_createObjPipeline()
{
	using namespace DemoFramework;

	LOG_WRITE("Creating scene object pipeline ...");

	const D3D12::Device::Ptr& device = m_renderBase->GetDevice();
	const D3D12::DescriptorAllocator::Ptr& descAlloc = m_renderBase->GetCbvSrvUavAllocator();

	const char* const vertexShaderFilePath = "shaders/env-diffuse/obj.vs.sbin";
	const char* const pixelShaderFilePath = "shaders/env-diffuse/obj.ps.sbin";

	D3D12::Blob::Ptr vertexShader = D3D12::LoadShaderFromFile(vertexShaderFilePath);
	if(!vertexShader)
	{
		LOG_ERROR("Failed to load shader: \"%s\"", vertexShaderFilePath);
		return false;
	}

	D3D12::Blob::Ptr pixelShader = D3D12::LoadShaderFromFile(pixelShaderFilePath);
	if(!pixelShader)
	{
		LOG_ERROR("Failed to load shader: \"%s\"", pixelShaderFilePath);
		return false;
	}

	constexpr D3D12_DESCRIPTOR_RANGE cbvDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	constexpr D3D12_DESCRIPTOR_RANGE srvDescRange =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // D3D12_DESCRIPTOR_RANGE_TYPE RangeType
		1,                               // UINT NumDescriptors
		0,                               // UINT BaseShaderRegister
		0,                               // UINT RegisterSpace
		0,                               // UINT OffsetInDescriptorsFromTableStart
	};

	// CBV table
	D3D12_ROOT_PARAMETER cbvTableParam;
	cbvTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	cbvTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	cbvTableParam.DescriptorTable.NumDescriptorRanges = 1;
	cbvTableParam.DescriptorTable.pDescriptorRanges = &cbvDescRange;

	// SRV table
	D3D12_ROOT_PARAMETER srvTableParam;
	srvTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	srvTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	srvTableParam.DescriptorTable.NumDescriptorRanges = 1;
	srvTableParam.DescriptorTable.pDescriptorRanges = &srvDescRange;

	const D3D12_ROOT_PARAMETER rootParams[] =
	{
		cbvTableParam,
		srvTableParam,
	};

	const D3D12_STATIC_SAMPLER_DESC staticSamplerDesc =
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
		1.0f,                                        // FLOAT MaxLOD
		0,                                           // UINT ShaderRegister
		0,                                           // UINT RegisterSpace
		D3D12_SHADER_VISIBILITY_PIXEL,               // D3D12_SHADER_VISIBILITY ShaderVisibility
	};

	constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

	const D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
	{
		_countof(rootParams), // UINT NumParameters
		rootParams,           // const D3D12_ROOT_PARAMETER *pParameters
		1,                    // UINT NumStaticSamplers
		&staticSamplerDesc,   // const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers
		rootSigFlags,         // D3D12_ROOT_SIGNATURE_FLAGS Flags
	};

	// Create the pipeline root signature.
	m_objRootSig = D3D12::CreateRootSignature(device, rootSigDesc);
	if(!m_objRootSig)
	{
		LOG_ERROR("Failed to create scene object root signature");
		return false;
	}

	const D3D12_SHADER_BYTECODE vertexShaderBytecode =
	{
		vertexShader->GetBufferPointer(), // const void *pShaderBytecode
		vertexShader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	const D3D12_SHADER_BYTECODE pixelShaderBytecode =
	{
		pixelShader->GetBufferPointer(), // const void *pShaderBytecode
		pixelShader->GetBufferSize(),    // SIZE_T BytecodeLength
	};

	constexpr D3D12_RENDER_TARGET_BLEND_DESC targetBlendState =
	{
		false,                        // BOOL BlendEnable
		false,                        // BOOL LogicOpEnable
		D3D12_BLEND_SRC_ALPHA,        // D3D12_BLEND SrcBlend
		D3D12_BLEND_INV_SRC_ALPHA,    // D3D12_BLEND DestBlend
		D3D12_BLEND_OP_ADD,           // D3D12_BLEND_OP BlendOp
		D3D12_BLEND_ONE,              // D3D12_BLEND SrcBlendAlpha
		D3D12_BLEND_ONE,              // D3D12_BLEND DestBlendAlpha
		D3D12_BLEND_OP_ADD,           // D3D12_BLEND_OP BlendOpAlpha
		D3D12_LOGIC_OP_NOOP,          // D3D12_LOGIC_OP LogicOp
		D3D12_COLOR_WRITE_ENABLE_ALL, // UINT8 RenderTargetWriteMask
	};

	constexpr D3D12_BLEND_DESC blendState =
	{
		false,                // BOOL AlphaToCoverageEnable
		false,                // BOOL IndependentBlendEnable
		{ targetBlendState }, // D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[ 8 ]
	};

	constexpr D3D12_RASTERIZER_DESC rasterizerState =
	{
		D3D12_FILL_MODE_SOLID,                     // D3D12_FILL_MODE FillMode
		D3D12_CULL_MODE_BACK,                      // D3D12_CULL_MODE CullMode
		false,                                     // BOOL FrontCounterClockwise
		D3D12_DEFAULT_DEPTH_BIAS,                  // INT DepthBias
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // FLOAT DepthBiasClamp
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // FLOAT SlopeScaledDepthBias
		true,                                      // BOOL DepthClipEnable
		false,                                     // BOOL MultisampleEnable
		false,                                     // BOOL AntialiasedLineEnable
		0,                                         // UINT ForcedSampleCount
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster
	};

	constexpr D3D12_DEPTH_STENCIL_DESC depthStencilState =
	{
		true,                             // BOOL DepthEnable
		D3D12_DEPTH_WRITE_MASK_ALL,       // D3D12_DEPTH_WRITE_MASK DepthWriteMask
		D3D12_COMPARISON_FUNC_LESS_EQUAL, // D3D12_COMPARISON_FUNC DepthFunc
		false,                            // BOOL StencilEnable
		0,                                // UINT8 StencilReadMask
		0,                                // UINT8 StencilWriteMask
		{},                               // D3D12_DEPTH_STENCILOP_DESC FrontFace
		{},                               // D3D12_DEPTH_STENCILOP_DESC BackFace
	};

	constexpr D3D12_INPUT_ELEMENT_DESC vertexPositionElement =
	{
		"POSITION",                                         // LPCSTR SemanticName
		0,                                                  // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                        // DXGI_FORMAT Format
		0,                                                  // UINT InputSlot
		offsetof(D3D12::StaticMesh::Geometry::Vertex, pos), // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,         // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                                  // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC vertexTexCoordElement =
	{
		"TEXCOORD",                                         // LPCSTR SemanticName
		0,                                                  // UINT SemanticIndex
		DXGI_FORMAT_R32G32_FLOAT,                           // DXGI_FORMAT Format
		0,                                                  // UINT InputSlot
		offsetof(D3D12::StaticMesh::Geometry::Vertex, tex), // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,         // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                                  // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC vertexNormalElement =
	{
		"NORMAL",                                           // LPCSTR SemanticName
		0,                                                  // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                        // DXGI_FORMAT Format
		0,                                                  // UINT InputSlot
		offsetof(D3D12::StaticMesh::Geometry::Vertex, norm), // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,         // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                                  // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC vertexTangentElement =
	{
		"TANGENT",                                          // LPCSTR SemanticName
		0,                                                  // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                        // DXGI_FORMAT Format
		0,                                                  // UINT InputSlot
		offsetof(D3D12::StaticMesh::Geometry::Vertex, tan), // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,         // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                                  // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC vertexBinormalElement =
	{
		"BINORMAL",                                         // LPCSTR SemanticName
		0,                                                  // UINT SemanticIndex
		DXGI_FORMAT_R32G32B32_FLOAT,                        // DXGI_FORMAT Format
		0,                                                  // UINT InputSlot
		offsetof(D3D12::StaticMesh::Geometry::Vertex, bin), // UINT AlignedByteOffset
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,         // D3D12_INPUT_CLASSIFICATION InputSlotClass
		0,                                                  // UINT InstanceDataStepRate
	};

	constexpr D3D12_INPUT_ELEMENT_DESC inputElements[] =
	{
		vertexPositionElement,
		vertexTexCoordElement,
		vertexNormalElement,
		vertexTangentElement,
		vertexBinormalElement,
	};

	const D3D12_INPUT_LAYOUT_DESC inputLayout =
	{
		inputElements,           // const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs
		_countof(inputElements), // UINT NumElements
	};

	const D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc =
	{
		m_objRootSig.Get(),                          // ID3D12RootSignature *pRootSignature
		vertexShaderBytecode,                        // D3D12_SHADER_BYTECODE VS
		pixelShaderBytecode,                         // D3D12_SHADER_BYTECODE PS
		{},                                          // D3D12_SHADER_BYTECODE DS
		{},                                          // D3D12_SHADER_BYTECODE HS
		{},                                          // D3D12_SHADER_BYTECODE GS
		{},                                          // D3D12_STREAM_OUTPUT_DESC StreamOutput
		blendState,                                  // D3D12_BLEND_DESC BlendState
		UINT_MAX,                                    // UINT SampleMask
		rasterizerState,                             // D3D12_RASTERIZER_DESC RasterizerState
		depthStencilState,                           // D3D12_DEPTH_STENCIL_DESC DepthStencilState
		inputLayout,                                 // D3D12_INPUT_LAYOUT_DESC InputLayout
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED, // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,      // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType
		1,                                           // UINT NumRenderTargets
		{ APP_BACK_BUFFER_FORMAT },                  // DXGI_FORMAT RTVFormats[ 8 ]
		APP_DEPTH_BUFFER_FORMAT,                     // DXGI_FORMAT DSVFormat
		defaultSampleDesc,                           // DXGI_SAMPLE_DESC SampleDesc
		0,                                           // UINT NodeMask
		{},                                          // D3D12_CACHED_PIPELINE_STATE CachedPSO
		D3D12_PIPELINE_STATE_FLAG_NONE,              // D3D12_PIPELINE_STATE_FLAGS Flags
	};

	// Create the graphics pipeline state.
	m_objPipeline = D3D12::CreatePipelineState(device, pipelineDesc);
	if(!m_objPipeline)
	{
		LOG_ERROR("Failed to create scene object graphics pipeline");
		return false;
	}

	const uint64_t alignedCbSize = Utility::Math::GetAlignedSize<uint64_t>(sizeof(ObjConstData), 256);

	// Create the constant buffers and their descriptors.
	for(size_t i = 0; i < APP_CONST_BUFFER_COUNT; ++i)
	{
		const D3D12_RESOURCE_DESC constBufferDesc =
		{
			D3D12_RESOURCE_DIMENSION_BUFFER, // D3D12_RESOURCE_DIMENSION Dimension
			0,                               // UINT64 Alignment
			alignedCbSize,                   // UINT64 Width
			1,                               // UINT Height
			1,                               // UINT16 DepthOrArraySize
			1,                               // UINT16 MipLevels
			DXGI_FORMAT_UNKNOWN,             // DXGI_FORMAT Format
			defaultSampleDesc,               // DXGI_SAMPLE_DESC SampleDesc
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR,  // D3D12_TEXTURE_LAYOUT Layout
			D3D12_RESOURCE_FLAG_NONE,        // D3D12_RESOURCE_FLAGS Flags
		};

		m_objConstBuffer[i] = D3D12::CreateCommittedResource(
			device,
			constBufferDesc,
			writeCombineHeapProps,
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		if(!m_objConstBuffer[i])
		{
			LOG_ERROR("Failed to create scene object constant buffer [%zu]", i);
			return false;
		}

		m_objCbvDesc[i] = descAlloc->Allocate();
		if(m_objCbvDesc[i].index == D3D12::Descriptor::Invalid.index)
		{
			LOG_ERROR("Failed to allocate descriptor for scene object constant buffer [%zu]", i);
			return false;
		}

		const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc =
		{
			m_objConstBuffer[i]->GetGPUVirtualAddress(), // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation
			uint32_t(alignedCbSize),                     // UINT SizeInBytes
		};

		// Create the constant buffer view.
		device->CreateConstantBufferView(&cbvDesc, m_objCbvDesc[i].cpuHandle);
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
