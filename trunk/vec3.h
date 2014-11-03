#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "debug.h"

#include <cmath>


template<typename> class vec3length;  // forward declare


template<typename T>
struct vec3
{
	T x, y, z;


	vec3   operator +(const vec3 & rhs) const;
	vec3   operator -(const vec3 & rhs) const;

	vec3   operator -() const;

	vec3 & operator+=(const vec3 & rhs);
	vec3 & operator-=(const vec3 & rhs);

	vec3 & operator*=(T k);

	void set(T x, T y, T z);

///	Orthogonal vector projection on a.
/**	The vector component of u along a =
		u.project_on(a)
	If you know a is unit vector (normalized), use instead:
		a * u.dot(a)
	The vector component of u perpendicular to a =
		u - u.project_on(a)  */
	vec3 project_on(const vec3 & a) const;
};

template<typename T>
vec3<T>   operator *(T k, const vec3<T> & v);

template<typename T>
T       dot(const vec3<T> & u, const vec3<T> & v);
template<typename T>
vec3<T> cross(const vec3<T> & u, const vec3<T> & v);

template<typename U, typename T> inline
bool operator <(U k, vec3length<T> vl)  { return vl > k; }
template<typename U, typename T> inline
bool operator >(U k, vec3length<T> vl)  { return vl < k; }
template<typename U, typename T> inline
bool operator==(U k, vec3length<T> vl)  { return vl == k; }
template<typename U, typename T> inline
bool operator!=(U k, vec3length<T> vl)  { return vl != k; }

template<typename T>
vec3length<T> length(const vec3<T> & v);
template<typename T>
T             len_square(const vec3<T> & v);  ///< Magnitude squared
/** @brief 1 / magnitude. Returns infinity if magnitude = 0.
*
* The cosine of the angle between vectors u and v =
*		dot(u, v) * recipr_len(u) * recipr_len(v)
* (skip multiplying with recipr_len for vectors of magnitude 1 (normalized))  */
template<typename T>
T             recipr_len(const vec3<T> & v);
template<typename T>
void          normalize_unsf(vec3<T> & v);   ///< Undefined for zero vector
template<typename T>
bool          check_normalize(vec3<T> & v);  ///< Returns false for zero vector

template<typename T>
vec3<T> make_vec(T x, T y, T z);
template<typename T>
vec3<T> make_vec3(T fillVal);
template<typename T>
vec3<T> make_vec(const T (&array)[3]);

/// Return element at Index in vec3 v
template<int Index, typename T>
T &       get(vec3<T> & v);
/// Return element at Index in const vec3 v
template<int Index, typename T>
const T & get(const vec3<T> & v);

typedef vec3<float>  vec3f;
typedef vec3<double> vec3d;


template<typename T>
class vec3length
{
public:
	vec3length(T lenSquare)  : lenSqr(lenSquare) {}

	operator T() const  { return sqrt(lenSqr); }

	bool operator <(vec3length rhs)  { return lenSqr < rhs.lenSqr; }
	bool operator >(vec3length rhs)  { return lenSqr > rhs.lenSqr; }
	bool operator==(vec3length rhs)  { return lenSqr == rhs.lenSqr; }
	bool operator!=(vec3length rhs)  { return lenSqr != rhs.lenSqr; }

	template<typename U>
	bool operator <(U k)  { return lenSqr < k * k; }
	template<typename U>
	bool operator >(U k)  { return lenSqr > k * k; }
	template<typename U>
	bool operator==(U k)  { return lenSqr == k * k; }
	template<typename U>
	bool operator!=(U k)  { return lenSqr != k * k; }

private:
	T lenSqr;
};


//-----------------------------------------------------------------------------


inline float rcpr_sqrt(float num)
{
	return 1.0F / sqrt(num);
}
inline double rcpr_sqrt(double num)
{
	return 1.0 / sqrt(num);
}

#define T_ZERO T()

template<typename T>
inline vec3<T> vec3<T>::operator -() const
{
	return {-x, -y, -z};
}

template<typename T>
inline vec3<T> vec3<T>::operator +(const vec3 & rhs) const
{
	return {x + rhs.x,
	        y + rhs.y,
	        z + rhs.z};
}

