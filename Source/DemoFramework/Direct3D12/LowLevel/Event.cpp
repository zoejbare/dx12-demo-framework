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

#include "Event.hpp"

#include "../../Application/Log.hpp"

//---------------------------------------------------------------------------------------------------------------------

EXTERN_C const IID IID_D3D12_ID3DEvent = __uuidof(ID3DEvent);

//---------------------------------------------------------------------------------------------------------------------

struct D3DEventImpl
	: public ID3DEvent
{
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& refId, void** ppObject) override;
	virtual ULONG STDMETHODCALLTYPE AddRef() override;
	virtual ULONG STDMETHODCALLTYPE Release() override;


	virtual HANDLE GetHandle() const override;

	HANDLE m_handle;
	ULONG m_ref;
};

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Event::Ptr DemoFramework::D3D12::CreateEvent(
	SECURITY_ATTRIBUTES* const pEventAttr,
	const bool manualReset,
	const bool initialState,
	const char* const name)
{
	HANDLE handle = ::CreateEventA(pEventAttr, manualReset, initialState, name);
	if(!handle)
	{
		LOG_ERROR("Failed to create event handle; result='0x%08" PRIX32 "'", GetLastError());
		return nullptr;
	}

	D3DEventImpl* const pOutput = new D3DEventImpl();
	assert(pOutput != nullptr);

	pOutput->m_handle = handle;
	pOutput->m_ref = 1;

	Event::Ptr output;
	output.Attach(pOutput);

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

HRESULT D3DEventImpl::QueryInterface(const IID& refId, void** const ppObject)
{
	if(refId == IID_D3D12_ID3DEvent)
	{
		(*ppObject) = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

//---------------------------------------------------------------------------------------------------------------------

ULONG D3DEventImpl::AddRef()
{
	++m_ref;
	return m_ref;
}

//---------------------------------------------------------------------------------------------------------------------

ULONG D3DEventImpl::Release()
{
	--m_ref;

	const ULONG ref = m_ref;

	if(ref == 0)
	{
		CloseHandle(m_handle);
		delete this;
	}

	return ref;
}

//---------------------------------------------------------------------------------------------------------------------

HANDLE D3DEventImpl::GetHandle() const
{
	return m_handle;
}

//---------------------------------------------------------------------------------------------------------------------
