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

#include "Mesh/StaticMesh.hpp"

#include <tiny_obj_loader.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class WavefrontObj;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::WavefrontObj
{
public:

	typedef std::shared_ptr<WavefrontObj> Ptr;

	WavefrontObj();

	static Ptr Load(
		const Device::Ptr& device,
		const GraphicsCommandList::Ptr& cmdList,
		const char* name,
		const char* filePath);

	void Draw(const GraphicsCommandList::Ptr& cmdList) const;

	const StaticMesh::PtrArray& GetMeshes() const;


private:

	struct InternalData;

	bool _build(const InternalData&, const Device::Ptr&, const GraphicsCommandList::Ptr&);

	StaticMesh::PtrArray m_meshes;
};

//---------------------------------------------------------------------------------------------------------------------

template class DF_API DemoFramework::D3D12::WavefrontObj::Ptr;

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::WavefrontObj::WavefrontObj()
	: m_meshes()
{
}

//---------------------------------------------------------------------------------------------------------------------

inline const DemoFramework::D3D12::StaticMesh::PtrArray& DemoFramework::D3D12::WavefrontObj::GetMeshes() const
{
	return m_meshes;
}

//---------------------------------------------------------------------------------------------------------------------