template<typename T>
inline vec3<T> vec3<T>::operator -(const vec3 & rhs) const
{
	return {x - rhs.x,
	        y - rhs.y,
	        z - rhs.z};
}

template<typename T>
inline vec3<T> & vec3<T>::operator+=(const vec3 & rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	return *this;
}

template<typename T>
inline vec3<T> & vec3<T>::operator-=(const vec3 & rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	return *this;
}

template<typename T>
inline vec3<T> & vec3<T>::operator*=(T k)
{
	x *= k;
	y *= k;
	z *= k;
	return *this;
}

template<typename T>
inline vec3<T> operator *(T k, const vec3<T> & v)
{
	return {k * v.x,
	        k * v.y,
	        k * v.z};
}

template<typename T>
inline T dot(const vec3<T> & u, const vec3<T> & v)
{
	return (u.x * v.x
	      + u.y * v.y
	      + u.z * v.z);
};

template<typename T>
inline vec3<T> cross(const vec3<T> & u, const vec3<T> & v)
{
	return {u.y * v.z - u.z * v.y,
	        u.z * v.x - u.x * v.z,
	        u.x * v.y - u.y * v.x};
}

template<typename T>
inline vec3length<T> length(const vec3<T> & v)
{
	return vec3length<T>(len_square(v));
}

template<typename T>
inline T len_square(const vec3<T> & v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

template<typename T>
inline T recipr_len(const vec3<T> & v)
{
	return rcpr_sqrt(len_square(v));
}

template<typename T>
inline void normalize_unsf(vec3<T> & v)
{
	T lenSqr = len_square(v);
	ASSERT(lenSqr != T_ZERO);
	T rcpLen = rcpr_sqrt(lenSqr);
	//ASSERT(std::isfinite(rcpLen));
	v *= rcpLen;
}

template<typename T>
bool check_normalize(vec3<T> & v)
{
	T lenSqr = len_square(v);
	bool nonZero = (lenSqr != T_ZERO);
	if (nonZero)
	{
		T rcpLen = rcpr_sqrt(lenSqr);
		v *= rcpLen;
	}

	return nonZero;
}

template<typename T>
vec3<T> vec3<T>::project_on(const vec3 & a) const
{
	T aLenSqr = len_square(a);
	if (aLenSqr != T_ZERO)
		return (dot(*this, a) / aLenSqr) * a;
	else
		return a;
}

template<typename T>
inline vec3<T> make_vec(T x, T y, T z)
{
	return {x, y, z};
}

template<typename T>
inline vec3<T> make_vec3(T fill)
{
	return {fill, fill, fill};
}

template<typename T>
inline vec3<T> make_vec(const T (&array)[3])
{
	return {arr[0], arr[1], arr[2]};
}

template<typename T>
inline void vec3<T>::set(T newX, T newY, T newZ)
{
	x = newX;
	y = newY;
	z = newZ;
}

namespace _detail
{
	template<int Index, typename T>
	struct vec3Get
	{	T & operator()(const vec3<T> & v)
		{
			static_ASSERT(Index >= 0 && Index < 3, "Invalid Index for get(vec3 &)");
		}
	};

	template<typename T> struct vec3Get<0, T>
	{
		T &       operator()(vec3<T> & v)       { return v.x; }
		const T & operator()(const vec3<T> & v) { return v.x; }
	};
	template<typename T> struct vec3Get<1, T>
	{
		T &       operator()(vec3<T> & v)       { return v.y; }
		const T & operator()(const vec3<T> & v) { return v.y; }
	};
	template<typename T> struct vec3Get<2, T>
	{
		T &       operator()(vec3<T> & v)       { return v.z; }
		const T & operator()(const vec3<T> & v) { return v.z; }
	};
}

template<int Index, typename T>
inline T & get(vec3<T> & v)
{
	_detail::vec3Get<Index, T> g;
	return g(v);
}
template<int Index, typename T>
inline const T & get(const vec3<T> & v)
{
	_detail::vec3Get<Index, T> g;
	return g(v);
}

#undef T_ZERO
