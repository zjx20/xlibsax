#ifndef __QSORT2_H_2011__
#define __QSORT2_H_2011__

/**
 * 1, what's the difference from the qsort in stdlib?
 * qsort2 add a dummy parameter "user", and deliver it to comparor.
 *
 * 2. why do we still care for qsort so much, here is the answer:
 * qsort easily beats std::sort() 100+ times!
 * http://www.codeguru.com/forum/archive/index.php/t-201353.html
 *
 * (a) Especially when memory usage is near full, std::sort() 
 * will be literally brought down to craw on the ground, desperately 
 * trying to create new objects, allocate memory, memcpy() memory, 
 * then destroy object, all in an effort that is un-necessary and wasted. 
 *
 * (b) While as qsort() is flying, sorting objects by just swapping
 * the 4bytes pointer that represent the class object.
 *
 * (c) No wonder qsort() can easily defeat std::sort() by 100+ times
 * or even more in such case.
 */

/* import size_t */
#include <stdlib.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void qsort2(void *pbase, size_t total_elems, size_t size,
	int (*cmp)(const void *, const void *, void *), void *user);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
