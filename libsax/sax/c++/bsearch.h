#ifndef __BSEARCH_H_2009_06__
#define __BSEARCH_H_2009_06__

/// @file bsearch.h some functions for simple binary search on arrays.
/// @author Qingshan Luo (qluo.cn@gmail.com)
/// @date 2009.6, modified 2010.7
///
/// @note to avoid overflow of "size_t" type, we modify 
///    -  size_t mid = (low + high)/2;
///  to a different form:
///    -  size_t mid = low + (high - low)/2;

/* import size_t */
#include <stdlib.h>

namespace sax {

/// @brief binary search an ordered array (increasing) to get the ID.
/// @param arr[] the testing array.
/// @param len length of the arr[].
/// @param x a testing value.
/// @return the ID of the right element.
/// @retval -1 can't find a matching element for x.
template <typename T>
size_t bsearch(T arr[], size_t len, T x)
{
	size_t low=0, high=len-1;
	while (low <= high)
	{
		size_t mid = low + (high-low)/2;
		if(arr[mid]==x) return mid;
		if(arr[mid]< x) low= mid+1;
		else high = mid-1;
	}
	return (size_t)(-1);
}

/// @brief binary search an ordered array (increasing) to get the ID, 
/// corresponds to a matching element, or a neighbor element (value>=x).
/// @param arr[] the testing array.
/// @param len length of the arr[].
/// @param x a testing value.
/// @return the ID of the matching element or a neighbor.
template <typename T>
size_t BinarySearch(T arr[], size_t len, T x)
{
	size_t low=0, high=len-1;
	while (low <= high)
	{
		size_t mid = low + (high-low)/2;
		if(arr[mid]==x) return mid;
		if(arr[mid]< x) low= mid+1;
		else high = mid-1;
	}
	return low; // patch the last line!
}

/// @brief binary search an ordered array (increasing), get pos in ID[],
/// corresponds to a matching element, or a neighbor element (value>=x).
/// @param arr[] the testing array.
/// @param len length of the arr[].
/// @param stride the skip steps between two elements.
/// @param ID[] the ID is stored in an array too.
/// @param x a testing value.
/// @return the ID of the matching element or a neighbor.
template <typename T>
size_t BinarySearch(
	T arr[], size_t len, size_t stride, size_t ID[], T x)
{
	size_t low=0, high=len-1;
	while (low <= high)
	{
		size_t mid = low + (high-low)/2;
		size_t off = ID[mid]*stride;
		if(x==arr[off]) return mid;
		if(x> arr[off]) low= mid+1;
		else high = mid-1;
	}
	return low; // patch the last line!
}

} // namespace

#endif //__BSEARCH_H_2009_06__
