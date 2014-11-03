#pragma once

#include <cstdint>


namespace oetl
{
/*
// Implementation details - see below for users
namespace _strDetail
{
	template<typename T>
	struct Num {
		T val;
	};

	template<typename T>
	inline ConcatStr< _strDetail::Num<T> > makeNumStr(T num)
	{
		_strDetail::Num<T> n = {num};
		return _strDetail::ConcatStr< _strDetail::Num<T> >(n);
	}
}*/
//-----------------------------------------------------------------------------


template<typename T>
T to_num(string_ref str);

//template<typename T>
//inline auto to_string(T num) -> decltype(_strDetail::makeNumStr(num))  { return _strDetail::makeNumStr(num); }

template<typename Char=char, typename T>
basic_string<Char> to_string(T num);

string to_string(double num, int signifDigit);
string to_string(long double num, int signifDigit);


//-----------------------------------------------------------------------------


namespace _strDetail
{
	inline int toStr(int num, char * str)
	{
		return sprintf(str, "%i", num);
	}
	inline int toStr(unsigned int num, char * str)
	{
		return sprintf(str, "%u", num);
	}
	inline int toStr(int num, wchar_t * str)
	{
		return swprintf(str, "%i", num);
	}
	inline int toStr(unsigned int num, wchar_t * str)
	{
		return swprintf(str, "%u", num);
	}

	inline int toStr(long num, char * str)
	{
		return sprintf(str, "%li", num);
	}
	inline int toStr(unsigned long num, char * str)
	{
		return sprintf(str, "%lu", num);
	}
	inline int toStr(unsigned long num, wchar_t * str)
	{
		return swprintf(str, "%lu", num);
	}
	inline int toStr(long num, wchar_t * str)
	{
		return swprintf(str, "%li", num);
	}

	inline int toStr(long long num, char * str)
	{
		return sprintf(str, "%lli", num);
	}
	inline int toStr(unsigned long long num, char * str)
	{
		return sprintf(str, "%llu", num);
	}
	inline int toStr(long long num, wchar_t * str)
	{
		return swprintf(str, "%lli", num);
	}
	inline int toStr(unsigned long long num, wchar_t * str)
	{
		return swprintf(str, "%llu", num);
	}

	inline int toStr(float num, char * str)
	{
		return sprintf(str, "%.6g", num);
	}
	inline int toStr(double num, char * str)
	{
		return sprintf(str, "%.6g", num);
	}
	inline int toStr(long double num, char * str)
	{
		return sprintf(str, "%.6Lg", num);
	}
	inline int toStr(float num, wchar_t * str)
	{
		return swprintf(str, "%.6g", num);
	}
	inline int toStr(double num, wchar_t * str)
	{
		return swprintf(str, "%.6g", num);
	}
	inline int toStr(long double num, wchar_t * str)
	{
		return swprintf(str, "%.6Lg", num);
	}


	inline long toNum(string_ref str, long)
	{
		return std::strtol(str.c_str(), nullptr, 0);
	}
	inline int toNum(string_ref str, int)
	{
		return static_cast<int>(toNum(str, long()));
	}
	inline unsigned long toNum(string_ref str, unsigned long)
	{
		return std::strtoul(str.c_str(), nullptr, 0);
	}
	inline unsigned int toNum(string_ref str, unsigned int)
	{
		return static_cast<unsigned int>(toNum(str, unsigned long()));
	}
	inline long long toNum(string_ref str, long long)
	{
		return std::strtoll(str.c_str(), nullptr, 0);
	}
	inline unsigned long long toNum(string_ref str, unsigned long long)
	{
		return std::strtoull(str.c_str(), nullptr, 0);
	}

	inline double toNum(string_ref str, double)
	{
		return std::strtod(str.c_str(), nullptr);
	}
	inline float toNum(string_ref str, float)
	{
		return std::strtof(str.c_str(), nullptr);
	}

