#pragma once

#define RANGE_COMPLETE 1

#include "array_iterator.h"
#include "util.h"

#include <boost/range/algorithm.hpp>
#include <iosfwd>
#include <locale>
//#include <array>


namespace oetl
{

using namespace boost::range;

using std::out_of_range;


typedef size_t strlen_type;

auto const str_npos = strlen_type(-1);


namespace _strDetail
{
	template<typename Char>
	class StringBase
	{
	public:
		typedef ptrdiff_t    difference_type;
		typedef Char         value_type;
		typedef const Char & const_reference;
#	if PTR_AS_OETL_ARRAY_ITERATOR
		typedef const Char *                             const_iterator
#	else
		typedef array_const_iterator< StringBase<Char> > const_iterator;
#	endif

		bool        empty() const NOEXCEPT;

		strlen_type length() const NOEXCEPT     { return _len; }
		strlen_type size() const NOEXCEPT;      { return _len; }

		const_iterator begin() const NOEXCEPT;

		const_iterator end() const NOEXCEPT;

		const_reference at(strlen_type index) const;

		const_reference operator[](strlen_type index) const NOEXCEPT;


		bool starts_with(Char ch) const NOEXCEPT
		{
			return size() > 0 && _data[0] == ch;
		}
		bool starts_with(StringBase str) const NOEXCEPT
		{
			return size() >= str.size() && memcmp(_data, str._data, str.size() * sizeof(Char)) == 0;
		}

		bool ends_with(Char ch) const NOEXCEPT
		{
			return size() > 0 && _data[size() - 1] == ch;
		}
		bool ends_with(StringBase str) const NOEXCEPT
		{
			ptrdiff_t diff = size() - str.size();
			return diff >= 0 && memcmp(_data + diff, str._data, str.size() * sizeof(Char)) == 0;
		}


	protected:
		Char *      _data;
		strlen_type _len;
	};
}


template<typename Char>
class basic_string_ref   : private _strDetail::StringBase<Char>
{
	typedef typename _strDetail::StringBase<Char> _stringBase;

public:
	typedef strlen_type                  size_type;
	typedef _stringBase::difference_type difference_type;
	typedef _stringBase::value_type      value_type;
	typedef _stringBase::const_reference const_reference;
	typedef _stringBase::const_iterator  const_iterator;

	basic_string_ref() NOEXCEPT;
	template<size_type Size>
	basic_string_ref(const Char (&toWrap)[Size]) NOEXCEPT;
	basic_string_ref(const Char * toWrap, size_type len) NOEXCEPT;

	using _stringBase::empty;

	using _stringBase::length;
	using _stringBase::size;

	using _stringBase::begin;
	using _stringBase::end;

	using _stringBase::at;

	using _stringBase::operator[];

	const Char * data() const NOEXCEPT   { return _data; }

	bool starts_with(Char ch) const NOEXCEPT;
	bool starts_with(basic_string_ref str) const NOEXCEPT;

	bool ends_with(Char ch) const NOEXCEPT;
	bool ends_with(basic_string_ref str) const NOEXCEPT;

	friend class basic_string;
};


template<typename> class concat_str;


template<typename Char>
class basic_string   : private _strDetail::StringBase<Char>
{
	typedef _strDetail::StringBase<Char> _stringBase;

public:
	typedef strlen_type                  size_type;
	typedef _stringBase::difference_type difference_type;
	typedef _stringBase::value_type      value_type;
	typedef Char &                       reference;
	typedef _stringBase::const_reference const_reference;
#if PTR_AS_OETL_ARRAY_ITERATOR
	typedef Char *                       iterator
#else
	typedef array_iterator<_stringBase>  iterator;
#endif
	typedef _stringBase::const_iterator  const_iterator;

	basic_string() NOEXCEPT;
	template<size_type Size>
	basic_string(const Char (&source)[Size]);
	basic_string(const Char * source, size_type lenToCopy);
	explicit basic_string(size_type len);
	basic_string(basic_string && source) NOEXCEPT;
	basic_string(const basic_string & source);
	basic_string(const basic_string_ref & source);
	template<typename T>
	basic_string(const concat_str<T> & source);

	~basic_string() NOEXCEPT;

	basic_string & operator =(basic_string && right) NOEXCEPT;
	basic_string & operator =(const basic_string & right);
	basic_string & operator =(const basic_string_ref & right);
	template<typename T>
	basic_string & operator =(const concat_str<T> & right);
	template<size_type Size>
	basic_string & operator =(const Char (&right)[Size]);

	void         assign(const Char * source, size_type lenToCopy);

	void         swap(basic_string & other) NOEXCEPT;

	operator const basic_string_ref &() const NOEXCEPT;

	using _stringBase::empty;

	using _stringBase::length;
	using _stringBase::size;

	iterator       begin() NOEXCEPT;
	const_iterator begin() const NOEXCEPT;

	iterator       end() NOEXCEPT;
	const_iterator end() const NOEXCEPT;

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) NOEXCEPT;
	const_reference operator[](size_type index) const NOEXCEPT;

	Char *       data() NOEXCEPT         { return _data; }
	const Char * data() const NOEXCEPT   { return _data; }

	const Char * c_str() const NOEXCEPT;

	bool starts_with(Char ch) const NOEXCEPT;
	bool starts_with(basic_string_ref str) const NOEXCEPT;

	bool ends_with(Char ch) const NOEXCEPT;
	bool ends_with(basic_string_ref str) const NOEXCEPT;

	void     truncate(iterator newEnd) NOEXCEPT;
	iterator erase(iterator position) NOEXCEPT;
	iterator erase(iterator first, iterator last) NOEXCEPT;

	void     shorten_to(size_type newLen) NOEXCEPT;
	void     erase_idx(size_type index);
	void     erase(size_type index, size_type count) NOEXCEPT;

