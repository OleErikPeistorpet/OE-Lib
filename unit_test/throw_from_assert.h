#include <stdexcept>

#if defined _CPPUNWIND || defined __EXCEPTIONS
	#define OEL_ABORT(errorMsg) throw std::logic_error(errorMsg)

#elif defined _MSC_VER
	#pragma warning(disable: 4577) // 'noexcept' used with no exception handling mode specified
#endif
