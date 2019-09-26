#include <stdexcept>

#if defined _CPPUNWIND || defined __EXCEPTIONS
	#define OEL_ABORT(errorMsg) throw std::logic_error(errorMsg)
#endif

