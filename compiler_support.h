#pragma once

#if !_MSC_VER || _MSC_VER >= 1900
#	define NOEXCEPT noexcept

#	define ALIGNOF alignof
#else
#	define NOEXCEPT throw()

#	define ALIGNOF __alignof
#endif

#if !_MSC_VER || _MSC_VER >= 1800
#	define CPP11_VARIADIC_TEMPL 1
#endif
