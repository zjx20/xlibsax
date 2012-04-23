#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "binary.h"

namespace sax {

void binary::destroy() 
{
	if (_start) {
		free(_start);
		_end = _free = _data = _start = NULL;
	}
}

int binary::find(const void *pattern, int len) 
{
	const char *p = (const char *) pattern;
	int dLen = _free - _data - len + 1;
	for (int i=0; i<dLen; i++) {
		if (_data[i] == p[0] && memcmp(_data+i, p, len) == 0) 
		{
			return i;
		}
	}
	return -1;
}

void binary::shrink() 
{
	const int MAX_SIZE = 2048;
	if (_start == NULL) {
		return;
	}
	if ((_end - _start) <= MAX_SIZE || (_free - _data) > MAX_SIZE) {
		return;
	}

	int dlen = _free - _data;
	if (dlen < 0) dlen = 0;

	char *newbuf = (char *) malloc(MAX_SIZE);

	if (dlen > 0) {
		memcpy(newbuf, _data, dlen);
	}
	free(_start);

	_data = _start = newbuf;
	_free = _start + dlen;
	_end = _start + MAX_SIZE;

	return;
}

bool binary::ensure(int need) 
{
	if (_start == NULL) 
	{
		int len = 256;
		while (len < need) len <<= 1;
		
		_start = (char *) malloc(len);
		if (!_start) return false;
		
		_free = _data = _start;
		_end = _start + len;
		return true;
	}
	
	if (_end - _free < need) // enough?
	{ 
		int flen = (_end - _free) + (_data - _start);
		int dlen = _free - _data;

		// not enough, or the idle is below 20%:
		if (flen < need || flen * 4 < dlen) 
		{
			int bufsize = (_end - _start) * 2;
			while (bufsize - dlen < need)
				bufsize <<= 1;

			char *neu = (char *) malloc(bufsize);
			if (!neu) return false;
			
			if (dlen > 0) {
				memcpy(neu, _data, dlen);
			}
			free(_start);

			_data = _start = neu;
			_free = _start + dlen;
			_end = _start + bufsize;
		}
		else {
			memmove(_start, _data, dlen);
			_data = _start;
			_free = _start + dlen;
		}
	}
	return true;
}

} // namespace