	//inline strlen_type length(Num<int64_t> n)            { return 26; }
	//inline strlen_type length(Num<uint64_t> n)           { return 26; }
	//template<typename T>
	//inline strlen_type length(Num<T> n)                  { return 14; }

	//template<typename T>
	//inline void appendPreAlloc(_strDetail::Num<T> n, char * dest, strlen_type & len)
	//{
	//	auto nChars = _strDetail::toStr(n.val, dest + len);
	//	assert(0 <= nChars && static_cast<strlen_type>(nChars) <= _strDetail::length(n));
	//	len += nChars;
	//}

	template<bool>
	struct TextLenForIntSizeMoreThan4
	{
		strlen_type MAX = 26;
	};

	template<>
	struct TextLenForIntSizeMoreThan4<false>
	{
		strlen_type MAX = 14;
	};
}

} // namespace oetl

template<typename T> inline
T oetl::to_num(string_ref str)
{
	return _strDetail::toNum(str, T());
}

template<typename Char, typename T>
oetl::basic_string<Char> oetl::to_string(T num)
{
	// Get string length upper bound, based on if type is integer with size greater than 4 bytes or not
	basic_string<Char> s{ _strDetail::TextLenForIntSizeUpTo4< (sizeof num > 4) && std::is_integral<T>::value >::MAX };
	auto len = _strDetail::toStr(num, s.data());
	assert(static_cast<strlen_type>(len) <= s.size());
	s.shorten_to(len);
	return s;
}

string to_string(double num, int signifDigit)
{
	int const BUF_SIZE = 24;
	string s{BUF_SIZE - 1};
	auto len = snprintf(s.data(), BUF_SIZE, "%.*g", signifDigit, num);
	s.shorten_to(len);
	return s;
}

string to_string(long double num, int signifDigit)
{
	int const BUF_SIZE = 26;
	string s{BUF_SIZE - 1};
	auto len = snprintf(s.data(), BUF_SIZE, "%.*Lg", signifDigit, num);
	s.shorten_to(len);
	return s;
}

/*
unsigned int numChars(int32_t n)
{
    unsigned int r = 1;
    if (n < 0) {
		++r;
		n = (n != INT32_MIN) ? -n : INT32_MAX;
	}
    if (n >= 100000000) {
        r += 8;
        n /= 100000000;
    }
    if (n >= 10000) {
        r += 4;
        n /= 10000;
    }
    if (n >= 100) {
        r += 2;
        n /= 100;
    }
    if (n >= 10)
        ++r;

    return r;
}

unsigned int numChars(uint32_t n)
{
    unsigned int r = 1;

	if (n >= 100000000) {
        r += 8;
        n /= 100000000;
    }
    if (n >= 10000) {
        r += 4;
        n /= 10000;
    }
    if (n >= 100) {
        r += 2;
        n /= 100;
    }
    if (n >= 10)
        ++r;

    return r;
}

unsigned int numChars(int64_t n)
{
    unsigned int r = 1;
    if (n < 0) {
		++r;
		n = (n != INT64_MIN) ? -n : INT64_MAX;
	}
    if (n >= 10000000000000000) {
        r += 16;
        n /= 10000000000000000;
    }
    if (n >= 100000000) {
        r += 8;
        n /= 100000000;
    }
    if (n >= 10000) {
        r += 4;
        n /= 10000;
    }
    if (n >= 100) {
        r += 2;
        n /= 100;
    }
    if (n >= 10)
        ++r;

    return r;
}

unsigned int numChars(uint64_t n)
{
    unsigned int r = 1;

	if (n >= 10000000000000000) {
        r += 16;
        n /= 10000000000000000;
    }
    if (n >= 100000000) {
        r += 8;
        n /= 100000000;
    }
    if (n >= 10000) {
        r += 4;
        n /= 10000;
    }
    if (n >= 100) {
        r += 2;
        n /= 100;
    }
    if (n >= 10)
        ++r;

    return r;
}
*/