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

#include "../BuildSetup.h"

#include <assert.h>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace Utility {
	template <typename T>
	class Array;
}}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
class DemoFramework::Utility::Array
{
public:

	Array();
	Array(const Array& other);
	Array(Array&& other);
	~Array();

	Array<T>& operator =(const Array<T>& other);
	Array<T>& operator =(Array<T>&& other);

	static Array<T> Create(size_t count);

	T* GetData();
	const T* GetData() const;

	size_t GetCount() const;


private:

	void _initCopy(T*);

	T* m_data;

	size_t m_count;
};

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
DemoFramework::Utility::Array<T>::Array()
	: m_data(nullptr)
	, m_count(0)
{
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
DemoFramework::Utility::Array<T>::Array(const Array<T>& other)
	: m_data(nullptr)
	, m_count(other.m_count)
{
	if(m_count > 0)
	{
		_initCopy(other.m_data);
	}
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
DemoFramework::Utility::Array<T>::Array(Array<T>&& other)
	: m_data(other.m_data)
	, m_count(other.m_count)
{
	other.m_data = nullptr;
	other.m_count = 0;
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
DemoFramework::Utility::Array<T>::~Array()
{
	if(m_data)
	{
		delete[] m_data;
	}
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
DemoFramework::Utility::Array<T>& DemoFramework::Utility::Array<T>::operator =(const Array<T>& other)
{
	if(m_data)
	{
		delete[] m_data;
		m_data = nullptr;
	}

	m_count = other.m_count;

	if(m_count > 0)
	{
		_initCopy(other.m_data);
	}

	return (*this);
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
DemoFramework::Utility::Array<T>& DemoFramework::Utility::Array<T>::operator =(Array<T>&& other)
{
	if(m_data)
	{
		delete[] m_data;
		m_data = nullptr;
	}

	m_data = other.m_data;
	m_count = other.m_count;

	other.m_data = nullptr;
	other.m_count = 0;

	return (*this);
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
DemoFramework::Utility::Array<T> DemoFramework::Utility::Array<T>::Create(const size_t count)
{
	Array<T> output;

	if(count == 0)
	{
		return output;
	}

	output.m_data = new T[count];
	output.m_count = count;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
T* DemoFramework::Utility::Array<T>::GetData()
{
	return m_data;
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
const T* DemoFramework::Utility::Array<T>::GetData() const
{
	return m_data;
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
size_t DemoFramework::Utility::Array<T>::GetCount() const
{
	return m_count;
}

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
void DemoFramework::Utility::Array<T>::_initCopy(T* const otherData)
{
	assert(m_data == nullptr);

	m_data = new T[m_count];

	// Don't do a memcpy here. We don't know the templated data type. Memcpy is fine for
	// primitive data, but complex types may require the use of their own copy constructors.
	for(uint32_t i = 0; i < m_count; ++i)
	{
		m_data[i] = otherData[i];
	}
}

//---------------------------------------------------------------------------------------------------------------------
