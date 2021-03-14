#include <view/counted.h>

constexpr auto v()
{
	return oel::view::counted((int *)nullptr, 0);
}