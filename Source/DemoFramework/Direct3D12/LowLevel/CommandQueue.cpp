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

#include "CommandQueue.hpp"

#include "../../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::CommandQueue::Ptr DemoFramework::D3D12::CreateCommandQueue(const Device::Ptr& device, const D3D12_COMMAND_QUEUE_DESC& desc)
{
	if(!device)
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	D3D12::CommandQueue::Ptr output;

	const HRESULT result = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&output));
	if(result != S_OK)
	{
		LOG_ERROR("Failed to create command queue; result='0x%08" PRIX32 "'", result);
		return nullptr;
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
