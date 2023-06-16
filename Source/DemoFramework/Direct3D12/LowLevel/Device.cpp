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

#include "Device.hpp"

#include "../../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::DevicePtr DemoFramework::D3D12::CreateDevice(const AdapterPtr& adapter)
{
	if(!adapter)
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	DevicePtr output;
	InfoQueuePtr infoQueue;

	// Create the device using the input display adapter.
	const HRESULT createDeviceResult = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&output));
	if(createDeviceResult != S_OK)
	{
		LOG_ERROR("Failed to create device; result='0x%08" PRIX32 "'", createDeviceResult);
		return nullptr;
	}

	// Setup the D3D12 logging interface when the debug layer is enabled.
	if(SUCCEEDED(output.As(&infoQueue)))
	{
		// Configure the internal logger to break when certain severities are logged.
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// Message severities to ignore.
		D3D12_MESSAGE_SEVERITY ignoredSeverities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO,
		};

		// Individual message IDs to ignore.
		D3D12_MESSAGE_ID ignoredMessageIds[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
		};

		D3D12_INFO_QUEUE_FILTER infoQueueFilter = {};
		infoQueueFilter.DenyList.NumSeverities = DF_ARRAY_LENGTH(ignoredSeverities);
		infoQueueFilter.DenyList.pSeverityList = ignoredSeverities;
		infoQueueFilter.DenyList.NumIDs = DF_ARRAY_LENGTH(ignoredMessageIds);
		infoQueueFilter.DenyList.pIDList = ignoredMessageIds;

		// Configure what is allowed to be logged.
		const HRESULT pushFilterResult = infoQueue->PushStorageFilter(&infoQueueFilter);
		if(pushFilterResult != S_OK)
		{
			LOG_ERROR("Failed to push info queue filter; result='0x%08" PRIX32, pushFilterResult);
			return nullptr;
		}
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