	void     clear() NOEXCEPT;


private:
	static const Char _nullChar = '\0';

	static Char * _alloc(size_type memSize)
	{
		return static_cast<Char *>( ::operator new[](memSize) * sizeof(Char) );
	}

	static void _dealloc(Char * ptr)
	{
		::operator delete[](ptr);
	}

	void _resetData(Char * newData)
	{
		_dealloc(_data);
		_data = newData;
	}

	void _copyData(const Char * source)
	{
		::memcpy(data(), source, _len * sizeof(Char));
		_data[_len] = _nullChar;
	}

	void _init(const Char * source, size_type len)
	{
		_data = _alloc(len + 1);
		_len = len;
		_copyData(source);
	}

	template<size_type Size>
	void _init(const Char (&source)[Size])
	{
		_data = _alloc(Size);
		_len = Size - 1;
		_copyData(source);
	}
	void _init(const Char (&)[1])
	{
		_data = nullptr;
		_len = 0;
	}

	template<size_type Size>
	void _assign(const Char (&source)[Size])
	{
		_resetData(_alloc(Size));
		_len = Size - 1;
		_copyData(source);
	}
	void _assign(const Char (&)[1])
	{
		_resetData(nullptr);
		_len = 0;
	}
};

template<typename Char> inline
void swap(basic_string<Char> & a, basic_string<Char> & b) NOEXCEPT  { a.swap(b); }

template<typename Char>
struct is_trivially_reallocatable< basic_string_ref<Char> > :
	public std::true_type {};

template<typename Char>
struct is_trivially_reallocatable< basic_string<Char> > :
	public std::true_type {};


typedef basic_string_ref<char> string_ref;
typedef basic_string<char>     string;

typedef basic_string_ref<wchar_t> wstring_ref;
typedef basic_string<wchar_t>     wstring;

typedef basic_string_ref<std::char16_t> u16string_ref;
typedef basic_string<std::char16_t>     u16string;


//inline int compare(basic_string_ref a, basic_string_ref b)     { return _strDetail::Compare(a, b); }
//inline int compare(basic_string_ref str, const str_char * cStr)       { return _strDetail::Compare(str, cStr); }
//inline int compare(const str_char * cStr, basic_string_ref str)       { return _strDetail::Compare(cStr, str); }
//inline int compare(const str_char * cStr1, const str_char * cStr2)           { return strcmp(cStr1, cStr2); }
//// Case insensitive - FIXME
//inline int compare_ci(basic_string_ref a, basic_string_ref b)  { return _stricmp(a.c_str(), b.c_str()); }
//inline int compare_ci(basic_string_ref str, const str_char * cStr)    { return _stricmp(str.c_str(), cStr); }
//inline int compare_ci(const str_char * cStr, basic_string_ref str)    { return _stricmp(cStr, str.c_str()); }
//inline int compare_ci(const str_char * cStr1, const str_char * cStr2)        { return _stricmp(cStr1, cStr2); }

