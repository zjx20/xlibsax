#ifndef __QUICK_SORT_H__
#define __QUICK_SORT_H__

/// @file quicksort.h a wrapper for qsort on arrays.
/// @author Qingshan Luo (qluo.cn@gmail.com)
/// @date 2009.6, modified 2010.7
/// @note how to use it, very simple:
/// <pre>
/// int values[] = { 40, 10, 100, 90, 20, 25 };
/// sax::quicksort<int>(values, 6, false);
/// </pre>
#include <sax/qsort2.h>

namespace sax {

template <class T>
class quicksort 
{
public:
	explicit quicksort(T val[], size_t num, bool asc=true)
	{
		if (asc) qsort2(val, num, sizeof(T), &quicksort::c1_asc, 0);
		else qsort2(val, num, sizeof(T), &quicksort::c1_dsc, 0);
	}
	
	explicit quicksort(const T val[], size_t ID[], size_t num, bool asc=true)
	{
		for (size_t j=0; j<num; j++) ID[j] = j;
		if (asc) qsort2(ID, num, sizeof(size_t), &quicksort::c2_asc, (void *)val);
		else qsort2(ID, num, sizeof(size_t), &quicksort::c2_dsc, (void *)val);
	}

private:
	static int c1_asc(const void *a, const void *b, void *user)
	{
		const T *_a = static_cast<const T *>(a);
		const T *_b = static_cast<const T *>(b);
		(void)(user);
		if (*_a < *_b) return -1;
		if (*_a > *_b) return +1;
		return 0;
	}
	static int c1_dsc(const void *a, const void *b, void *user)
	{
		const T *_a = static_cast<const T *>(a);
		const T *_b = static_cast<const T *>(b);
		(void)(user);
		if (*_a < *_b) return +1;
		if (*_a > *_b) return -1;
		return 0;
	}
	
	static int c2_asc(const void *a, const void *b, void *user)
	{
		const T *val = static_cast<const T *>(user);
		const T *_a = val + *static_cast<const size_t *>(a);
		const T *_b = val + *static_cast<const size_t *>(b);
		if (*_a < *_b) return -1;
		if (*_a > *_b) return +1;
		return 0;
	}
	static int c2_dsc(const void *a, const void *b, void *user)
	{
		const T *val = static_cast<const T *>(user);
		const T *_a = val + *static_cast<const size_t *>(a);
		const T *_b = val + *static_cast<const size_t *>(b);
		if (*_a < *_b) return +1;
		if (*_a > *_b) return -1;
		return 0;
	}
};

} // namespace

#endif//__QUICK_SORT_H__
