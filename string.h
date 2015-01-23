#ifndef ren_cxx_filesystem__string_h
#define ren_cxx_filesystem__string_h

#include "../ren-cxx-basics/extrastandard.h"

#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <list>
#include <fstream>
#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef WINDOWS

inline std::vector<wchar_t> ToNativeString(std::string const &Input) // Return contains 0 at size() - 1
{
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "Assumption that all Windows systems use 16-bit wide characters failed!");
	if (Input.empty()) return {};
	const int Length = MultiByteToWideChar(CP_UTF8, 0, Input.c_str(), Input.length(), nullptr, 0);
	AssertGT(Length, 0);
	std::vector<wchar_t> ConversionBuffer(Length + 1);
	MultiByteToWideChar(CP_UTF8, 0, Input.c_str(), Input.length(), (LPWSTR)&ConversionBuffer[0], Length);
	ConversionBuffer[Length] = 0;
	return ConversionBuffer;
}

inline std::string FromNativeString(wchar_t const *Input, size_t InputLength)
{
	const int Length = WideCharToMultiByte(CP_UTF8, 0, Input, InputLength, nullptr, 0, nullptr, nullptr);
	AssertGT(Length, 0);
	std::vector<char> ConversionBuffer(Length);
	WideCharToMultiByte(CP_UTF8, 0, Input, InputLength, &ConversionBuffer[0], Length, nullptr, nullptr);
	return std::string(&ConversionBuffer[0], Length);
}

inline std::string FromNativeString(std::vector<wchar_t> const &Input) // Input must not be null terminated
	{ return FromNativeString(&Input[0], Input.size()); }

#else

inline std::string ToNativeString(std::string const &Input)
	{ return Input; }

inline std::string FromNativeString(std::string const &Input)
	{ return Input; }

#endif

#endif
