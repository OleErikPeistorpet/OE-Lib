#pragma once

#ifndef NOEXCEPT
#	if !_MSC_VER || _MSC_VER >= 1900
#		define NOEXCEPT noexcept
#	else
#		define NOEXCEPT throw()
#	endif
#endif

#if !_MSC_VER || _MSC_VER >= 1800
#	define CPP11_VARIADIC_TEMPL 1

#	define CPP11_INITIALIZER_LIST 1
#endif