// REMINDER: test performance
template<typename Char>
bool operator==(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT;
template<typename Char>
bool operator!=(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT;
template<typename Char>
bool operator <(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT;
template<typename Char>
bool operator >(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT;
template<typename Char>
bool operator<=(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT;
template<typename Char>
bool operator>=(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT;

template<typename Char>
basic_string_ref<Char> to_string_ref(const Char * cStr) NOEXCEPT;

template<typename Char, strlen_type Size>
strlen_type copy_cstr_min(basic_string_ref<Char> source, Char (&dest)[Size]) NOEXCEPT;
template<typename Char>
strlen_type copy_cstr_min(basic_string_ref<Char> source, Char * dest, strlen_type destSize) NOEXCEPT;

template<typename Char, strlen_type Size>
void copy_cstr(basic_string_ref<Char> source, Char (&dest)[Size]);
template<typename Char>
void copy_cstr(basic_string_ref<Char> source, Char * dest, strlen_type destSize);


template<typename Char>
strlen_type find_idx(basic_string_ref<Char> toSearch, Char ch, strlen_type minPos) NOEXCEPT;

template<typename Char>
strlen_type find_str(basic_string_ref<Char> toSearch, basic_string_ref<Char> str, strlen_type minPos = 0) NOEXCEPT;

template<typename Char>
strlen_type rfind_idx(basic_string_ref<Char> toSearch, Char ch, strlen_type endPos) NOEXCEPT;

template<typename Char>
strlen_type rfind_str(basic_string_ref<Char> toSearch, basic_string_ref<Char> str, strlen_type endPos = str_npos) NOEXCEPT;

template<typename Char>
strlen_type find_first_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> oneOf, strlen_type minPos = 0) NOEXCEPT;

template<typename Char>
strlen_type find_first_not_of(basic_string_ref<Char> toSearch, Char notOf, strlen_type minPos = 0) NOEXCEPT;
template<typename Char>
strlen_type find_first_not_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> notOf, strlen_type minPos = 0) NOEXCEPT;

template<typename Char>
strlen_type find_last_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> oneOf, strlen_type endPos = str_npos) NOEXCEPT;

template<typename Char>
strlen_type find_last_not_of(basic_string_ref<Char> toSearch, Char notOf, strlen_type endPos = str_npos) NOEXCEPT;
template<typename Char>
strlen_type find_last_not_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> notOf, strlen_type endPos = str_npos) NOEXCEPT;

template<typename Char>
basic_string_ref<Char> substr(basic_string_ref<Char> str, strlen_type startPos) NOEXCEPT;
template<typename Char>
basic_string<Char>     substr(basic_string<Char> && str, strlen_type startPos) NOEXCEPT;
template<typename Char>
basic_string_ref<Char> substr(basic_string_ref<Char> str, strlen_type startPos, strlen_type count) NOEXCEPT;
template<typename Char>
basic_string<Char>     substr(basic_string<Char> && str, strlen_type startPos, strlen_type count) NOEXCEPT;

template<typename Char>
basic_string_ref<Char> left(basic_string_ref<Char> str, strlen_type count) NOEXCEPT;
template<typename Char>
basic_string<Char>     left(basic_string<Char> && str, strlen_type count) NOEXCEPT;

template<typename Char>
basic_string_ref<Char> right(basic_string_ref<Char> str, strlen_type count) NOEXCEPT;
template<typename Char>
basic_string<Char>     right(basic_string<Char> && str, strlen_type count) NOEXCEPT;

template<typename Char>
basic_string_ref<Char> rtrim(basic_string_ref<Char> str, std::locale loc = std::locale());
template<typename Char>
basic_string<Char>     rtrim(basic_string<Char> && str, std::locale loc = std::locale());
template<typename Char>
basic_string_ref<Char> rtrim(basic_string_ref<Char> str, Char toErase) NOEXCEPT;
template<typename Char>
basic_string<Char>     rtrim(basic_string<Char> && str, Char toErase) NOEXCEPT;
template<typename Char>
basic_string_ref<Char> rtrim(basic_string_ref<Char> str, basic_string_ref<Char> charsToErase) NOEXCEPT;
template<typename Char>
basic_string<Char>     rtrim(basic_string<Char> && str, basic_string_ref<Char> charsToErase) NOEXCEPT;

template<typename Char>
basic_string_ref<Char> trim(basic_string_ref<Char> str, std::locale loc = std::locale());
template<typename Char>
basic_string<Char>     trim(basic_string<Char> && str, std::locale loc = std::locale());
template<typename Char>
basic_string_ref<Char> trim(basic_string_ref<Char> str, Char toErase) NOEXCEPT;
template<typename Char>
basic_string<Char>     trim(basic_string<Char> && str, Char toErase) NOEXCEPT;
template<typename Char>
basic_string_ref<Char> trim(basic_string_ref<Char> str, basic_string_ref<Char> charsToErase) NOEXCEPT;
template<typename Char>
basic_string<Char>     trim(basic_string<Char> && str, basic_string_ref<Char> charsToErase) NOEXCEPT;

template<typename Char>
std::ostream & operator<<(std::ostream & os, basic_string_ref<Char> str);

//-----------------------------------------------------------------------------
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Implementation details

template<typename T>
class concat_str
{
public:
	typedef T value_type;

	concat_str(const T & data)  : _data(data) {}

	const T & data() const      { return _data; }
	strlen_type length() const  { return _strDetail::Length(_data); }

	friend concat_str< std::pair<basic_string_ref, basic_string_ref> >
		operator +(basic_string_ref<Char> first, basic_string_ref<Char> second);
	friend concat_str< std::pair<basic_string_ref, Char> >
		operator +(basic_string_ref<Char> first, Char second);
	friend concat_str< std::pair<Char, basic_string_ref> >
		operator +(Char first, basic_string_ref<Char> second);
	template<typename T>
	friend concat_str< std::pair<T, Char> >
		operator +(const concat_str<T> & first, Char second);
	template<typename T>
	friend concat_str< std::pair<Char, T> >
		operator +(Char first, const concat_str<T> & second);
	template<typename T>
	friend concat_str< std::pair<T, basic_string_ref> >
		operator +(const concat_str<T> & first, basic_string_ref<Char> second);
	template<typename T>
	friend concat_str< std::pair<basic_string_ref, T> >
		operator +(basic_string_ref<Char> first, const concat_str<T> & second);
	template<typename T, typename T2>
	friend concat_str< std::pair<T, T2> >
		operator +(const concat_str<T> & first, const concat_str<T2> & second);

private:
	T _data;

	concat_str(const concat_str & other) :
		_data(other._data) {
	}

	void operator =(const concat_str &) = delete;
};

inline concat_str< std::pair<basic_string_ref, basic_string_ref> >
	operator +(basic_string_ref<Char> first, basic_string_ref<Char> second)
{
	typedef concat_str< std::pair<basic_string_ref, basic_string_ref> > Ret;
	return Ret(Ret::value_type(first, second));
}
inline concat_str< std::pair<basic_string_ref, Char> >
	operator +(basic_string_ref<Char> first, Char second)
{
	typedef concat_str< std::pair<basic_string_ref, Char> > Ret;
	return Ret(Ret::value_type(first, second));
}
inline concat_str< std::pair<Char, basic_string_ref> >
	operator +(Char first, basic_string_ref<Char> second)
{
	typedef concat_str< std::pair<Char, basic_string_ref> > Ret;
	return Ret(Ret::value_type(first, second));
}
template<typename T>
inline concat_str< std::pair<T, Char> >
	operator +(const concat_str<T> & first, Char second)
{
	typedef concat_str< std::pair<T, Char> > Ret;
	return Ret(Ret::value_type(first.data(), second));
}
template<typename T>
inline concat_str< std::pair<Char, T> >
	operator +(Char first, const concat_str<T> & second)
{
	typedef concat_str< std::pair<Char, T> > Ret;
	return Ret(Ret::value_type(first, second.data()));
}
template<typename T>
inline concat_str< std::pair<T, basic_string_ref> >
	operator +(const concat_str<T> & first, basic_string_ref<Char> second)
{
	typedef concat_str< std::pair<T, basic_string_ref> > Ret;
	return Ret(Ret::value_type(first.data(), second));
}
template<typename T>
inline concat_str< std::pair<basic_string_ref, T> >
	operator +(basic_string_ref<Char> first, const concat_str<T> & second)
{
	typedef concat_str< std::pair<basic_string_ref, T> > Ret;
	return Ret(Ret::value_type(first, second.data()));
}
template<typename T, typename T2>
inline concat_str< std::pair<T, T2> >
	operator +(const concat_str<T> & first, const concat_str<T2> & second)
{
	typedef concat_str< std::pair<T, T2> > Ret;
	return Ret(Ret::value_type(first.data(), second.data()));
}

template<typename T> inline
basic_string<Char> & operator+=(basic_string<Char> & left, const concat_str<T> & right)
{
	return left = left + right;
}


//-----------------------------------------------------------------------------

namespace _strDetail
{
	template<class String>
	inline strlen_type Length(const String & str)        { return str.length(); }
	inline strlen_type Length(Char)                      { return 1; }
	template<typename T1, typename T2>
	inline strlen_type Length(const std::pair<T1, T2> a) { return length(a.first) + length(a.second); }

	template<typename T1, typename T2>
	inline void AppendPreAlloc(const std::pair<T1, T2> & p, Char * dest, strlen_type & len)
	{
		AppendPreAlloc(p.first, dest, len);
		AppendPreAlloc(p.second, dest, len);
	}
	template<class String>
	inline void AppendPreAlloc(const String & s, Char * dest, strlen_type & len)
	{
		::memcpy(dest + len, s.data(), s.length() * sizeof(Char));
		len += s.length();
	}
	inline void AppendPreAlloc(Char c, Char * dest, strlen_type & len)
	{
		dest[len] = c;
		++len;
	}


	inline const char * FindCh(const char * toSearch, size_t len, int toFind)
	{
		return static_cast<const char *>(memchr(toSearch, toFind, len));
	}

	inline const wchar_t * FindCh(const wchar_t * toSearch, size_t len, wchar_t toFind)
	{
		return static_cast<const wchar_t *>(wmemchr(toSearch, toFind, len));
	}


	template<typename Char>
	inline int Compare(basic_string_ref<Char> a, basic_string_ref<Char> b)
	{
		auto minLen = std::min(a.size(), b.size());
		int result = memcmp(a.data(), b.data(), minLen * sizeof(Char));
		if (result != 0)
			return result;

		return a.size() - b.size();
	}

	template<typename String> inline
	String RTrimStd(String && str, const std::locale & loc)
	{
		strlen_type lastIdx = str.size();
		while (lastIdx > 0)
		{
			--lastIdx;
			if (!std::isspace(str[lastIdx], loc))
				break;
		}
		return left(std::forward<String>(str), str.size() - lastIdx + 1);
	}

	template<typename String, typename StringOrChar> inline
	String RTrim(String && str, StringOrChar whiteSpace)
	{
		return left(std::forward<String>(str), str.size() - find_last_not_of(str, whiteSpace) + 1);
	}

	template<typename String>
	String TrimStd(String && str, const std::locale & loc)
	{
		strlen_type firstIdx = 0;
		for (; firstIdx < str.size(); ++firstIdx)
		{
			if (!std::isspace(str[firstIdx], loc))
				break;
		}
		return substr(RTrimStd(std::forward<String>(str), loc), firstIdx);
	}

	template<typename String, typename StringOrChar>
	String Trim(String && str, StringOrChar whiteSpace)
	{
		auto firstIdx = find_first_not_of(str, whiteSpace);
		return substr(RTrim(std::forward<String>(str), whiteSpace), firstIdx);
	}

	//template<size_t Size> inline
	//strlen_type copyCStrMin(Char * dest, basic_string_ref<Char> source)
	//{
	//	static_assert(Size > 0, "copy_cstr_min does not support empty array");
	//	auto cpyLen = std::min(source.size(), Size - 1);
	//	::memcpy(dest, source.data(), cpyLen * sizeof(Char));
	//	dest[cpyLen] = '\0';
	//	return cpyLen;
	//}

//-----------------------------------------------------------------------------
// StringBase and basic_string_ref implementation

	template<typename Char>
	inline bool StringBase<Char>::empty() const NOEXCEPT
	{
		return 0 == _len;
	}

	template<typename Char>
	inline typename StringBase<Char>::const_iterator  StringBase<Char>::begin() const NOEXCEPT {
		return {_data, this};
	}
	template<typename Char>
	inline typename StringBase<Char>::const_iterator  StringBase<Char>::end() const NOEXCEPT {
		return {_data + _len, this};
	}

	template<typename Char>
	inline typename StringBase<Char>::const_reference  StringBase<Char>::at(strlen_type index) const
	{
		if (size() > index)
			return _data[index];
		else
			throw out_of_range("Invalid basic_string subscript");
	}

	template<typename Char>
	inline typename StringBase<Char>::const_reference  StringBase<Char>::operator[](strlen_type index) const NOEXCEPT
	{
		MEM_BOUND_ASSERT(size() > index);
		return _data[index];
	}
}

template<typename Char>
inline basic_string_ref<Char>::basic_string_ref() NOEXCEPT
{
	_data = nullptr;
	_len = 0;
}

template<typename Char, strlen_type Size>
inline basic_string_ref<Char>::basic_string_ref(const Char (&toWrap)[Size]) NOEXCEPT
{
	_data = const_cast<Char *>(toWrap);
	_len = Size - 1;
}

template<typename Char>
inline basic_string_ref<Char>::basic_string_ref(const Char * toWrap, size_type len) NOEXCEPT
{
	_data = const_cast<Char *>(toWrap);
	_len = len;
}

template<typename Char>
inline bool basic_string_ref<Char>::starts_with(Char ch) const NOEXCEPT {
	return string_base::starts_with(ch);
}
template<typename Char>
inline bool basic_string_ref<Char>::starts_with(basic_string_ref<Char> str) const NOEXCEPT {
	return string_base::starts_with(str);
}

template<typename Char>
inline bool basic_string_ref<Char>::ends_with(Char ch) const NOEXCEPT {
	return string_base::ends_with(ch);
}
template<typename Char>
inline bool basic_string_ref<Char>::ends_with(basic_string_ref<Char> str) const NOEXCEPT {
	return string_base::ends_with(str);
}

//-----------------------------------------------------------------------------
// basic_string implementation

template<typename Char>
inline basic_string<Char>::basic_string() NOEXCEPT
{
	_data = nullptr;
	_len = 0;
}

template<typename Char>
inline basic_string<Char>::basic_string(basic_string<Char> && source) NOEXCEPT
{
	_data = source._data;
	_len = source._len;
	source._data = nullptr;
}

template<typename Char>
inline basic_string<Char> & basic_string<Char>::operator =(basic_string<Char> && right) NOEXCEPT
{
	using std::swap;
	swap(_data, right._data);
	_len = right._len;
	return *this;
}

template<typename Char>
inline basic_string<Char>::basic_string(const basic_string<Char> & source) {
	_init(source.data(), source.size());
}
template<typename Char>
inline basic_string<Char>::basic_string(const basic_string_ref<Char> & source) {
	_init(source.data(), source.size());
}

template<typename Char>
inline basic_string<Char>::basic_string(const Char * source, size_type lenToCopy)
{
	assert(source || 0 == lenToCopy);
	_init(source, lenToCopy);
}

template<typename Char>
inline basic_string<Char>::basic_string(size_type len)
{
	_data = _alloc(len + 1);
	_len = len;
	_data[_len] = _nullChar;
}

template<typename Char>
inline basic_string<Char> & basic_string<Char>::operator =(const basic_string<Char> & right)
{
	if (this != &right)
		assign(right.data(), right.size());

	return *this;
}

template<typename Char, strlen_type Size>
inline basic_string<Char>::basic_string(const Char (&source)[Size])
{
	_init(source);
}

template<typename Char, strlen_type Size>
inline basic_string<Char> & basic_string<Char>::operator =(const Char (&right)[Size])
{
	_assign(right);
	return *this;
}

template<typename Char, typename T>
basic_string<Char>::basic_string(const concat_str<T> & source)
{
	auto allocSize = source.length() + 1;
	_data = _alloc(allocSize);
	_len = 0;

	_strDetail::AppendPreAlloc(source.data(), _data, _len);
	assert(_len < allocSize);
	_data[_len] = _nullChar;
}

template<typename Char, typename T>
basic_string<Char> & basic_string<Char>::operator =(const concat_str<T> & right)
{
	auto allocSize = right.length() + 1;
	Char *const newData = _alloc(allocSize);
	size_type newLen = 0;

	_strDetail::AppendPreAlloc(right.data(), newData, newLen);
	assert(newLen < allocSize);
	newData[newLen] = _nullChar;
	_resetData(newData);
	_len = newLen;

	return *this;
}

template<typename Char>
inline basic_string<Char>::~basic_string() NOEXCEPT
{
	_dealloc(_data);
}

template<typename Char>
basic_string<Char> & basic_string<Char>::operator =(const basic_string_ref<Char> & right)
{
	if (_data != right.data() || _len != right.size())
	{
		auto newData = _alloc(right.size() + 1);
		::memcpy(newData, right.data(), right.size() * sizeof(Char));
		newData[right.size()] = _nullChar;
		_dealloc(_data);
		_data = newData;
		_len = right.size();
	}
	return *this;
}

template<typename Char>
void basic_string<Char>::assign(const Char * source, size_type lenToCopy)
{
	assert(source || 0 == lenToCopy);

	_resetData(_alloc(lenToCopy + 1));
	_len = lenToCopy;
	_copyData(source);
}

template<typename Char>
inline void basic_string<Char>::swap(basic_string<Char> & other) NOEXCEPT
{
	using std::swap;
	swap(_data, other._data);
	swap(_len, other._len);
}

template<typename Char>
inline void basic_string<Char>::clear() NOEXCEPT
{
	_resetData(nullptr);
	_len = 0;
}

template<typename Char>
inline typename basic_string<Char>::iterator  basic_string<Char>::begin() NOEXCEPT {
	return {_data, this};
}
template<typename Char>
inline typename basic_string<Char>::const_iterator  basic_string<Char>::begin() const NOEXCEPT {
	return string_base::begin();
}

template<typename Char>
inline typename basic_string<Char>::iterator  basic_string<Char>::end() NOEXCEPT {
	return {_data + _len, this};
}
template<typename Char>
inline typename basic_string<Char>::const_iterator  basic_string<Char>::end() const NOEXCEPT {
	return string_base::end();
}

template<typename Char>
inline typename basic_string<Char>::reference  basic_string<Char>::at(size_type index)
{
	return const_cast<reference>(string_base::at(index));
}
template<typename Char>
inline typename basic_string<Char>::const_reference  basic_string<Char>::at(size_type index) const
{
	return string_base::at(index);
}

template<typename Char>
inline typename basic_string<Char>::reference  basic_string<Char>::operator[](size_type index) NOEXCEPT
{
	MEM_BOUND_ASSERT(size() > index);
	return _data[index];
}
template<typename Char>
inline typename basic_string<Char>::const_reference  basic_string<Char>::operator[](size_type index) const NOEXCEPT
{
	return string_base::operator[](index);
}

template<typename Char>
inline const Char * basic_string<Char>::c_str() const NOEXCEPT
{
	if (_data)
		return _data;
	else
		return &_nullChar;
}

template<typename Char>
inline bool basic_string<Char>::starts_with(Char ch) const NOEXCEPT {
	return string_base::starts_with(ch);
}
template<typename Char>
inline bool basic_string<Char>::starts_with(basic_string_ref<Char> str) const NOEXCEPT {
	return string_base::starts_with(str);
}

template<typename Char>
inline bool basic_string<Char>::ends_with(Char ch) const NOEXCEPT {
	return string_base::ends_with(ch);
}
template<typename Char>
inline bool basic_string<Char>::ends_with(basic_string_ref<Char> str) const NOEXCEPT {
	return string_base::ends_with(str);
}

template<typename Char>
inline void basic_string<Char>::truncate(iterator newEnd) NOEXCEPT
{
	_len = newEnd - begin();
	*newEnd = _nullChar;
}

template<typename Char>
inline void basic_string<Char>::shorten_to(size_type newLen) NOEXCEPT
{
	if (newLen < _len)
	{
		_len = newLen;
		_data[newLen] = _nullChar;
	}
}

template<typename Char>
inline void basic_string<Char>::erase_idx(size_type index)
{
	if (size() > index)
	{
		Char *const pFirst = _data + index;
		::memmove(pFirst, pFirst + 1, (_len - (index + 1)) * sizeof(Char)); // copy [pos + 1, _end) to [pos, _end - 1)
		--_len;
	}
	else
		throw out_of_range("Invalid basic_string erase_idx");
}

template<typename Char>
inline typename basic_string<Char>::iterator  basic_string<Char>::erase(iterator pos) NOEXCEPT
{
	Char *const next = std::addressof(*pos) + 1;
	::memmove(to_ptr(pos), next, ((data() + _len) - next) * sizeof(Char)); // copy [pos + 1, _end) to [pos, _end - 1)
	--_len;
	return pos;
}

template<typename Char>
inline typename basic_string<Char>::iterator  basic_string<Char>::erase(iterator first, iterator last) NOEXCEPT
{
	MEM_BOUND_ASSERT(data() <= to_ptr(first) && to_ptr(first) <= to_ptr(last));
	if (first < last)
	{
		size_type nAfterLast = end() - last;
		_len -= last - first;
		::memmove(to_ptr(first), to_ptr(last), nAfterLast * sizeof(Char));
		_data[_len] = _nullChar;
	}
	return first;
}

template<typename Char>
inline void basic_string<Char>::erase(size_type index, size_type count) NOEXCEPT
{
	MEM_BOUND_ASSERT(index <= _len);
	count = std::min(count, _len - index);
	if (0 < count)
	{
		size_type const newLen = _len - count;
		Char *const pFirst = _data + index;
		size_type nElemsAfter = newLen - index;
		::memmove(pFirst, pFirst + count, nElemsAfter * sizeof(Char));
		_data[newLen] = _nullChar;
		_len = newLen;
	}
}

template<typename Char>
basic_string<Char>::operator const basic_string_ref<Char> &() const NOEXCEPT
{
	return static_cast<const basic_string_ref<Char> &>(static_cast<const string_base &>(*this));
}

//void basic_string<Char>::resize(size_type newLen)
//{
//	if (newLen != _len)
//	{
//		Char * newData = _alloc(newLen + 1);
//		::memcpy(newData, _data, _len * sizeof(Char));
//		newData[newLen] = _nullChar;
//		_dealloc(_data);
//		_data = newData;
//		_len = newLen;
//	}
//}

} // namespace oetl

//-----------------------------------------------------------------------------

template<typename Char>
inline bool oetl::operator==(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT
{
	return left.size() == right.size() &&
		memcmp(left.data(), right.data(), left.size() * sizeof(Char)) == 0;
}
template<typename Char>
inline bool oetl::operator!=(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT {
	return !(left == right);
}
template<typename Char>
inline bool oetl::operator <(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT {
	return _strDetail::Compare(left, right) < 0;
}
template<typename Char>
inline bool oetl::operator >(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT {
	return _strDetail::Compare(left, right) > 0;
}
template<typename Char>
inline bool oetl::operator<=(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT {
	return _strDetail::Compare(left, right) <= 0;
}
template<typename Char>
inline bool oetl::operator>=(basic_string_ref<Char> left, basic_string_ref<Char> right) NOEXCEPT {
	return _strDetail::Compare(left, right) >= 0;
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::to_string_ref(const Char * cStr) NOEXCEPT
{
	return {cStr, strlen(cStr)};
}


template<typename Char, oetl::strlen_type Size>
inline oetl::strlen_type oetl::copy_cstr_min(basic_string_ref<Char> source, Char (&dest)[Size]) NOEXCEPT
{
	auto cpyLen = std::min(source.size(), Size - 1);
	::memcpy(dest, source.data(), cpyLen * sizeof(Char));
	dest[cpyLen] = '\0';
	return cpyLen;
}

template<typename Char>
inline oetl::strlen_type oetl::copy_cstr_min(basic_string_ref<Char> source, Char * dest, strlen_type destSize) NOEXCEPT
{
	strlen_type cpyLen;
	if (0 < destSize)
	{
		cpyLen = std::min(source.size(), destSize - 1);
		::memcpy(dest, source.data(), cpyLen * sizeof(Char));
		dest[cpyLen] = '\0';
	}
	else
	{
		cpyLen = 0;
	}
	return cpyLen;
}

template<typename Char, oetl::strlen_type Size>
inline void oetl::copy_cstr(basic_string_ref<Char> source, Char (&dest)[Size])
{
	copy_cstr(source, begin(dest), Size);
}

template<typename Char>
inline void oetl::copy_cstr(basic_string_ref<Char> source, Char * dest, strlen_type destSize)
{
	if (source.size() < destSize)
	{
		::memcpy(dest, source.data(), source.size() * sizeof(Char));
		dest[source.size()] = '\0';
	}
	else
		throw std::length_error("copy_cstr destination too small");
}


template<typename Char>
inline oetl::strlen_type oetl::find_idx(basic_string_ref<Char> toSearch, Char ch, strlen_type pos) NOEXCEPT
{
	if (pos < toSearch.size())
		return find_idx(toSearch.data() + pos, toSearch.size() - pos, ch);
	else
		return str_npos;
}

template<typename Char>
inline oetl::strlen_type oetl::rfind_idx(basic_string_ref<Char> toSearch, Char ch, strlen_type pos) NOEXCEPT
{
	pos = std::min(pos, toSearch.size());
	return rfind_idx(basic_string_ref(toSearch.data(), pos), ch);
}

template<typename Char>
inline oetl::strlen_type oetl::find_last_not_of(basic_string_ref<Char> toSearch, Char notOf, strlen_type pos) NOEXCEPT
{
	pos = std::min(pos, toSearch.size());
	while (--pos != strlen_type(-1))
	{
		if (toSearch[pos] != notOf)
			break;
	}
	return pos;
}

template<typename Char>
inline oetl::strlen_type oetl::find_first_not_of(basic_string_ref<Char> toSearch, Char notOf, strlen_type pos) NOEXCEPT
{
	for (; pos < toSearch.size(); ++pos)
	{
		if (toSearch[pos] != notOf)
			return pos;
	}
	return str_npos;
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::substr(basic_string_ref<Char> str, strlen_type startPos) NOEXCEPT
{	// Returns a wrapper referencing substring of str. str may itself be wrapper of some other basic_string data
	startPos = std::min(startPos, str.length());
	strlen_type count = str.length() - startPos;
	return {str.data() + startPos, count};
}
template<typename Char>
inline oetl::basic_string_ref<Char> oetl::substr(basic_string_ref<Char> str, strlen_type startPos, strlen_type count) NOEXCEPT
{
	startPos = std::min(startPos, str.length());
	count = std::min(count, str.length() - startPos);
	return {str.data() + startPos, count};
}

template<typename Char>
inline oetl::basic_string<Char> oetl::substr(basic_string<Char> && str, strlen_type startPos) NOEXCEPT
{
	str.erase(0, startPos);
	return str;
}
template<typename Char>
inline oetl::basic_string<Char> oetl::substr(basic_string<Char> && str, strlen_type startPos, strlen_type count) NOEXCEPT
{
	str.erase(0, startPos);
	str.shorten_to(count);
	return str;
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::left(basic_string_ref<Char> str, strlen_type count) NOEXCEPT
{	// Returns a wrapper referencing substring of str. str may itself be wrapper of some other basic_string data
	count = std::min(count, str.length());
	return {str.data(), count};
}
template<typename Char>
inline oetl::basic_string<Char> oetl::left(basic_string<Char> && str, strlen_type count) NOEXCEPT
{
	str.shorten_to(count);
	return str;
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::right(basic_string_ref<Char> str, strlen_type count) NOEXCEPT
{
	count = std::min(count, str.length());
	return {str.data() + (str.length() - count), count};
}
template<typename Char>
inline oetl::basic_string<Char> oetl::right(basic_string<Char> && str, strlen_type count) NOEXCEPT
{
	str.erase(0, str.length() - count);
	return str;
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::rtrim(basic_string_ref<Char> str, std::locale loc)
{
	return _strDetail::RTrimStd(std::move(str), loc);
}
template<typename Char>
inline oetl::basic_string<Char> oetl::rtrim(basic_string<Char> && str, std::locale loc)
{
	return _strDetail::RTrimStd(std::move(str), loc);
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::rtrim(basic_string_ref<Char> str, Char toErase) NOEXCEPT
{
	return _strDetail::RTrim(std::move(str), toErase);
}
template<typename Char>
inline oetl::basic_string<Char> oetl::rtrim(basic_string<Char> && str, Char toErase) NOEXCEPT
{
	return _strDetail::RTrim(std::move(str), toErase);
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::rtrim(basic_string_ref<Char> str, basic_string_ref<Char> charsToErase) NOEXCEPT
{
	return _strDetail::RTrim(std::move(str), charsToErase);
}
template<typename Char>
inline oetl::basic_string<Char> oetl::rtrim(basic_string<Char> && str, basic_string_ref<Char> charsToErase) NOEXCEPT
{
	return _strDetail::RTrim(std::move(str), charsToErase);
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::trim(basic_string_ref<Char> str, std::locale loc)
{
	return _strDetail::TrimStd(std::move(str), loc);
}
template<typename Char>
inline oetl::basic_string<Char> oetl::trim(basic_string<Char> && str, std::locale loc)
{
	return _strDetail::TrimStd(std::move(str), loc);
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::trim(basic_string_ref<Char> str, Char toErase) NOEXCEPT
{
	return _strDetail::Trim(std::move(str), toErase);
}
template<typename Char>
inline oetl::basic_string<Char> oetl::trim(basic_string<Char> && str, Char toErase) NOEXCEPT
{
	return _strDetail::Trim(std::move(str), toErase);
}

template<typename Char>
inline oetl::basic_string_ref<Char> oetl::trim(basic_string_ref<Char> str, basic_string_ref<Char> charsToErase) NOEXCEPT
{
	return _strDetail::Trim(std::move(str), charsToErase);
}
template<typename Char>
inline oetl::basic_string<Char> oetl::trim(basic_string<Char> && str, basic_string_ref<Char> charsToErase) NOEXCEPT
{
	return _strDetail::Trim(std::move(str), charsToErase);
}

template<typename Char>
oetl::strlen_type oetl::find_str(basic_string_ref<Char> toSearch, basic_string_ref<Char> str, strlen_type pos) NOEXCEPT
{
	if (!str.empty())
	{
		strlen_type nm;
		if (pos < toSearch.size() && str.size() <= (nm = toSearch.size() - pos))
		{	// room for match, look for it
			const Char * uPtr;
			const Char * vPtr;
			for (nm -= str.size() - 1, vPtr = toSearch.data() + pos;
				(uPtr = _detail::FindCh(vPtr, nm, str[0])) != nullptr;
				nm -= uPtr - vPtr + 1, vPtr = uPtr + 1)
			{
				if (memcmp(uPtr, str.data(), str.size() * sizeof(Char)) == 0)
					return (uPtr - toSearch.data());  // found a match
			}
		}
	}
	else if (pos <= toSearch.size())
	{
		return pos;  // empty basic_string always matches (if inside basic_string)
	}
	return str_npos;  // no match
}
/*
oetl::strlen_type oetl::rfind(basic_string_ref<Char> toSearch, basic_string_ref<Char> str, strlen_type maxPos) NOEXCEPT
{
	//auto diff = static_cast<c_str_wrap::difference_type>(toSearch.length()) - static_cast<c_str_wrap::difference_type>(str.length());
	//if (diff >= 0)
	if (toSearch.length() >= str.length())
	{
		strlen_type pos = std::min(maxPos, toSearch.length() - str.length());
		for (; pos != -1; --pos)
		{
			bool equal = true;
			for (strlen_type sub = 0; sub < str.length() && equal; ++sub)
				if (toSearch[pos + sub] != str[sub])
					equal = false;

			if (equal)
				return pos;
		}
	}
	return str_npos;
}
*/
template<typename Char>
oetl::strlen_type oetl::rfind_str(basic_string_ref<Char> toSearch, basic_string_ref<Char> str, strlen_type pos) NOEXCEPT
{
	//if (str.empty())
	//{
	//	return pos < toSearch.size() ?
	//		   pos :
	//		   toSearch.size();  // empty always matches
	//}
	if (str.size() <= toSearch.size())
	{	// room for match, look for it
		const Char * ptr = toSearch.data() +
				(pos < toSearch.size() - str.size() ?
				 pos :
				 toSearch.size() - str.size());
		for (;; --ptr)
		{
			if (memcmp(ptr, str.data(), str.size() * sizeof(Char)) == 0)
				return (ptr - toSearch.data()); // found a match
			else if (ptr == toSearch.data())
				break;  // at beginning, no more chance for match
		}
	}
	return str_npos;  // no match
}

template<typename Char>
oetl::strlen_type oetl::find_first_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> oneOf, strlen_type pos) NOEXCEPT
{
	if (0 < oneOf.size() && pos < toSearch.size())
	{	// room for match, look for it
		const Char *const end = toSearch.data() + toSearch.size();
		for (const Char * ptr = toSearch.data() + pos; ptr < end; ++ptr)
		{
			if (_detail::FindCh(oneOf.data(), oneOf.size(), *ptr))
				return (ptr - toSearch.data());  // found a match
		}
	}
	return str_npos;  // no match
}

template<typename Char>
oetl::strlen_type oetl::find_first_not_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> notOf, strlen_type pos) NOEXCEPT
{
	if (pos < toSearch.size())
	{	// room for match, look for it
		const Char *const end = toSearch.data() + toSearch.size();
		for (const Char * ptr = toSearch.data() + pos; ptr < end; ++ptr)
		{
			if (_detail::FindCh(notOf.data(), notOf.size(), *ptr) == nullptr)
				return (ptr - toSearch.data());
		}
	}
	return str_npos;
}

template<typename Char>
oetl::strlen_type oetl::find_last_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> oneOf, strlen_type pos) NOEXCEPT
{
	if (0 < oneOf.size() && 0 < toSearch.size())
	{	// worth searching, do it
		const Char * uPtr = toSearch.data() +
				(pos < toSearch.size() ?
				 pos :
				 toSearch.size() - 1);
		for (;; --uPtr)
		{
			if (_detail::FindCh(oneOf.data(), oneOf.size(), *uPtr))
				return (uPtr - toSearch.data());  // found a match
			else if (uPtr == toSearch.data())
				break;  // at beginning, no more chance for match
		}
	}
	return str_npos;  // no match
}

template<typename Char>
oetl::strlen_type oetl::find_last_not_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> notOf, strlen_type pos) NOEXCEPT
{
	if (0 < toSearch.size())
	{	// worth searching, do it
		const Char * uPtr = toSearch.data()
			+ (pos < toSearch.size() ? pos : toSearch.size() - 1);
		for (;; --uPtr)
		{
			if (_detail::FindCh(notOf.data(), notOf.size(), *uPtr) == nullptr)
				return (uPtr - toSearch.data());
			else if (uPtr == toSearch.data())
				break;
		}
	}
	return str_npos;
}

/*
oetl::strlen_type oetl::find_first_not_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> str) NOEXCEPT
{
	for (strlen_type i = 0; i < toSearch.length(); ++i)
	{
		bool noMatch = true;
		for (strlen_type j = 0; j < str.length() && noMatch; ++j)
			noMatch = toSearch[i] != str[j];

		if (noMatch)
			return i;
	}
	return str_npos;
}
oetl::strlen_type oetl::find_last_not_of(basic_string_ref<Char> toSearch, basic_string_ref<Char> str) NOEXCEPT
{
	for (strlen_type i = toSearch.length(); i > 0; )
	{
		--i;
		bool noMatch = true;
		for (strlen_type j = 0; j < str.length() && noMatch; ++j)
			noMatch = toSearch[i] != str[j];

		if (noMatch)
			return i;
	}
	return str_npos;
}
*/

namespace std
{

template<>
class hash<oetl::string_ref>
{
public:
	size_t operator()(oetl::string_ref str) const NOEXCEPT
	{	// hash str to size_t value by pseudorandomizing transform
		size_t val = 2166136261U;
		size_t const last = str.size();
		size_t stride = 1 + last / 10;
		for (size_t pos = 0; pos < last; pos += stride)
			val = 16777619U * val ^ static_cast<size_t>(str[pos]);
		return val;
	}
};

template<>
class hash<oetl::string>
{
public:
	size_t operator()(const oetl::string & str) const NOEXCEPT
	{
		return hash<oetl::string_ref>()(str);
	}
};

} // namespace std
