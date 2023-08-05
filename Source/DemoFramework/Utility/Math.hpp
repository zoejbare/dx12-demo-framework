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
	class Math;
}}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::Utility::Math
{
public:

	Math() = delete;
	Math(const Math&) = delete;
	Math(Math&&) = delete;

	template <typename T>
	static T GetPowerOfTwo(T value);
};

//---------------------------------------------------------------------------------------------------------------------

template <typename T>
inline T DemoFramework::Utility::Math::GetPowerOfTwo(const T value)
{
	// Unsupported type
	assert(false);
	return T();
}

//---------------------------------------------------------------------------------------------------------------------

template <>
inline int8_t DemoFramework::Utility::Math::GetPowerOfTwo(const int8_t value)
{
	if(value > 0)
	{
		int8_t temp = value - 1;

		temp |= temp >> 1;
		temp |= temp >> 2;
		temp |= temp >> 4;

		return temp + 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------

template <>
inline int16_t DemoFramework::Utility::Math::GetPowerOfTwo(const int16_t value)
{
	if(value > 0)
	{
		int16_t temp = value - 1;

		temp |= temp >> 1;
		temp |= temp >> 2;
		temp |= temp >> 4;
		temp |= temp >> 8;

		return temp + 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------

template <>
inline int32_t DemoFramework::Utility::Math::GetPowerOfTwo(const int32_t value)
{
	if(value > 0)
	{
		int32_t temp = value - 1;

		temp |= temp >> 1;
		temp |= temp >> 2;
		temp |= temp >> 4;
		temp |= temp >> 8;
		temp |= temp >> 16;

		return temp + 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------

template <>
inline int64_t DemoFramework::Utility::Math::GetPowerOfTwo(const int64_t value)
{
	if(value > 0)
	{
		int64_t temp = value - 1;

		temp |= temp >> 1;
		temp |= temp >> 2;
		temp |= temp >> 4;
		temp |= temp >> 8;
		temp |= temp >> 16;
		temp |= temp >> 32;

		return temp + 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------

template <>
inline uint8_t DemoFramework::Utility::Math::GetPowerOfTwo(const uint8_t value)
{
	if(value > 0)
	{
		uint8_t temp = value - 1;

		temp |= temp >> 1;
		temp |= temp >> 2;
		temp |= temp >> 4;

		return temp + 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------

template <>
inline uint16_t DemoFramework::Utility::Math::GetPowerOfTwo(const uint16_t value)
{
	if(value > 0)
	{
		uint16_t temp = value - 1;

		temp |= temp >> 1;
		temp |= temp >> 2;
		temp |= temp >> 4;
		temp |= temp >> 8;

		return temp + 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------

template <>
inline uint32_t DemoFramework::Utility::Math::GetPowerOfTwo(const uint32_t value)
{
	if(value > 0)
	{
		uint32_t temp = value - 1;

		temp |= temp >> 1;
		temp |= temp >> 2;
		temp |= temp >> 4;
		temp |= temp >> 8;
		temp |= temp >> 16;

		return temp + 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------

template <>
inline uint64_t DemoFramework::Utility::Math::GetPowerOfTwo(const uint64_t value)
{
	if(value > 0)
	{
		uint64_t temp = value - 1;

		temp |= temp >> 1;
		temp |= temp >> 2;
		temp |= temp >> 4;
		temp |= temp >> 8;
		temp |= temp >> 16;
		temp |= temp >> 32;

		return temp + 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------
