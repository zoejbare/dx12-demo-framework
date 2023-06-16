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

#include "RootSignature.hpp"

#include "../../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::RootSignaturePtr DemoFramework::D3D12::CreateRootSignature(const DevicePtr& device, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	if(!device)
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionDesc;
	versionDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
	versionDesc.Desc_1_0 = desc;

	return CreateVersionedRootSignature(device, versionDesc);
}

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::RootSignaturePtr DemoFramework::D3D12::CreateVersionedRootSignature(
	const DevicePtr& device,
	const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc)
{
	if(!device)
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	D3D12::BlobPtr signature;
	D3D12::BlobPtr error;

	const HRESULT serializeResult = D3D12SerializeVersionedRootSignature(&desc, &signature, &error);
	if(serializeResult != S_OK)
	{
		char* const errorMsg = reinterpret_cast<char*>(error->GetBufferPointer());
		assert(errorMsg != nullptr);

		// Trim all whitespace from the end of the error message.
		for(;;)
		{
			const size_t length = strlen(errorMsg);

			if(length == 0)
			{
				// The message string is empty.
				break;
			}

			const size_t endIndex = length - 1;

			if(isspace(errorMsg[endIndex]))
			{
				// Replace the character at the end of the string with a null terminator.
				errorMsg[endIndex] = '\0';
			}
			else
			{
				// The last character is valid, so we can stop trimming now.
				break;
			}
		}

		LOG_ERROR("Failed to serialize root signature; result='0x%08" PRIX32 "'\n\tMsg: %s", serializeResult, errorMsg);
		return nullptr;
	}

	D3D12::RootSignaturePtr output;

	const HRESULT createResult = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&output));
	if(createResult != S_OK)
	{
		LOG_ERROR("Failed to create root signature; result='0x%08" PRIX32 "'", createResult);
		return nullptr;
	}

	return output;
}

//---------------------------------------------------------------------------------------------------------------------
