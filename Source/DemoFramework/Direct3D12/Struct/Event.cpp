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

EXTERN_C const IID IID_D3D12_IEvent = __uuidof(DemoFramework::D3D12::IEvent);

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	struct EventImpl
		: public IEvent
	{
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& refId, void** ppObject) override;
        virtual ULONG STDMETHODCALLTYPE AddRef() override;
        virtual ULONG STDMETHODCALLTYPE Release() override;


		virtual HANDLE GetHandle() const override;

		HANDLE m_handle;
		ULONG m_ref;
	};
}}

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::EventPtr DemoFramework::D3D12::CreateEvent(
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

	EventImpl* const pOutput = new EventImpl();
	assert(pOutput != nullptr);

	pOutput->m_handle = handle;
	pOutput->m_ref = 1;

	EventPtr output;
	output.Attach(pOutput);

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

HRESULT DemoFramework::D3D12::EventImpl::QueryInterface(const IID& refId, void** const ppObject)
{
	if(refId == IID_D3D12_IEvent)
	{
		(*ppObject) = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

//---------------------------------------------------------------------------------------------------------------------

ULONG DemoFramework::D3D12::EventImpl::AddRef()
{
	++m_ref;
	return m_ref;
}

//---------------------------------------------------------------------------------------------------------------------

ULONG DemoFramework::D3D12::EventImpl::Release()
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

HANDLE DemoFramework::D3D12::EventImpl::GetHandle() const
{
	return m_handle;
}

//---------------------------------------------------------------------------------------------------------------------
