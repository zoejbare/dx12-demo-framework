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

#include "Shader.hpp"

#include "../Application/Log.hpp"

#include "LowLevel/Blob.hpp"

#include <stdio.h>

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Blob::Ptr DemoFramework::D3D12::LoadShaderFromFile(const char* const filePath)
{
	if(!filePath || filePath[0] == '\0')
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	LOG_WRITE("Loading shader '%s' ...", filePath);

	// Open the input file.
	FILE* const pFile = fopen(filePath, "rb");
	if(!pFile)
	{
		LOG_ERROR("Failed to open file: %s", filePath);
		return nullptr;
	}

	// Find the size of the file.
	fseek(pFile, 0, SEEK_END);
	const size_t fileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	// Verify the file is not empty.
	if(fileSize == 0)
	{
		fclose(pFile);
		LOG_ERROR("Shader file is empty: %s", filePath);
		return nullptr;
	}

	// Create the blob that will manage the lifetime of the shader data.
	Blob::Ptr pOutput = CreateBlob(fileSize);
	if(!pOutput)
	{
		fclose(pFile);
		return nullptr;
	}

	// Read the contents of the file directly into the blob.
	fread(pOutput->GetBufferPointer(), fileSize, 1, pFile);
	fclose(pFile);

	return pOutput;
}

//---------------------------------------------------------------------------------------------------------------------
